from pymongo import MongoClient
import random


from .pre import PreDagAlg
from .alg import DagAlg


class CheckAlg(PreDagAlg):
    def __init__(self, db):
        super(CheckAlg, self).__init__(db)
        self.hpo_dag = 'hpo_tree'
        self.hpo_anc = 'hpo_anc'

    def get_all_lcas(self, hpo1, hpo2):
        '''lca by definition'''
        cas = self._get_all_ca_nodes(hpo1, hpo2)
        sub_dag = self._induce_dag_by_nodes(cas)
        lcas = []
        for d in sub_dag:
            if len(d[u'children']) == 0:
                lcas.append(d[u'hpo'])
        return lcas

    def _get_all_ca_nodes(self, hpo1, hpo2):
        anc1 = self.db[self.hpo_anc].find_one({'hpo': hpo1})[u'ancestors']
        anc2 = self.db[self.hpo_anc].find_one({'hpo': hpo2})[u'ancestors']
        anc1 = map(lambda d: d['hpo'], anc1)
        anc2 = map(lambda d: d['hpo'], anc2)
        return list(set(anc1) & set(anc2))

    def _induce_dag_by_nodes(self, nodes):
        cur = self.db[self.hpo_dag].find({"hpo": {"$in": nodes}})
        docs = []
        for doc in cur:
            children = doc[u'children']
            children = list(set(children) & set(nodes))
            doc[u'children'] = children
            docs.append(doc)
        return docs


def random_check(db):
    checkAlg = CheckAlg(db)
    all_hpos = db['hpo_tree'].distinct("hpo")
    for i in xrange(10):
        hpo1 = random.choice(all_hpos)
        hpo2 = random.choice(all_hpos)
        print hpo1, hpo2, '----'
        print checkAlg.get_all_lcas(hpo1, hpo2)


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    checkAlg = CheckAlg(db)

    all_hpos = db['hpo_tree'].distinct("hpo")
    for i in xrange(10000):
        hpo1 = random.choice(all_hpos)
        hpo2 = random.choice(all_hpos)
        cor = checkAlg.get_all_lcas(hpo1, hpo2)
        print cor