import datetime
from pymongo import MongoClient


##################
split_dag = 'hpo_split_dag'
split_tree = 'hpo_split_tree'
dag_anc = 'hpo_anc'
dag = 'hpo_tree'
##################


def store_ancestors(db):
    all_hpos = db[split_dag].distinct("hpo")
    flag = {}
    for h in all_hpos:
        flag[h] = False

    leaves = db[split_dag].find({"$or": [{"parents": {"$size": 0}},
                                {"parents": {"$exists": False}}]}
                                ).distinct("hpo")
    while len(leaves) > 0:
        leaves_ = set()
        cur = db[split_dag].find({"hpo": {"$in": leaves}})
        for _d in cur:
            hpo, parents = _d[u'hpo'], _d.get(u'parents', [])
            calculated_parents = filter(lambda p: flag[p], parents)
            if len(calculated_parents) < len(parents):
                continue
            ancs = set([hpo])
            for p in parents:
                anc_of_p = db[split_dag].find_one({"hpo": p}).get(u'ancestors', [])
                anc_of_p = map(lambda d: d[u'hpo'], anc_of_p)
                ancs |= set(anc_of_p)
            # add depth in whole dag
            ancs_ = []
            for h in ancs:
                lvl = db[dag].find_one({"hpo": h})['level']
                ancs_.append({"hpo": h, "level_whole_dag": lvl})
            db[split_dag].update({"_id": _d[u'_id']}, {
                "$set": {
                    "ancestors": ancs_
                }    
            })
            flag[hpo] = True
            leaves_ |= set(_d.get(u'children', []))
        leaves = list(leaves_)


def alter_split_dag(db):
    cur = db[split_dag].find({"children": {"$ne": []}})
    for d in cur:
        d_dag = db[dag].find_one({"hpo": d['hpo']})
        lvl = d_dag['level']
        for c in d['children']:
            dag_desc = db[dag_anc].find({"ancestors.hpo": c}).distinct("hpo")  # 加快：hpo_anc的ancestors.hpo建索引
            for d_of_c in dag_desc:
                db[split_dag].update({"hpo": d_of_c}, {
                    "$addToSet": {
                        "ancestors": {"hpo": d['hpo'], "level_whole_dag": lvl}
                    },
                    "$set": {"up_date": datetime.datetime.utcnow()}
                }, upsert=True)


def prep(db, drop=False):
    if drop:
        db[split_dag].update({}, {"$unset": {"ancestors": ""}}, multi=True)
    store_ancestors(db)
    alter_split_dag(db)


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    prep(db, drop=True)
