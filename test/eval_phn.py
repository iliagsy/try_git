import json
import logging
from multiprocessing.pool import ThreadPool
import os
from pymongo import MongoClient
import random
import re
from threading import Event


logging.basicConfig(level=logging.DEBUG, filename='log/test.eval_on_bm.phenomizer.log', 
                    filemode='w', format='%(message)s')
logger = logging.getLogger(__name__)


####################
bm_result = 'benchmark_result'
#####################


def eval(db):
    cur = db[bm_result].find()
    for d in cur:
        result = d['result']
        result = map(lambda d: d['disease'], result)
        try:
            rank = result.index(d['disease']) + 1
        except:
            rank = 100
        logger.debug("disease {} pid {} rank {}".format(d['disease'], d['pid'], rank))


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    eval(db)
