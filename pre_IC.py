import math
from pymongo import MongoClient


#################
anno = 'hpo_disease'
dag = 'hpo_tree'
hpo_IC = 'hpo_IC'
anc = 'hpo_anc'
##############


def store_IC(db, drop=False):
    if drop:
        db.drop_collection(hpo_IC)
        db[hpo_IC].create_index([("hpo", 1)])
    all_hpos = db[dag].distinct("hpo")
    n_obj = len(db[anno].distinct("disease"))
    for h in all_hpos:
        IC = get_IC(db, h, n_obj)
        db[hpo_IC].update({"hpo": h}, {"$set": {"IC": IC}}, upsert=True)


def get_IC(db, hpo, n_obj):
    desc = db[anc].find({"ancestors.hpo": hpo}).distinct("hpo")
    n_anno_obj = len(db[anno].find({"hpo": {"$in": desc}}).distinct("disease"))
    IC = -math.log((n_anno_obj+1) / float(n_obj+1))
    return IC


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    store_IC(db, True)