# coding: utf-8
from pymongo import MongoClient
import random


from ..alg import TreeAlg, DagAlg
from ..check import *


class SampleTreeAlg(TreeAlg):
    rmq_cache_key = 'sample:' + "rmq:idx1:{:>04}:idx2:{:>04}"
    rmq_cache_expire = 720
    euler_tour_cache_key = "sample:" + 'euler_tour'
    euler_idx_map_cache_key = "sample:" + 'euler_idx_map'

    def __init__(self, db):
        super(self.__class__, self).__init__(db)


class SampleDagAlg(DagAlg):
    rmq_cache_key = 'sample:' + "rmq:idx1:{:>04}:idx2:{:>04}"
    rmq_cache_expire = 720
    euler_tour_cache_key = "sample:" + 'euler_tour'
    euler_idx_map_cache_key = "sample:" + 'euler_idx_map'
    hpo_depth_map_cache_key = 'sample:' + "hpo_depth_map"

    def __init__(self, db):
        super(self.__class__, self).__init__(db)
        self.hpo_dag = 'hpo_tree'


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer_partial']
    sTreeDag = SampleDagAlg(db)
    # db.drop_collection('temp_rmq')
    # db.drop_collection("lca_tree")

    # sTreeDag.all_pair_RMQ()
    # sTreeDag.all_pair_lca_in_tree()

    sDagAlg = SampleDagAlg(db)
    checkAlg = CheckAlg(db)
    all_hpos = db['hpo_tree'].distinct("hpo")
    for i in xrange(100):
        hpo1 = random.choice(all_hpos)
        hpo2 = random.choice(all_hpos)
        cor = checkAlg.get_all_lcas(hpo1, hpo2)
        res = sDagAlg._one_pair_lca_dag(hpo1, hpo2)
        try:
            assert res in cor
        except:
            print hpo1, hpo2, res, cor

