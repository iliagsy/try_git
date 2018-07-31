import json
import logging
import os
from pymongo import MongoClient
import random
import re


from ..new.model import get_diseases


logging.basicConfig(level=logging.DEBUG, filename='log/models.tests.eval_on_bm.log', 
                    filemode='w', format='%(message)s')


##############
bm_db = MongoClient('localhost', 27017)['phenomizer']
benchmark = 'benchmark'
patients = 'benchmark_patients'
hpo_tree = 'hpo_tree'
phenomizer_result = 'benchmark_result'
##############


def eval(res_db):
    db = res_db
    all_hpos = set(db[hpo_tree].distinct('hpo'))
    cur = db[phenomizer_result].find().sort([("pid",1), ("disease",1)])
    for d in cur:
        re_res = re.findall(r'^(\w+):(\d+)$', d['disease'])
        if len(re_res) == 0:
            continue
        phn_res = d['result']
        phn_diss = map(lambda d__: d__['disease'].replace("ORPHANET", "ORPHA"), phn_res)

        d_ = bm_db[patients].find_one({"disease": d['disease'], 'pid': d['pid']})
        query = d_['hpos']

        query = list(set(query) & all_hpos)
        if not len(query) >= 3:
            continue
        result = get_diseases(db, query)
        res_diss = map(lambda t: t[0], result)

        try:
            rank_my = res_diss.index(d['disease']) + 1
        except ValueError:
            rank_my = len(res_diss)

        corr = set(res_diss[:len(phn_diss)]) & set(phn_diss)
        logging.debug("disease {1} pid {2} match {0}".format(float(len(corr)) / len(phn_diss), d['disease'], d['pid']))
        logging.debug("rank of correct result in my alg: {}".format(rank_my))


if __name__ == '__main__':
    eval(bm_db)
