import json
import logging
import os
from pymongo import MongoClient
import random
import re


from ..new.model import get_diseases


logging.basicConfig(level=logging.DEBUG, filename='eval/bmRes-models.new.txt', 
                    filemode='w', format='%(message)s')


##############
bm_db = MongoClient('localhost', 27017)['phenomizer']
benchmark = 'benchmark'
patients = 'benchmark_patients'
hpo_tree = 'hpo_tree'
##############


def eval(db):
    all_hpos = set(db[hpo_tree].distinct('hpo'))

    cur = bm_db[patients].find().sort([("pid", 1)])
    for d in cur:
        if len(re.findall(r'(\w+):(\d+)', d['disease'])) == 0:
            continue
        query = d['hpos']

        query = list(set(query) & all_hpos)
        if not len(query) >= 3:
            continue

        result = get_diseases(db, query)
        res_diss = map(lambda t: t[0], result)
        try:
            rank = res_diss.index(d['disease']) + 1
        except ValueError:
            rank = len(res_diss)
        logging.debug("disease {} patient_id {} rank {}".format(d['disease'], d['pid'], rank))
        print "disease {} patient_id {} rank {}".format(d['disease'], d['pid'], rank)


if __name__ == '__main__':
    db = bm_db
    eval(db)