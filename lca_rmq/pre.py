# coding: utf-8
from collections import deque
from pymongo import MongoClient


class PreAlg(object):
    def __init__(self, db):
        self.db = db
        self.root = "HP:0000118"
        self.hpo_split_tree = 'hpo_split_tree'
        self.hpo_split_dag = 'hpo_split_dag'
        self.hpo_dag = 'hpo_tree'

    def index_in_euler_of_tree(self):
        '''每个节点在欧拉tour中第一次出现的index'''
        euler_tour = self.db['meta'].find_one({"key": "euler_tour"})[u'value']
        visited = set()
        for idx, hpo in enumerate(euler_tour):
            if hpo in visited:
                continue
            self.db['meta'].update({"key": "index_in_euler_tour", "hpo": hpo}, {
                "$set": {
                    "index": idx
                }
            }, upsert=True)
            visited.add(hpo)

    def level_of_euler_tour(self):
        self._store_level_in_tree()
        euler_tour = self.db['meta'].find_one({"key": "euler_tour"})[u'value']
        levels = []
        for idx, hpo in enumerate(euler_tour):
            depth = self.db[self.hpo_split_tree].find_one({"hpo": hpo})[u'level']
            levels.append(depth)
        self.db['meta'].update({"key": "level_of_euler_tour"}, {
            "$set": {
                "value": levels
            }
        }, upsert=True)

    def depth_of_whole_dag(self):
        q = deque([(self.root, 0)])
        while len(q) > 0:
            hpo, depth = q.popleft()
            self.db[self.hpo_dag].update({"hpo": hpo}, {
                "$set": {
                    "level": depth
                }
            }, upsert=True)
            children = self.db[self.hpo_dag].find_one({"hpo": hpo})[u'children']
            for c in children:
                q.append((c, depth + 1))  # 取离根最远的路径长度作为一个节点的深度

    def _store_level_in_tree(self):
        '''存在分解得到的tree里'''
        q = deque([(self.root, 0)])
        while len(q) > 0:
            hpo, depth = q.popleft()
            children = self.db[self.hpo_split_tree].find_one({'hpo': hpo})[u'children']
            for c in children:
                q.append((c, depth+1))
            self.db[self.hpo_split_tree].update({"hpo": hpo}, {"$set": {"level": depth}})

    def euler_tour(self):
        euler_tour = self._euler_tour_r(self.root)
        self.db['meta'].update({"key": "euler_tour"}, {
            "$set": {
                "value": euler_tour
            }
        }, upsert=True)

    def split_dag(self):
        '''split edges into a spanning tree and a dag,
        in the form of `hpo_split_tree`, `hpo_split_dag` collection in mongodb
        '''
        self._split_dag()

    def _euler_tour_r(self, node):
        children = self.db[self.hpo_split_tree].find_one({"hpo": node})[u'children']
        euler_tour = [node]
        for c in children:
            c_euler_tour = self._euler_tour_r(c)
            euler_tour += c_euler_tour
            euler_tour += [node]
        return euler_tour

    def _split_dag(self):
        self.depth_of_whole_dag()
        self.descendents_without_index()
        all_hpos = self.db[self.hpo_dag].distinct("hpo")
        for hpo in all_hpos:
            self.db[self.hpo_split_dag].update({"hpo": hpo}, {"$set": {"children": []}}, upsert=True)
        for hpo in all_hpos:
            parents_cur = self.db[self.hpo_dag].find({"children": hpo})
            parents = map(lambda d: (d[u'hpo'], d.get(u'depth')), parents_cur)
            if len(parents) == 0:
                continue
            parents.sort(key=lambda t:t[1])

            best_depth = parents[-1][1]
            best_parents = map(lambda t: t[0], filter(lambda t: t[1] == best_depth, parents))
            best_parents = list(set(best_parents))
            best_parent = best_parents[0]

            desc = self.db[self.hpo_dag].find_one({"hpo": hpo})[u'descendents']
            for h in desc:
                # for p in best_parents[1:]:
                for p in set(map(lambda t: t[0], parents)) - set([best_parent]):
                    self.db[self.hpo_split_dag].update({"hpo": p}, {"$addToSet": {"children": h}}, upsert=True)

            self.db[self.hpo_split_tree].update({"hpo": best_parent}, {"$addToSet": {"children": hpo}}, upsert=True)
            for p in set(map(lambda t: t[0], parents)) - set([best_parent]):
                self.db[self.hpo_split_dag].update({"hpo": p}, {"$addToSet": {"children": hpo}}, upsert=True)
        for hpo in all_hpos:
            if self.db[self.hpo_split_tree].find_one({"hpo": hpo}) is None:
                self.db[self.hpo_split_tree].update({"hpo": hpo}, {"$set": {"children": []}}, upsert=True)

    def descendents_without_index(self):
        self._dag_store_descendents_without_index(self.db[self.hpo_dag])

    def _dag_store_descendents_without_index(self, dag):
        all_hpos = dag.distinct("hpo")
        flag = {}
        for h in all_hpos:
            flag[h] = False

        leaves = dag.find({"$or": [{"children": {"$size": 0}},
                                   {"children": {"$exists": False}}]}
                          ).distinct("hpo")
        while len(leaves) > 0:
            leaves_ = set()
            cur = dag.find({"hpo": {"$in": leaves}})
            for _d in cur:
                hpo, children = _d[u'hpo'], _d.get(u'children', [])
                calculated_children = filter(lambda p: flag[p], children)
                if len(calculated_children) < len(children):
                    continue
                descs = set([hpo])
                for c in children:
                    desc_of_c = dag.find_one({"hpo": c}).get(u'descendents', [])
                    descs |= set(desc_of_c)
                descs = list(descs)
                dag.update({"_id": _d[u'_id']}, {
                    "$set": {
                        "descendents": descs
                    }    
                })
                flag[hpo] = True
                parents = dag.find({"children": hpo}).distinct("hpo")
                leaves_ |= set(parents)
            leaves = list(leaves_)


