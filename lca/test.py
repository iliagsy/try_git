from pymongo import MongoClient


from lca_rmq.check import CheckAlg
from .alg import get_lca, get_lca_tree

#############
meta = 'meta'
dag = 'hpo_tree'
anc = 'hpo_anc'
euler_tour_key = "split_tree_euler_tour"
##############


def compare(db):
    import random
    alg = CheckAlg(db)
    euler_tour = db[meta].find_one({"key": euler_tour_key})['value']
    all_hpos = db[dag].distinct("hpo")
    err_dist = []
    for i in xrange(10000):
        hpo1 = random.choice(all_hpos)
        hpo2 = random.choice(all_hpos)
        my_lca = get_lca(db, hpo1, hpo2, euler_tour)
        corr_lcas = alg.get_all_lcas(hpo1, hpo2)
        if my_lca in corr_lcas:
            continue
        else:
            cur = db[anc].find({"hpo": {'$in': corr_lcas}, "ancestors.hpo": my_lca})
            if cur.count() > 0:
                Min = None
                for doc in cur:
                    for d in doc['ancestors']:
                        if d['hpo'] == my_lca:
                            if Min is None or d['depth'] < Min:
                                Min = d['depth']
                err_dist.append(Min)
                # print Min
                if Min >= 3:
                    print "{} {} my {} corr {}".format(hpo1, hpo2, my_lca, corr_lcas)
            else:
                print "{} {} corr {} my {}".format(hpo1, hpo2, corr_lcas, my_lca)
                raise Exception("corr and my results have no relation")
    print (sum(err_dist, 0.0) / len(err_dist), err_dist[len(err_dist) / 2], 
           float(err_dist.count(1)) / 10000.0, float(len(err_dist)) / 10000.0
           )


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    compare(db)


'''
我的算法返回结果是正确结果的祖先，平均距离1.34, 距离中位数2
距离为1: 1.1%
距离>1: 0.4%
'''