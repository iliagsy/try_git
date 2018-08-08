# coding: utf-8
'''hpo-obo-file -> hpo_tree, hpo_anc'''
from argparse import ArgumentParser, RawTextHelpFormatter
from bson import ObjectId
from collections import deque
import itertools
import json
import logging
import logging.config
from pymongo import MongoClient
import random
import time


from gdconfig import ConfigManager


logging_conf_d = json.load(open("logging_conf.json", 'rb'))
logging.config.dictConfig(logging_conf_d)
logger = logging.getLogger(__name__)


class ImportHpoTree(object):
    hpo_tree = 'hpo_tree'
    hpo_anc = 'hpo_anc'

    def __init__(self, db, filepath='data/hp.obo'):
        client = ConfigManager.ConfigManager().getdb().connection
        self.db = client[db]
        self.filepath = filepath
        self.op_id = "{}-{}".format(time.time(), ObjectId())

    def run(self):
        try:
            self.store_hpo_tree()
            self.add_depth_2_tree()
            self.store_ancestors()
        except Exception as e:
            logger.exception("storing ontology/ancestors failed, message\n {}".format(str(e)))
            logger.warning("removing all data inserted during this op")
            self.db[self.hpo_tree].remove({"op_id": self.op_id})
            self.db[self.hpo_anc].remove({"op_id": self.op_id})

    def store_hpo_tree(self):
        hpo_tree = self.hpo_tree
        hpo_anc = self.hpo_anc
        self.db[hpo_tree].create_index([("hpo", 1)], unique=True, background=True)
        for d in self._gen_data():
            self._store_one_node(d)
        self._set_children_according_parents()
        self._remove_alias()
        self._keep_phenotype_only()

    def store_ancestors(self):
        '''hpo_tree -> hpo_anc'''
        hpo_tree = self.hpo_tree
        hpo_anc = self.hpo_anc
        self.db[hpo_anc].create_index([('hpo',1)], unique=True, background=True)
        all_hpos = self.db[hpo_tree].distinct("hpo")
        flag = {}
        for h in all_hpos:
            flag[h] = False
        leaves = self.db[hpo_tree].find({"parents": {"$size": 0}}).distinct("hpo")
        while True:
            leaves_ = set()
            for _d in self.db[hpo_tree].find({"hpo": {"$in": leaves}}):
                hpo = _d[u'hpo']
                parents = _d['parents']
                if flag[hpo]:
                    continue
                calculated_parents = filter(lambda h: flag[h], parents)
                if len(calculated_parents) < len(parents):
                    continue
                ancestors = {hpo: 0}
                for c in _d[u'parents']:
                    desc_of_child = (self.db[hpo_anc].find_one({"hpo": c}) or {}).get(u'ancestors', [])
                    desc_of_child = {d['hpo']:d['depth']+1 for d in desc_of_child}
                    for H in desc_of_child:
                        D = desc_of_child[H]
                        if H not in ancestors or D < ancestors[H]:  # 取最短路径长度
                            ancestors[H] = D
                ancestors = [{"hpo": k, "depth": v} for k,v in ancestors.items()]
                data = {"hpo": hpo, "ancestors": ancestors, "op_id": self.op_id}
                self.db[hpo_anc].insert(data)
                flag[hpo] = True
                leaves_ |= set(_d['children'])
            if len(leaves_) == 0:
                break
            leaves = list(leaves_)

    def add_depth_2_tree(self):
        '''hpo_tree -> hpo_tree
        每个节点的depth = 它离根最远的路径长度
        '''
        hpo_dag = self.hpo_tree
        root = self.db[hpo_dag].find_one({"parents": {"$size": 0}})['hpo']
        q = deque([(root, 0)])
        while len(q) > 0:
            hpo, depth = q.popleft()
            self.db[hpo_dag].update({"hpo": hpo}, {
                "$set": {
                    "level": depth
                }
            }, upsert=True)
            children = self.db[hpo_dag].find_one({"hpo": hpo})[u'children']
            for c in children:
                q.append((c, depth + 1))  # 取离根最远的路径长度作为一个节点的深度

    def _gen_data(self):
        with open(self.filepath) as fh:
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
                    elif data[0] == 'alt_id':
                        block[data[0]] = block.get(data[0], []) + [data[1]]
                    elif data[0] == 'is_a':
                        block[data[0]] = block.get(data[0], []) + [data[1].split(' ! ')[0]]
            yield block

    def _store_one_node(self, d):
        data = map(d.get, ['id', 'is_a', 'alt_id'])
        data[1] = data[1] or []
        data[2] = data[2] or []
        data_ = dict(zip(['hpo', 'parents', 'alt_id'], data))
        data_['children'] = []
        data_["op_id"] = self.op_id
        self.db[self.hpo_tree].insert(data_)

    def _set_children_according_parents(self):
        hpo_tree = self.hpo_tree
        cur = self.db[hpo_tree].find()
        for data_ in cur:
            for p_hpo in data_['parents']:
                self.db[hpo_tree].update({"hpo": p_hpo}, {
                    "$addToSet": {
                        "children": data_['hpo']
                    }
                })

    def _remove_alias(self):
        '''删除别名节点（不与其他节点联系的）'''
        hpo_tree = self.hpo_tree
        self.db[hpo_tree].remove({"parents": {"$size": 0}, "children": {"$size": 0}}, multi=True)

    def _keep_phenotype_only(self):
        '''只保留表型 subontology (HP:0000118的后代)
        '''
        hpo_tree = self.hpo_tree
        hpo_anc = self.hpo_anc
        root = 'HP:0000118'
        def get_all_desc(hpo):
            '''得到后代'''
            q = deque([hpo])
            desc = set([hpo])
            while len(q) > 0:
                h = q.popleft()
                children = self.db[hpo_tree].find_one({"hpo": h})['children']
                children = list(set(children) - desc)
                desc |= set(children)
                q.extend(children)
            return desc
        pheno_desc = get_all_desc(root)
        all_hpos = self.db[hpo_tree].distinct("hpo")
        other_subonto_hpo = set(all_hpos) - set(pheno_desc)
        self.db[hpo_tree].remove({"hpo": {"$in": list(other_subonto_hpo)}})
        # 清理边
        cur = self.db[hpo_tree].find()
        for _d in cur:
            ps = list(set(_d[u'parents']) - other_subonto_hpo)
            cs = list(set(_d[u'children']) - other_subonto_hpo)
            self.db[hpo_tree].update({"hpo": _d[u'hpo']}, {
                "$set": {
                    "parents": ps,
                    "children": cs
                }
            })


def setupCommandLine():
    parser = ArgumentParser(
        description='''[GeneDock] Import HPO Tree from obo file.\n''',
        formatter_class=RawTextHelpFormatter
    )
    parser.add_argument("-d", "--db", required=True, help="database name to import data into")
    parser.add_argument("-f", "--filepath", default="data/hp.obo", help="hpo obo file downloaded from hpo official site")
    return parser


if __name__ == '__main__':
    parser = setupCommandLine()
    parsed = parser.parse_args()
    db_name = parsed.db
    filepath = parsed.filepath
    ImportHpoTree(db_name, filepath).run()
    
    