class PreDagAlg(PreAlg):
    def __init__(self, db):
        PreAlg.__init__(self, db)

    def prepare_dag_ancestors(self):
        self._dag_add_parents(self.db[self.hpo_split_dag])
        self._dag_store_ancestors(self.db[self.hpo_split_dag])

    def prepare_whole_dag_ancestors(self):
        self._dag_add_parents(self.db[self.hpo_dag])
        self._dag_store_ancestors(self.db[self.hpo_dag])

    def _dag_store_ancestors(self, dag):
        all_hpos = dag.distinct("hpo")
        flag = {}
        for h in all_hpos:
            flag[h] = False

        leaves = dag.find({"$or": [{"parents": {"$size": 0}},
                                   {"parents": {"$exists": False}}]}
                          ).distinct("hpo")
        while len(leaves) > 0:
            leaves_ = set()
            cur = dag.find({"hpo": {"$in": leaves}})
            for _d in cur:
                hpo, parents = _d[u'hpo'], _d.get(u'parents', [])
                calculated_parents = filter(lambda p: flag[p], parents)
                if len(calculated_parents) < len(parents):
                    continue
                ancs = set([hpo])
                for p in parents:
                    anc_of_p = dag.find_one({"hpo": p}).get(u'ancestors', [])
                    anc_of_p = map(lambda d: d[u'hpo'], anc_of_p)
                    ancs |= set(anc_of_p)
                ancs_ = self._sort_by_index_in_euler_tour(ancs)
                dag.update({"_id": _d[u'_id']}, {
                    "$set": {
                        "ancestors": ancs_
                    }    
                })
                flag[hpo] = True
                leaves_ |= set(_d.get(u'children', []))
            leaves = list(leaves_)

    def _sort_by_index_in_euler_tour(self, set_of_hpos):
        '''
        :param hpos: set
        :return: 根据split_tree里欧拉遍历的顺序排序的hpo
        '''
        cur = self.db['meta'].find({"key": "index_in_euler_tour", "hpo": {"$in": list(set_of_hpos)}})
        _l = list(cur)
        _l.sort(key=lambda _d: _d[u'index'])
        for d in _l:
            d.pop('_id')
            d.pop('key')
        return _l

    def _dag_add_parents(self, dag):
        all_hpos = dag.distinct("hpo")
        for hpo in all_hpos:
            dag.update({"hpo": hpo}, {
                "$set": {
                    "parents": []
                }
            }, upsert=True)
        cur = dag.find()
        for _d in cur:
            for c in _d[u'children']:
                dag.update({"hpo": c}, {
                    "$addToSet": {
                        "parents": _d[u'hpo']
                    }
                })


def alter_split_dag(dag, split_tree, split_dag):
    cur = dag.find()
    children_map = {}
    all_hpos = dag.distinct("hpo")
    for hpo in all_hpos:
        split_dag.update({"hpo": hpo}, {"$set": {"children": []}}, upsert=True)
    for d in cur:
        hpo = d[u'hpo']
        children = d[u'children']
        split_tree_children = split_tree.find_one({"hpo": hpo})[u'children']
        for c in set(children) - set(split_tree_children):
            desc_of_c = dag.find_one({"hpo": c})[u'descendents']
            for d_of_c in desc_of_c:
                split_dag.update({"hpo": hpo}, {
                    "$addToSet": {
                        "children": d_of_c
                    }
                }, upsert=True)
            print 'edge {} - {}'.format(hpo, c)    


def add_anc_to_whole_dag(dag, dag_anc):
    cur = dag_anc.find()
    for d in cur:
        hpo = d[u'hpo']
        anc = d[u'ancestors']
        dag.update({"hpo": hpo}, {"$set": {"ancestors": anc}})


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']

    preDagAlg = PreDagAlg(db)

    preAlg = PreAlg(db)


