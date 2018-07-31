from argparse import ArgumentParser
from Crypto.Random.random import StrongRandom
import datetime
import logging
from multiprocessing.pool import ThreadPool
from pymongo import MongoClient
# import random
import sys
from threading import Event


########################
dag = 'hpo_tree'
disease_sim = 'hpo_disease_sim'
null_dist = 'score_distribution_copy_'
null_result = 'benchmark_result_null'
########################


logging.basicConfig(stream=sys.stderr, format="%(message)s")
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

random = StrongRandom()



class StoreNull(object):
    def __init__(self, prep_sim_manager, null_manager, dag_manager, null_result_manager, n_query=128, pool_size=4):
        self.prep_sim_manager = prep_sim_manager
        self.null_manager = null_manager
        self.dag_manager = dag_manager
        self.null_result_manager = null_result_manager
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
        all_hpos = self.dag_manager.distinct("hpo")
        self.all_hpos_idx = range(len(all_hpos))
        self.all_hpos_used_idx = range(len(all_hpos_used_in_sim))

        doc = self.null_manager.find_one({"type": "meta", "key": "all_diseases_used"})
        if doc is None:
            self.all_dis_used = self.prep_sim_manager.find_one({"type": "meta", "key": "all_diseases_used"})['value']
            self.null_manager.update({"type": "meta", "key": "all_diseases_used"}, {"$set": {"value": self.all_dis_used}}, upsert=True)
        else:
            self.all_dis_used = doc['value']

        self.all_dis_used_sim = self.prep_sim_manager.find_one({"type": "meta", "key": "all_diseases_used"})['value']

        self.target_disease_names = set(self.parse_low())
        self.target_diseases = map(self.all_dis_used_sim.index, self.target_disease_names)
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
                # map disease-id-in-prep_sim to disease-id-in-null-dist
                result_w_dn = map(lambda t: (self.all_dis_used_sim[t[0]], t[1]), result)
                result = map(lambda t: (self.all_dis_used.index(t[0]), t[1]), result_w_dn)
                #######################################################
                result_d = dict(result)
                for disease in xrange(len(self.all_dis_used)):
                # for disease in self.target_diseases:
                    scr = result_d.get(disease, 0.0)
                    scr_key = "{:.3f}".format(scr).replace('.', '_')
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
        '''
        :param all_hpos_used_idx:  hpos used in disease-hpo sim
        :param all_hpos_idx:       all hpos existing in hpo_tree
        '''
        query = []
        for i in xrange(size):
            hpo = random.choice(self.all_hpos_idx)
            if hpo in self.all_hpos_used_idx:
                query.append(hpo)
        return query

    def parse_low(self):
        cur = self.null_result_manager.find()
        for d in cur:
            if len(d['result']) == 0:
                continue
            corr = d['disease']
            dis_ranked = map(lambda t: t[0], d['result'])
            try:
                corr_idx_in_res = dis_ranked.index(corr)
            except ValueError:
                corr_idx_in_res = -1
            corr_p_in_res = d['result'][corr_idx_in_res][1]
            for dis in map(lambda t: t[0], filter(lambda t: t[1] < corr_p_in_res, d['result'])):
                yield dis



if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-n', '--n-query', type=int, default=10)
    parsed = parser.parse_args()
    db = MongoClient("localhost", 27017)['phenomizer']
    # store_score_distribution(db, parsed.n_query)
    store_null = StoreNull(db[disease_sim], db[null_dist], db[dag], db[null_result], parsed.n_query)
    store_null.run()

