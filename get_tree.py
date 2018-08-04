# coding: utf-8
'''hpo-obo-file -> hpo_tree, hpo_anc'''
from collections import deque
import itertools
from pymongo import MongoClient
import random


#############
hpo_tree = 'hpo_tree'
hpo_desc = 'hpo_desc'
hpo_anc = 'hpo_anc'
##############


def store_hpo_tree(db, drop=False, filename='data/hp.obo'):
    if drop:
        db.drop_collection(hpo_tree)
        db[hpo_tree].create_index([("hpo": 1)], unique=True, background=True)
    def gen():
        with open(filename) as fh:
            block = {}
            first = True
            for line in fh.readlines():
                if line.strip() == '[Term]':
                    if first:
                        first = False
                        continue
                    yield block
                    block = {}
                else:
                    data = line.strip().split(': ')
                    if data[0] == 'id':
                        block.update({data[0]: data[1]})
                    if data[0] == 'alt_id':
                        block[data[0]] = block.get(data[0], []) + [data[1]]
                    if data[0] == 'is_a':
                        block[data[0]] = block.get(data[0], []) + [data[1].split(' ! ')[0]]
            yield block
    for d in gen():
        data = map(d.get, ['id', 'is_a', 'alt_id'])
        data[1] = data[1] or []
        data[2] = data[2] or []
        data_ = dict(zip(['hpo', 'parents', 'alt_id'], data))
        data_['children'] = []
        db[hpo_tree].update({"hpo": data_['hpo']}, data_, upsert=True)
    # set children
    cur = db[hpo_tree].find()
    for data_ in cur:
        for p_hpo in data_['parents']:
            db[hpo_tree].update({"hpo": p_hpo}, {
                "$addToSet": {
                    "children": data_['hpo']
                }
            })
    # 删掉别名
    hpos = db[hpo_tree].remove({"parents": {"$size": 0}, "children": {"$size": 0}})
    # 删掉non-pheno节点
    _clear_hpo_tree_subonto(db)


def _clear_hpo_tree_subonto(db):
    '''删掉除phenotypic abnormality外的subontology以及相关的边
    只保留phenotypic的后代
    hpo_tree, HP:0000118 -> hpo_tree'''
    root = 'HP:0000118'
    def get_all_desc(hpo):
        q = deque([hpo])
        desc = set([hpo])
        while len(q) > 0:
            h = q.popleft()
            children = db[hpo_tree].find_one({"hpo": h})['children']
            children = list(set(children) - desc)
            desc |= set(children)
            q.extend(children)
        return desc
    pheno_desc = get_all_desc(root)
    all_hpos = db[hpo_tree].distinct("hpo")
    other_subonto_hpo = set(all_hpos) - set(pheno_desc)
    db[hpo_tree].remove({"hpo": {"$in": list(other_subonto_hpo)}})
    cur = db[hpo_tree].find()
    for _d in cur:
        ps = list(set(_d[u'parents']) - other_subonto_hpo)
        cs = list(set(_d[u'children']) - other_subonto_hpo)
        db[hpo_tree].update({"hpo": _d[u'hpo']}, {
            "$set": {
                "parents": ps,
                "children": cs
            }
        })


def store_ancestors(db, drop=False):
    '''hpo_tree -> hpo_anc'''
    if drop:
        db.drop_collection(hpo_anc)
        db[hpo_anc].create_index([('hpo',1)], unique=True, background=True)
    all_hpos = db[hpo_tree].distinct("hpo")
    flag = {}
    for h in all_hpos:
        flag[h] = False

    leaves = db[hpo_tree].find({"parents": {"$size": 0}}).distinct("hpo")
    while True:
        leaves_ = set()
        for _d in db[hpo_tree].find({"hpo": {"$in": leaves}}):
            hpo = _d[u'hpo']
            parents = _d['parents']
            calculated_parents = filter(lambda h: flag[h], parents)
            if len(calculated_parents) < len(parents):
                continue

            descendents = {hpo: 0}
            for c in _d[u'parents']:
                desc_of_child = (db[hpo_anc].find_one({"hpo": c}) or {}).get(u'ancestors', [])
                desc_of_child = {d['hpo']:d['depth']+1 for d in desc_of_child}
                for H in desc_of_child:
                    D = desc_of_child[H]
                    if H not in descendents or D < descendents[H]:  # 取最短路径长度
                        descendents[H] = D

            descendents = [{"hpo": k, "depth": v} for k,v in descendents.items()]
            print 'storing {} ancestors for hpo {}'.format(len(descendents), hpo)
            db[hpo_anc].update({"hpo": hpo}, {
                "$set": {
                    "ancestors": descendents
                }
            }, upsert=True)
            flag[hpo] = True
            leaves_ |= set(_d['children'])
        if len(leaves_) == 0:
            break
        leaves = list(leaves_)


def add_depth_2_tree(db):
    '''hpo_tree -> hpo_tree
    每个节点的depth = 它离根最远的路径长度
    '''
    hpo_dag = hpo_tree
    root = db[hpo_dag].find_one({"parents": {"$size": 0}})['hpo']
    q = deque([(root, 0)])
    while len(q) > 0:
        hpo, depth = q.popleft()
        db[hpo_dag].update({"hpo": hpo}, {
            "$set": {
                "level": depth
            }
        }, upsert=True)
        children = db[hpo_dag].find_one({"hpo": hpo})[u'children']
        for c in children:
            q.append((c, depth + 1))  # 取离根最远的路径长度作为一个节点的深度



def setupHpoTree(db, drop=False):
    store_hpo_tree(db, drop=drop)
    add_depth_2_tree(db)

    store_ancestors(db, drop=drop)


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    
    