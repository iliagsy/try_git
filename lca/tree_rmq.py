import math
from pymongo import MongoClient
import redis


###############
rmq = 'temp_rmq'
meta = 'meta'
euler_tour_key = "split_tree_euler_tour"
################


class RMQAlg(object):
    cache = redis.Redis()
    cache_ttl = 600
    cache_key = 'rmq'
    field_fmt = "idx1:{:0>4}:range:{:0>4}"

    def __init__(self):
        self.cache.flushall()

    @classmethod
    def prepare_2_pow_rmq(cls, rmq, values):
        _range = 1
        while _range < len(values):
            print _range
            for idx1 in xrange(0, len(values)):
                idx2 = idx1 + _range
                if idx2 >= len(values): break
                if _range == 1:
                    min_idx = idx1 if values[idx1] < values[idx2] else idx2
                else:
                    idx_mid = idx1 + _range / 2
                    min_idx1 = RMQAlg._get_rmq_idx(rmq, idx1, _range / 2)
                    min_idx2 = RMQAlg._get_rmq_idx(rmq, idx_mid, _range / 2)
                    min_idx = min_idx1 if values[min_idx1] < values[min_idx2] else min_idx2
                RMQAlg._store_rmq(rmq, idx1, _range, min_idx)
            _range *= 2

    @classmethod
    def _get_rmq_idx(cls, rmq, idx1, _range):
        res = cls.cache.hget(cls.cache_key, 
                            cls.field_fmt.format(idx1, _range)
                            )
        if res is not None:
            res = int(res)
        else:
            res = rmq.find_one({"idx1": idx1, "range": _range})[u'min_idx']
        return res

    @classmethod
    def _store_rmq(cls, rmq, idx1, _range, min_idx):
        idx2 = idx1 + _range
        cls.cache.hset(cls.cache_key, 
                        cls.field_fmt.format(idx1, _range),
                        min_idx)
        cls.cache.expire(cls.cache_key, cls.cache_ttl)
        rmq.update({"idx1": idx1, "range": _range}, {"$set": {"min_idx": min_idx}}, upsert=True)


def run(db, drop=False):
    if drop:
        db.drop_collection(rmq)
        db[rmq].create_index([("range",1), ("idx1",1)])
        db[rmq].create_index([("up_date", -1)])
    _l = db[meta].find_one({"key": euler_tour_key})['value']
    values = map(lambda d: d['level'], _l)
    RMQAlg().prepare_2_pow_rmq(db[rmq], values)


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    run(db, drop=True)