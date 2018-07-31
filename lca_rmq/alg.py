# coding: utf-8
from itertools import chain, combinations
import json
import multiprocessing
from pymongo import MongoClient
import random
import redis


from util import setupLogger


logger = setupLogger(__name__, 'log/{}.log'.format("lca_rmq.alg.py"), 300)


class TreeAlg(object):
    cache = redis.Redis()
    rmq_cache_key = "rmq:idx1:{:>04}:idx2:{:>04}"
    rmq_cache_expire = 720

    # euler_tour_cache_key = 'euler_tour'
    # euler_idx_map_cache_key = 'euler_idx_map'

    def __init__(self, db):
        self.db = db
        self.hpo_dag = 'hpo_tree'
        self.hpo_split_tree = 'hpo_split_tree'
        self.hpo_split_dag = 'hpo_split_dag'
        self.lca_tree = 'lca_tree'
        self.rmq = 'temp_rmq'
        self.pid = multiprocessing.current_process().pid
        self.cache.flushdb()

        self._euler_tour = None
        self._euler_idx_map = None

    @property
    def index_in_euler_tour(self):
        cur = self.db['meta'].find({"key": "index_in_euler_tour"})
        return map(lambda d: (d[u'hpo'], d[u'index']), cur)

    @property
    def level_of_euler_tour(self):
        level = self.db['meta'].find_one({"key": "level_of_euler_tour"})[u'value'] 
        return level

    @property
    def euler_tour(self):
        if self._euler_tour is not None:
            return self._euler_tour
        self._euler_tour = self.db['meta'].find_one({"key": "euler_tour"})[u'value']
        return self._euler_tour

    @property
    def euler_idx_map(self):
        if self._euler_idx_map is not None:
            return self._euler_idx_map
        cur = self.db['meta'].find({"key": "index_in_euler_tour"})
        self._euler_idx_map = {d[u'hpo']: d[u'index'] for d in cur}
        return self._euler_idx_map

    def all_pair_RMQ(self):
        level = self.level_of_euler_tour
        l_idx = map(lambda t: t[1], self.index_in_euler_tour)

        start_range = max(self.db['temp_rmq'].find({"idx1": l_idx[0]}).count(), 1)
        self._all_pair_RMQ(l_idx, level, start_range=start_range)

    def one_pair_lca_in_tree(self, hpo1, hpo2):
        # TODO: 测试
        if hpo1 == hpo2:
            return hpo1
        idx1, idx2 = map(
            self._get_first_idx_in_euler_tour_by_hpo,
            [hpo1, hpo2]
        )
        if idx2 < idx1:
            idx1, idx2 = idx2, idx1
        min_idx = self.db['temp_rmq'].find_one({"idx1": idx1, "idx2": idx2})[u'min_idx']
        lca = self._get_hpo_by_idx_in_euler_tour(min_idx)
        return lca

    def all_pair_lca_in_tree(self):
        all_hpos = self.db[self.hpo_dag].distinct("hpo")
        all_hpos.sort()
        hpo1 = all_hpos[0]
        cur = self.db[self.lca_tree].find().sort([("hpo1", -1)])
        if cur.count() > 0:
            hpo1 = cur.next()[u'hpo1']
        for hpo1_ in all_hpos:
            if hpo1_ < hpo1:
                continue
            self._all_pair_lca_in_tree_for_hpo(hpo1_, self.db[self.rmq], self.db[self.lca_tree])

    def _all_pair_lca_in_tree_for_hpo(self, hpo1, rmq, lca_tree):
        idx1 = self._get_first_idx_in_euler_tour_by_hpo(hpo1)
        cur = rmq.find({"idx1": idx1})
        data = []
        for _d in cur:
            hpo2, lca = map(
                self._get_hpo_by_idx_in_euler_tour,
                map(
                    _d.get, 
                    ['idx2', 'min_idx']
                )
            )
            data.append((hpo2, lca))
        data = map(lambda t: {"hpo2": t[0], "lca": t[1]}, data)
        lca_tree.update({"hpo1": hpo1}, {"$set": {"data": data}}, upsert=True)

    def _all_pair_lca_in_tree(self, start_, N = 300000):
        total_cnt = self.db['temp_rmq'].find().count()
        for i in xrange(start_, total_cnt, N):  # 为防止cursor过期
            cur = self.db['temp_rmq'].find().skip(i).limit(N)
            for _d in cur:
                hpo1, hpo2, lca = map(
                    self._get_hpo_by_idx_in_euler_tour,
                    map(
                        lambda k: _d.get(k), 
                        ['idx1', 'idx2', 'min_idx']
                    )
                )
                logger.debug("pid {3} storing lca {0} for {1} {2}".format(lca, hpo1, hpo2, self.pid))
                self.db[self.lca_tree].update({"hpo1": hpo1, "hpo2": hpo2}, {
                    "$set": {
                        "lca": lca
                    }
                }, upsert=True)

    def _get_hpo_by_idx_in_euler_tour(self, idx):
        return self.euler_tour[idx]

    def _get_first_idx_in_euler_tour_by_hpo(self, hpo):
        return self.euler_idx_map[hpo]

    def _all_pair_RMQ(self, l_idx, level, **kw):
        '''
        :param l_idx: index_in_euler_tour的list
        :param level: euler_tour各元素对应的level
        '''
        start_range = kw.get('start_range', 1)

        l_idx.sort()
        for _range in chain([1], xrange(start_range, len(l_idx))):
            # 断点重启必须算_range=1, 存到缓存
            for i in xrange(len(l_idx)):
                try:
                    idx1, idx2 = l_idx[i], l_idx[i+_range]
                except IndexError:
                    break
                if _range == 1:
                    min_idx = self._calc_rmq_atomic(idx1, idx2, level)
                    self._store_rmq(idx1, idx2, min_idx, non_expire=True)
                    continue
                idx_spl = l_idx[i+_range-1]
                min_idx1 = self._get_rmq(idx1, idx_spl)
                min_idx2 = self._get_rmq(idx_spl, idx2)
                if level[min_idx1] < level[min_idx2]:
                    min_idx = min_idx1
                else:
                    min_idx = min_idx2
                self._store_rmq(idx1, idx2, min_idx)
                logger.debug("pid {} getting rmq idx1 {} idx2 {} min_idx {}".format(self.pid, idx1, idx2, min_idx))

    def _calc_rmq_atomic(self, idx1, idx2, level):
        '''
        :return: argmin(level[idx1, idx2])
        '''
        min_idx = None
        for idx in xrange(idx1, idx2+1):
            if min_idx is None or level[idx] < level[min_idx]:
                min_idx = idx
        return min_idx

    def _get_rmq(self, idx1, idx2):
        cache_key = self.rmq_cache_key.format(idx1, idx2)
        res = self.cache.get(cache_key)
        if res is not None:
            if self.cache.ttl(cache_key) is not None:  # if not persistent in the first place
                self.cache.expire(cache_key, self.rmq_cache_expire)  # 缓存续命
            return int(res)

        min_idx = self.db['temp_rmq'].find_one({"idx1": idx1, "idx2": idx2})[u'min_idx']

        self.cache.set(cache_key, min_idx)
        self.cache.expire(cache_key, self.rmq_cache_expire)
        return min_idx

    def _store_rmq(self, idx1, idx2, min_idx, non_expire=False):
        self.db['temp_rmq'].update({"idx1": idx1, "idx2": idx2}, {
            "$set": {
                "min_idx": min_idx
            }
        }, upsert=True)
        cache_key = self.rmq_cache_key.format(idx1, idx2)
        self.cache.set(cache_key, min_idx)
        if not non_expire:
            self.cache.expire(cache_key, self.rmq_cache_expire)


