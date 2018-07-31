import bisect
import datetime
import json
import logging
from multiprocessing.pool import ThreadPool
import os
from pymongo import MongoClient
import random
import re
from threading import Event


from alg import get_diseases


logging.basicConfig(level=logging.DEBUG, filename='log/test.raw.score.eval_on_bm.log', 
                    filemode='w', format='%(message)s')


##############
bm_db = MongoClient('localhost', 27017)['phenomizer']
benchmark = 'benchmark'
patients = 'benchmark_patients'
hpo_tree = 'hpo_tree'
phenomizer_result = 'benchmark_result'
prep_sim = 'hpo_disease_sim'
null_dist = 'score_distribution'
null_result = 'benchmark_result_null'
##############


class EvalOnBm(object):
    def __init__(self, db, phenomizer_manager, patient_manager, dag_manager, prep_sim_manager, null_manager, result_manager, pool_size=8):
        self.phenomizer_manager = phenomizer_manager
        self.patient_manager = patient_manager
        self.dag_manager = dag_manager
        self.prep_sim_manager = prep_sim_manager
        self.result_manager = result_manager
        self.null_manager = null_manager
        self.db = db
        self.logger = logging.getLogger()

        self.pool_size = pool_size
        self.stop_event = Event()

    def run(self):
        self.all_hpos = set(self.dag_manager.distinct('hpo'))
        self.all_hpos_used = self.prep_sim_manager.find_one({"type": "meta", "key": "all_hpos_used"})['value']
        self.all_dis_used = self.prep_sim_manager.find_one({"type": "meta", "key": 'all_diseases_used'})['value']
        self.all_dis_used_null = self.null_manager.find_one({"type": "meta", "key": 'all_diseases_used'})['value']

        pids = range(1, 101)
        pool = ThreadPool(processes=self.pool_size)
        pool.map(self.eval, pids)
        pool.close()
        pool.join()

        if self.stop_event.is_set():
            self.logger.warning("operation aborted halfway")

    def eval(self, pid):
        try:
            if self.stop_event.is_set():
                return False
            cur = self.phenomizer_manager.find({"pid": pid}).batch_size(1)
            for d in cur:
                if d['disease'] not in self.all_dis_used:
                    continue
                    
                re_res = re.findall(r'^(\w+):(\d+)$', d['disease'])
                if len(re_res) == 0:
                    continue
                phn_res = d['result']
                phn_diss = map(lambda d__: d__['disease'].replace("ORPHANET", "ORPHA"), phn_res)

                d_ = self.patient_manager.find_one({"disease": d['disease'], 'pid': d['pid']})
                query = d_['hpos']

                query = list(set(query) & self.all_hpos)
                if len(query) == 0:
                    continue
                result = get_diseases(self.db, query)
                result.sort(key=lambda t: (t[1], -t[2]))
                res_diss = map(lambda t: t[0], result)
                res_scrs = map(lambda t: (t[1], t[2]), result)
                res_ps = map(lambda t: t[1], result)

                try:
                    idx = res_diss.index(d['disease'])
                    rank_my = res_scrs.index(res_scrs[idx]) + 1
                except ValueError:
                    rank_my = 15000

                corr = set(res_diss[:len(phn_diss)]) & set(phn_diss)
                self.logger.debug(("disease {0} pid {1} match {2}\n"
                                   "rank of correct result in my alg: {3}\n"
                                   "query_size {4}").format(
                                        d['disease'],
                                        d['pid'],
                                        float(len(corr)) / len(phn_diss),
                                        rank_my,
                                        len(query)
                                   )
                                  )
                self.result_manager.update({"disease": d['disease'], "pid": d['pid']}, {
                    "$set": {
                        "result": result[:bisect.bisect_right(res_ps, 0.05)],
                        "up_date": datetime.datetime.utcnow()
                    }    
                }, upsert=True)
        except Exception as e:
            self.stop_event.set()
            self.logger.exception('pid {} failed.\n message: {}'.format(pid, str(e)))
        return True


if __name__ == '__main__':
    # eval(bm_db)
    db = MongoClient("localhost", 27017)['phenomizer']
    evalOnBm = EvalOnBm(db, db[phenomizer_result], db[patients], db[hpo_tree], db[prep_sim], db[null_dist], db[null_result])
    evalOnBm.run()
