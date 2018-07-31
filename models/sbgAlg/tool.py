# coding: utf-8
import math
import timeit
from decimal import Decimal
from collections import deque
from pymongo import MongoClient
import random
import itertools
import json
import re


class Tool(object):

    def __init__(self, db):
        self.db = db
        self.n_all_gene = len(self.db['gene_hpo'].distinct("gene"))
        self._hpo_IC_map = None

    @property
    def hpo_IC_map(self):
        if self._hpo_IC_map is not None:
            return self._hpo_IC_map
        cur = self.db['hpo_IC'].find()
        data = {d[u'hpo']: d[u'IC'] for d in cur}
        self._hpo_IC_map = data
        return self._hpo_IC_map

    def MICA_w_subgraph(self, term1, term2):
        anc1, anc2 = map(
            lambda t: {d['hpo']: d['depth'] for d in self.getAllAncestorsWDepth(t)},
            [term1, term2]
        )
        common_anc = list(set(anc1.keys()) & set(anc2.keys()))
        assert len(common_anc) > 0
        inf = map(lambda h: anc1[h] + anc2[h], common_anc)
        min_i = inf.index(min(inf))
        mica = common_anc[min_i]
        desc_of_mica = self.getAllDescendents(mica)
        subgraph = list(set(desc_of_mica) & (set(anc1.keys()) | set(anc2.keys())))
        return mica, subgraph

    def MICA(self, term1, term2, **kw):
        '''a.k.a LCA, lowest common ancestor'''
        db = self.db
        # 暴力
        anc1, anc2 = map(
            lambda t: {d['hpo']: d['depth'] for d in self.getAllAncestorsWDepth(t)},
            [term1, term2]
        )
        common_anc = list(set(anc1.keys()) & set(anc2.keys()))
        assert len(common_anc) > 0
        inf = map(lambda h: anc1[h] + anc2[h], common_anc)
        min_i = inf.index(min(inf))
        mica = common_anc[min_i]
        return mica

    def getAllAncestorsWDepth(self, term, **kw):
        ancs = (self.db['hpo_anc'].find_one({"hpo": term}) or {}).get(u'ancestors', [])
        return ancs

    def getAllDescendents(self, term):
        desc = (self.db['hpo_desc'].find_one({"hpo": term}) or {}).get(u'descendents', [])
        return desc

    def IC_pre(self, term):
        db = self.db
        genes = (db['hpo_gene_all'].find_one({"hpo": term}) or {}).get(u'genes', [])
        freq_en = len(genes) + 1
        freq_de = self.n_all_gene + 1
        freq_en, freq_de = map(Decimal, [freq_en, freq_de])
        IC = -math.log(freq_en / freq_de)
        return IC

    def IC(self, term):
        IC = self.hpo_IC_map.get(term) or self.db['hpo_IC'].find_one({"hpo": term})[u'IC']
        return IC


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    tool = Tool(db)