class DagAlg(TreeAlg):
    hpo_depth_map_cache_key = "hpo_depth_map"

    def __init__(self, db):
        TreeAlg.__init__(self, db)
        self.hpo_dag = 'hpo_tree'

    @property
    def hpo_depth_map(self):
        hpo_depth_map = self.cache.hgetall(self.hpo_depth_map_cache_key)
        if hpo_depth_map != {}:
            return hpo_depth_map
        cur = self.db[self.hpo_dag].find()
        dct = {d[u'hpo']: d[u'level'] for d in cur}
        self.cache.hmset(self.hpo_depth_map_cache_key, dct)
        return dct

    def _one_pair_lca_dag(self, hpo1, hpo2):
        # TODO：测试
        l1, l2 = map(
            lambda hpo: self.db[self.hpo_split_dag].find_one({"hpo": hpo})[u'ancestors'],
            [hpo1, hpo2]
        )
        for h,l in enumerate([l1,l2]):
            for d in l:
                d['owner'] = h
        l = sorted(l1+l2, key=lambda d: d['index'])

        candidate = []
        for i in xrange(len(l)-1):
            d, d_next = l[i], l[i+1]
            if not d['owner'] == d_next['owner']:
                candidate.append((d[u'hpo'], d_next[u'hpo']))

        lcas = map(
            lambda t: self._get_lca_in_tree(t[0],t[1]), 
            candidate
        )
        max_depth = -1
        for h in lcas:
            depth = self._get_depth_by_hpo(h)
            if depth > max_depth:
                max_depth = depth
                max_depth_lca = h
        return max_depth_lca

    def _get_depth_by_hpo(self, hpo):
        depth = self.cache.hget(self.hpo_depth_map_cache_key, hpo)
        if depth is not None:
            return depth
        return self.hpo_depth_map[hpo]

    def _get_lca_in_tree(self, hpo1, hpo2):
        return self.one_pair_lca_in_tree(hpo1, hpo2)
        # return (self.db[self.lca_tree].find_one({"hpo1": hpo1, "hpo2": hpo2})
        #         or self.db[self.lca_tree].find_one({"hpo1": hpo2, "hpo2": hpo1})
        #         or {}
        #         ).get(u'lca')


