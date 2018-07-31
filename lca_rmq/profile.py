from pymongo import MongoClient
import random
import timeit


from .alg import DagAlg
from tool import Tool
from alg_simp import lca


db = MongoClient('localhost', 27017)['phenomizer']
dagAlg = DagAlg(db)
_ = dagAlg.euler_tour
_ = dagAlg.euler_idx_map
tool = Tool(db)
all_hpos = db['hpo_split_dag'].distinct("hpo")


def run():
    hpo1 = random.choice(all_hpos)
    hpo2 = random.choice(all_hpos)
    lca = dagAlg._one_pair_lca_dag(hpo1, hpo2)
    print lca


def run1():
    hpo1 = random.choice(all_hpos)
    hpo2 = random.choice(all_hpos)
    print lca(tool, hpo1, hpo2)


N = 100
rtime = timeit.timeit('run()', 'from __main__ import run', number=N)
# rtime1 = timeit.timeit('run1()', 'from __main__ import run1', number=N)

print rtime / N * 1000, 'ms'
# print rtime1 / N * 1000, 'ms'