from argparse import ArgumentParser
from Crypto.Random.random import StrongRandom
import datetime
import json
import logging
import logging.config
from multiprocessing.pool import ThreadPool
from pymongo import MongoClient
# import random
import sys
from threading import Event


########################
dag = 'hpo_tree'
disease_sim = 'hpo_disease_sim'
null_dist = 'score_distribution_copy'
null_result = 'benchmark_result_null'
########################


logging_conf_d = json.load(open("logging_conf.json", 'rb'))
logging.config.dictConfig(logging_conf_d)
logger = logging.getLogger(__name__)

random = StrongRandom()



class StoreNull(object):
    def __init__(self, prep_sim_manager, null_manager, precision=3, n_query=128, pool_size=4):
        self.prep_sim_manager = prep_sim_manager
        self.null_manager = null_manager
        self.precision = precision
        self.pool_size = pool_size
        self.n_query = n_query

        self.stop_event = Event()

    def run(self):
        logger.info('start generating and inserting `{}` null sim scores'.format(self.n_query))

        self.null_manager.create_index([("disease",1), ("query_size",1)])
        self.null_manager.create_index([("up_date",-1)])
        self.null_manager.create_index([("query_size", 1)])
        self.null_manager.create_index([("type", 1), ("key", 1)])
        logger.info('creating index `{}` `{}` for null dist'.format('disease_1_query_size_1', 'up_date_-1'))
        
        all_hpos_used_in_sim = self.prep_sim_manager.find_one({"type": "meta", "key": "all_hpos_used"})['value']
        self.all_hpos_used_idx = range(len(all_hpos_used_in_sim))

        self.all_dis_used_sim = self.prep_sim_manager.find_one({"type": "meta", "key": "all_diseases_used"})['value']
        logger.info('prepared necessary data all_hpos, all_diseases, etc..')

        sizes = range(1, 11) + [20]
        pool = ThreadPool(processes=self.pool_size)
        pool.map(self.store_score_dist, sizes)
        pool.close()
        pool.join()

        if self.stop_event.is_set():
            logging.warning("inserting `{}` null scores for each (disease, query_size) aborted halfway.")

    def store_score_dist(self, size):
        logger.debug("start generating & storing null scores for query size `{}`".format(size))
        try:
            if self.stop_event.is_set():
                return False
            for i in xrange(self.n_query):
                hpos = self.gen_hpo_idxs(size)
                cur = self.prep_sim_manager.aggregate([
                    {"$match": {"hpo": {"$in": hpos}}},
                    {"$group": {"_id": "$disease", "total_score": {"$sum": "$score"}}}
                ], cursor={})
                result = map(lambda d: (d['_id'], d['total_score'] / size), cur)
                result_w_dn = map(lambda t: (self.all_dis_used_sim[t[0]], t[1]), result)
                #######################################################
                result_d = dict(result_w_dn)
                for disease in self.all_dis_used_sim:
                    scr = result_d.get(disease, 0.0)
                    scr_key = "{1:.{0}f}".format(self.precision, scr).replace('.', '_')                       
                    self.null_manager.update({"query_size": size, "disease": disease}, {
                        "$set": {
                            "type": "data",
                            "up_date": datetime.datetime.utcnow()
                        },
                        "$inc": {
                            "n_sample": 1,
                            "dist.{}".format(scr_key): 1
                        }
                    }, upsert=True)
        except Exception as e:
            logger.exception('storing query size `{}` failed'.format(size))
            self.stop_event.set()
        return True

    def gen_hpo_idxs(self, size):
        return random.sample(self.all_hpos_used_idx, size)


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-n', '--n-query', type=int, default=10)
    parser.add_argument('-p', '--precision', type=int, default=3)
    parsed = parser.parse_args()
    db = MongoClient("localhost", 27017)['phenomizer']
    store_null = StoreNull(db[disease_sim], db[null_dist], precision=parsed.precision, n_query=parsed.n_query)
    store_null.run()