def alter_hpo_sim(db):
    i = -10000
    while True:
        i += 10000
        cur = db['hpo_sim'].find().skip(i).limit(10000)
        for d in cur:
            hpo1, hpo2 = map(d.get, ['hpo1', 'hpo2'])
            print hpo1, hpo2
            db['hpo_sim'].update({"_id": d['_id']}, {"$set": {"hpos": [hpo1, hpo2]}})


def alter_lca_tree(db):
    all_hpos = db['hpo_tree'].distinct("hpo")
    for hpo1 in all_hpos:
        doc = db['lca_tree'].find_one({"hpo1": hpo1})
        data = doc['data']
        for i in xrange(len(data)):
            d = data[i]
            d['hpo1'] = hpo1
            if d['hpo1'] > d['hpo2']:
                d['hpo1'], d['hpo2'] = d['hpo2'], d['hpo1']
            db['lca_tree_'].insert(d)



def store_lca_dag_proc_unit(db_args, iter_):
    db = MongoClient(*db_args[:-1])[db_args[-1]]
    dagAlg = DagAlg(db)
    for hpo1, hpo2 in iter_:
        lca = dagAlg._one_pair_lca_dag(hpo1, hpo2)
        db['lca_dag'].update({"hpos": [hpo1, hpo2]}, {"$set": {"lca": lca}}, upsert=True)
        msg = "pid {} storing lca dag for {} {}".format(multiprocessing.current_process().pid, 
                                                        hpo1, hpo2)
        print msg
        logger.debug(msg)


def store_lca_dag(dbargs):
    db = MongoClient(*dbargs[:-1])[dbargs[-1]]
    simple_hpos = db['hpo_split_dag'].find({"ancestors": {"$size": 1}}).distinct("hpo")
    all_hpos = db['hpo_split_dag'].distinct("hpo")
    complex_hpos = set(all_hpos) - set(simple_hpos)
    print len(complex_hpos)
    iter1 = combinations(simple_hpos, 2)
    def gen():
        for hpo1 in complex_hpos:
            for hpo2 in simple_hpos:
                if hpo1 > hpo2:
                    hpo1, hpo2 = hpo2, hpo1 
                yield hpo1, hpo2
    iter2 = gen()
    iter3 = combinations(complex_hpos, 2)
    prc1 = multiprocessing.Process(target=store_lca_dag_proc_unit, args=(dbargs, iter2))
    prc2 = multiprocessing.Process(target=store_lca_dag_proc_unit, args=(dbargs, iter3))
    prc1.start()
    prc2.start()
    prc1.join()
    prc2.join()



if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    treeAlg = TreeAlg(db)

    store_lca_dag(['localhost', 27017, 'phenomizer'])




        
