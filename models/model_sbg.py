# coding: utf-8
import json
from pymongo import MongoClient


class BaseManager(object):
    collection_name = ''

    def __init__(self, db):
        self.collection = db[self.collection_name]

    def find(self, *args, **kwargs):
        return self.collection.find(*args, **kwargs)

    def find_one(self, *args, **kwargs):
        return self.collection.find_one(*args, **kwargs)

    def distinct(self, *args, **kwargs):
        return self.collection.distinct(*args, **kwargs)


class GeneGraphManager(BaseManager):
    collection_name = "disease_subgraph"

    def __init__(self, db):
        super(GeneGraphManager, self).__init__(db)

    def get_gene_cond_on_hpo(self, **kwargs):
        should_lst = kwargs.get('should', [])
        result = []
        shouldSet = set(should_lst)
        for d in self.find({'subgraph': {'$in': should_lst}}):
            gene = d[u'disease']
            subgraph = d[u'subgraph']
            subgraphSet = set(subgraph)
            match = subgraphSet & shouldSet
            matchScore = float(len(match)) / float(len(subgraphSet))
            # if matchScore < 0.5:
            #     continue
            geneResult = {'id': gene, 'score': matchScore, "size": len(subgraphSet)}
            result.append(geneResult)
        result.sort(key=lambda _d: _d['score'], reverse=True)
        return result


class HPGraphManager(BaseManager):
    collection_name = "hpo_anc"

    def __init__(self, db):
        super(HPGraphManager, self).__init__(db)

    def get_full_hpo(self, **kwargs):
        should_lst = kwargs.get('should', [])
        result = self.find({"hpo": {"$in": should_lst}}).distinct("ancestors.hpo")
        return list(result), ""
        

def get_diseases(db, query):
    hp_manager = HPGraphManager(db)
    gegh_manager = GeneGraphManager(db)
    kw = {"should": query}
    hp_lst, msg = hp_manager.get_full_hpo(**kw)
    hpkw = {"should": hp_lst}
    ret_lst = gegh_manager.get_gene_cond_on_hpo(**hpkw)
    return map(lambda d: (d['id'], d['score']), ret_lst)



def eval():
    db = MongoClient('localhost', 27017)['phenomizer']
    all_dis = db['hpo_disease'].distinct("disease")
    cur = db['benchmark_patients'].find({"disease": {'$in': all_dis}})
    result = []
    for d in cur:
        pid = d['pid']
        qSet = d['hpos']
        cor_dis = d['disease']
        if 'MIM' not in cor_dis: continue
        res = get_diseases(db, qSet)
        res = map(lambda t:t[0], res)
        try:
            rank = res.index(cor_dis) + 1
        except ValueError:
            rank = None
        result.append(rank)
    result.sort()
    js_ = json.dumps(result, ensure_ascii=False)
    with open('eval/bmRes-subgraph.json', 'w') as fh:
        fh.write(js_)


if __name__ == '__main__':
    eval()

