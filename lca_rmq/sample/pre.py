from collections import deque
import json
from pymongo import MongoClient


from ..pre import PreAlg, PreDagAlg


class SamplePreAlg(PreAlg):
    def __init__(self, db):
        super(self.__class__, self).__init__(db)
        self.hpo_dag = 'hpo_tree'
        self.root = '0'

    def artifact_tree(self):
        _l = json.load(open('data/temp_sample_tree.json'))
        for d in _l:
            d[u'hpo'] = '{}'.format(d[u'hpo'])
            children = []
            for c in d[u'children']:
                children.append('{}'.format(c))
            d[u'children'] = children
        self.db[self.hpo_dag].insert(_l)


class PartialPreAlg(PreAlg):
    def __init__(self, db):
        super(PartialPreAlg, self).__init__(db)
        self.whole_hpo_dag = 'hpo_tree_'
        self.hpo_dag = 'hpo_tree'
        self.roots = [('HP:0000119', 2, 5), 
                      ('HP:0000118', 3, 4),
                      ("HP:0000118", 4, 5),
                      ("HP:0000118", 7, 10)]
        self.root, self.level, self.max_children = self.roots[-1]

    def partial_hpo_tree(self, drop=False):
        if drop:
            self.db.drop_collection(self.hpo_dag)
        nodes = self._get_levels(self.db[self.whole_hpo_dag], self.root, self.level, self.max_children)
        self._copy_partial_tree(self.db[self.whole_hpo_dag], self.db[self.hpo_dag], nodes)

    def _copy_partial_tree(self, old, new, nodes):
        cur = old.find({"hpo": {"$in": nodes}})
        partial_tree = {}
        for d in cur:
            dn = d.copy()
            dn.pop("_id")
            children = list(set(d[u'children']) & set(nodes))
            parents = list(set(d['parents']) & set(nodes))
            dn['children'] = children
            dn['parents'] = parents
            new.update({"hpo": d[u'hpo']}, {
                "$set": dn
            }, upsert=True)
            partial_tree[d[u'hpo']] = children
        return partial_tree

    def _get_levels(self, tree, root, level=3, max_children=5):
        q = deque([(root, 0)])
        all_hpos = [root]
        while len(q) > 0:
            hpo, depth = q.popleft()
            children = tree.find_one({"hpo": hpo})[u'children']
            children = children[:max_children]
            all_hpos += children
            if depth < level:
                for c in children:
                    q.append((c, depth+1))
        return list(set(all_hpos))


class SamplePreDagAlg(PreDagAlg):
    def __init__(self, db):
        super(SamplePreDagAlg, self).__init__(db)
        self.hpo_dag = 'hpo_tree'
        self.root = '0'


class PartialPreDagAlg(PreDagAlg):
    def __init__(self, db):
        super(PartialPreDagAlg, self).__init__(db)
        self.hpo_dag = 'hpo_tree'
        self.root = 'HP:0000118'


def main():
    db = MongoClient('localhost', 27017)['phenomizer_partial']
    for c in db.collection_names():
        if c == 'hpo_tree_':
            continue
        db.drop_collection(c)

    sPreAlg = PartialPreAlg(db)

    # sPreAlg.partial_hpo_tree()
    # sPreAlg.depth_of_whole_dag()

    # sPreAlg.split_dag()
    # sPreAlg.euler_tour()
    # sPreAlg.level_of_euler_tour()
    # sPreAlg.index_in_euler_of_tree()

    # sPreDagAlg = PartialPreDagAlg(db)

    # sPreDagAlg.prepare_dag_ancestors()
    # sPreDagAlg.prepare_whole_dag_ancestors()



if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer_partial']

    sPreAlg = PartialPreAlg(db)
    sPreAlg.partial_hpo_tree(drop=True)
