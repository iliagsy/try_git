from pymongo import MongoClient


####################
dag = 'hpo_tree'
split_tree = 'hpo_split_tree'
split_dag = 'hpo_split_dag'
dag_anc = 'hpo_anc'
##################


def split(db, drop=False):
    '''split dag into span tree and sparse dag
    '''
    if drop:
        db.drop_collection(split_tree)
        db.drop_collection(split_dag)
        db[split_tree].create_index([("hpo", 1)], unique=True)
        db[split_dag].create_index([("hpo", 1)], unique=True)
    cur = db[dag].find()
    for d in cur:
        if len(d['parents']) <= 1:
            db[split_tree].update({"hpo": d['hpo']}, {
                "$set": {
                    "parents": d['parents']
                }
            }, upsert=True)
            db[split_dag].update({"hpo": d['hpo']}, {
                "$set": {
                    "parents": []
                }
            }, upsert=True)
            continue
        cur_ = db[dag].find({"hpo": {"$in": d['parents']}})
        _l = list(cur_)
        _l.sort(key=lambda d: d['level'], reverse=True)
        parent = _l[0]['hpo']
        db[split_tree].update({"hpo": d['hpo']}, {
            "$set": {
                "parents": [parent]
            }
        }, upsert=True)
        dag_parents = map(lambda d: d['hpo'], _l[1:])
        db[split_dag].update({"hpo": d['hpo']}, {
            "$set": {
                "parents": dag_parents
            }    
        }, upsert=True)
    # set `children` from `parents` field
    for cn in [split_tree, split_dag]:
        cur = db[cn].find()
        for d in cur:
            for h in d['parents']:
                db[cn].update({"hpo": h}, {"$addToSet": {"children": d['hpo']}}, upsert=True)
        db[cn].update({'children': {"$exists": False}}, {"$set": {"children": []}}, multi=True)


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    split(db, drop=True)