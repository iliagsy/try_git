from argparse import ArgumentParser
import datetime
import itertools
import math
from pymongo import MongoClient


#############################
split_dag = 'hpo_split_dag'
split_tree = 'hpo_split_tree'
rmq = 'temp_rmq'
dag = 'hpo_tree'
meta = 'meta'
ic_map = 'hpo_IC'
anno = 'hpo_disease'
hpo_sim = 'hpo_sim'
dag_lca = 'hpo_tree_lca'
euler_tour_key = "split_tree_euler_tour"
###################


def get_lca_tree(db, hpo1, hpo2, euler_tour):
    if hpo1 == hpo2: return hpo1
    cur = db[split_tree].find({"hpo": {"$in": [hpo1, hpo2]}})
    hpo_w_idx = map(lambda d: (d['hpo'], d['euler_idx']), cur)
    idx1, idx2 = map(lambda t: t[1], hpo_w_idx)
    if idx2 < idx1:
        idx1, idx2 = idx2, idx1

    _range = idx2 - idx1
    pow_ = int(math.log(_range, 2))
    encl_range = 2 ** pow_
    min_idx1 = _get_rmq_idx(db, idx1, encl_range)
    min_idx2 = _get_rmq_idx(db, idx2 - encl_range, encl_range)
    min_idx = min_idx1 if euler_tour[min_idx1]['level'] < euler_tour[min_idx2]['level'] else min_idx2
    return euler_tour[min_idx]['hpo']


def _get_rmq_idx(db, idx1, _range):
    return db[rmq].find_one({"range": _range, "idx1": idx1})[u'min_idx']


def get_lca(db, hpo1, hpo2, euler_tour):
    if hpo1 == hpo2: return hpo1
    cur = db[split_dag].find({"hpo": {"$in": [hpo1, hpo2]}})
    _l = []
    for d in cur:
        for _d in d['ancestors']:
            _l.append((d['hpo'], _d['hpo'], _d['level_whole_dag']))
    _l.sort(key=lambda t: t[2])

    max_level_lca = None
    max_level = -1
    grouped = itertools.groupby(_l, lambda t: t[2])
    gpd = map(lambda t: list(t[1]), grouped)
    for g in gpd:
        for t1,t2 in itertools.combinations(g,2):
            if t1[0] == t2[0]: 
                continue
            hpo1, hpo2 = t1[1], t2[1]
            lca = get_lca_tree(db, hpo1, hpo2, euler_tour)   ####
            lvl_whole_dag = db[dag].find_one({"hpo": lca})['level']
            if lvl_whole_dag > max_level:
                max_level = lvl_whole_dag
                max_level_lca = lca
    for i1 in xrange(len(gpd)-1):
        g1, g2 = gpd[i1], gpd[i1+1]
        for t1 in g1:
            for t2 in g2:
                if t1[0] == t2[0]:
                    continue
                hpo1, hpo2 = t1[1], t2[1]
                lca = get_lca_tree(db, hpo1, hpo2, euler_tour)   ####
                lvl_whole_dag = db[dag].find_one({"hpo": lca})['level']
                if lvl_whole_dag > max_level:
                    max_level = lvl_whole_dag
                    max_level_lca = lca
    return max_level_lca


def store_dag_lca(db, drop=False, start_hpo1=0, end_hpo1=14000):
    if drop:
        db.drop_collection(dag_lca)
        db[dag_lca].create_index([("type", 1)], unique=False)
        db[dag_lca].create_index([("hpo1", 1), ("range", 1)], unique=True)
    doc = db[dag_lca].find_one({"type": "meta", "key": "all_hpos_used"})
    if doc is None:
        all_hpos = db[split_tree].distinct("hpo")
        all_hpos.sort()
        db[dag_lca].update({"type": "meta", "key": "all_hpos_used"}, {"$set": {"value": all_hpos}}, upsert=True)
    else:
        all_hpos = doc['value']
    euler_tour = db[meta].find_one({"key": euler_tour_key})['value']
    def gen():
        for i in xrange(start_hpo1, end_hpo1):
            for j in xrange(i, len(all_hpos)):
                hpo1, hpo2 = all_hpos[i], all_hpos[j]
                yield hpo1, hpo2, i, j
    for hpo1, hpo2, hpo1id, hpo2id in gen():
        lca = get_lca(db, hpo1, hpo2, euler_tour)
        if lca == 'HP:0000118':
            continue
        lca_id = all_hpos.index(lca)
        # db[dag_lca].update({"type": "data", "hpo1": hpo1id, "range": hpo2id - hpo1id}, {
        #     "$set": {"lca": lca_id, "up_date": datetime.datetime.utcnow()}
        # }, upsert=True)
        db[dag_lca].insert({
            "type": "data", 
            "hpo1": hpo1id, 
            "range": hpo2id - hpo1id,
            "lca": lca_id, 
            "up_date": datetime.datetime.utcnow()
        })
        print hpo1, hpo2, lca


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    ap = ArgumentParser()
    ap.add_argument("-s", "--start", type=int, default=0)
    ap.add_argument("-e", '--end', type=int, default=14000)
    parsed = ap.parse_args()
    store_dag_lca(db, False, start_hpo1=parsed.start, end_hpo1=parsed.end)