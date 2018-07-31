# coding: utf-8
'''本脚本执行的前提：hpo_tree, hpo_anc'''
from collections import deque
import itertools
from pymongo import MongoClient
import random
import datetime


from util import setupLogger


#######
logger = setupLogger(__name__, "log/{}.log".format(__file__))
hpo_tree = 'hpo_tree'
hpo_disease = 'hpo_disease'
hpo_anc = 'hpo_anc'
hpo_desc = 'hpo_desc'
hpo_IC = 'hpo_IC'
##########

##################
freq_excl = 'HP:0040285'
####################


def store_anno(db, drop=False):
    '''hpo_tree'''
    _store_anno_raw(db, drop)
    _clear_anno_anc(db)
    _clear_anno_excluded(db)


def _store_anno_raw(db, drop=False, filename='data/phenotype.hpoa'):
    if drop:
        db.drop_collection(hpo_disease)
        db[hpo_disease].createIndex([("frequency",1)])
        db[hpo_disease].createIndex([("hpo",1)])
        db[hpo_disease].createIndex(["disease",1])
    def gen():
        keys = []
        with open(filename) as fh:
            for line in fh.readlines():
                if line.startswith("#"):
                    data = line.lstrip('#').split()
                    keys = map(lambda s: s.lower(), data)
                    continue
                data = line.strip().split('\t')
                data__ = dict(zip(keys, data))
                if data__['qualifier'].lower() == 'not':
                    continue
                data__['disease'] = '{}:{}'.format(data__['db'], data__['db_object_id'])
                data__['hpo'] = data__['hpo_id']
                data__.pop('db'); data__.pop('db_object_id'); data__.pop('hpo_id')
                yield data__
    all_hpos = db[hpo_tree].distinct("hpo")
    all_alts = db[hpo_tree].distinct("alt_id")
    all_hpos = set(all_hpos)
    all_alts = set(all_alts)
    for data in gen():
        if data['hpo'] in all_hpos:
            pass
        elif data['hpo'] in all_alts:
            doc = db[hpo_tree].find_one({"alt_id": data['hpo']})
            data['hpo'] = doc['hpo']
        else:
            continue
        db[hpo_disease].insert(data)


def _clear_anno_excluded(db):
    '''remove annotation where frequency==excluded'''
    db[hpo_disease].remove({"frequency": freq_excl}, multi=True)


def _clear_anno_anc(db):
    '''hpo_anc, hpo_disease -> hpo_disease'''
    all_hpos = db[hpo_disease].distinct("hpo")
    cur = db[hpo_disease].aggregate([
        {"$group": {"_id": "$hpo", "diseases": {"$push": "$disease"}}}
    ], cursor={})
    for d in cur:
        hpo = d['_id']
        print 'clearing {}'.format(hpo)
        diseases = list(set(d['diseases']))
        _l = db[hpo_anc].find_one({"hpo": hpo})['ancestors']
        anc = [d['hpo'] for d in _l if not d['hpo'] == hpo]
        db[hpo_disease].update({"hpo": {"$in": anc}, "disease": {"$in": diseases}}, {
            "$set": {
                "true_path": True
            }
        })



if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    store_anno(db, drop=True)