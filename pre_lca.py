from argparse import ArgumentParser
import datetime

from pymongo import MongoClient


#############################
dag = 'hpo_tree'
dag_lca = 'hpo_tree_lca'
meta = 'meta'
euler_tour_key = "split_tree_euler_tour"
hpo_anc = 'hpo_anc'
###################



def get_lca(db, hpo1, hpo2, _):
    if hpo1 == hpo2:
        return hpo1
    anc1 = db[hpo_anc].find_one({"hpo": hpo1})['ancestors']
    anc2 = db[hpo_anc].find_one({"hpo": hpo2})['ancestors']
    anc1 = map(lambda d: d['hpo'], anc1)
    anc2 = map(lambda d: d['hpo'], anc2)
    common_anc = list(set(anc1) & set(anc2))
    lcas = get_border(db, common_anc)
    return lcas[0]


def get_border(db, hpos):
    cur = db[dag].find({"hpo": {"$in": hpos}})
    parents = set()
    for d in cur:
        parents |= set(d['parents'])
    return list(set(hpos) - parents)


def store_dag_lca(db, drop=False, start_hpo1=0, end_hpo1=14000):
    if drop:
        db.drop_collection(dag_lca)
        db[dag_lca].create_index([("type", 1)], unique=False)
        db[dag_lca].create_index([("hpo1", 1), ("range", 1)], unique=True)
    db[dag_lca].remove({"hpo1": start_hpo1}, multi=True)
    # get all hpos used in this collection
    doc = db[dag_lca].find_one({"type": "meta", "key": "all_hpos_used"})
    if doc is None:
        all_hpos = db[dag].distinct("hpo")
        all_hpos.sort()
        db[dag_lca].update({"type": "meta", "key": "all_hpos_used"}, {"$set": {"value": all_hpos}}, upsert=True)
    else:
        all_hpos = doc['value']
    # euler tour
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