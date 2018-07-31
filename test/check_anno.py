# coding: utf8
from pymongo import MongoClient

####################
dag_lca = 'hpo_tree_lca'
anno = 'hpo_disease'
dag = 'hpo_tree'
######################


db = MongoClient('localhost', 27017)['phenomizer']

cur = db[anno].aggregate([
    {"$group": {"_id": "$hpo", "disease_cnt": {"$sum": 1}}}
], cursor={})


dis_cnt = []
for d in cur:
    hpo = d['_id']
    lvl = db[dag].find_one({"hpo": hpo})['level']
    if lvl <= 3:  # 前 k+1 层
        continue
    dis_cnt.append(d['disease_cnt'])


dis_cnt.sort()
print dis_cnt