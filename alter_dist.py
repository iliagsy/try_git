# coding: utf-8
import bisect
from collections import Counter
import datetime
import json
import logging, logging.config
import math
from multiprocessing.pool import ThreadPool
from pymongo import MongoClient
import sys
from threading import Event


############################
null_dist = 'score_distribution'
##############################


logging_conf_d = json.load(open("logging_conf.json", 'rb'))
logging.config.dictConfig(logging_conf_d)


class AlterDist(object):
    def __init__(self, null_manager, pool_size=4):
        self.null_manager = null_manager
        self.pool_size = pool_size

        self.stop_event = Event()
        self.logger = logging.getLogger(__name__)

    def run(self):
        sizes = self.null_manager.find({"type": "data"}).distinct("query_size")
        
        pool = ThreadPool(processes=self.pool_size)
        pool.map(self.alter, sizes)
        pool.close()
        pool.join()

        if self.stop_event.is_set():
            self.logger.warning("operation aborted halfway")

    def alter(self, size):
        try:
            self.logger.debug("altering size `{}`".format(size))
            if self.stop_event.is_set():
                return False

            keys = ["_id", "disease", "median", "upper_qt"]
            metadata = []
            cur = self.null_manager.find({"type": "data", "query_size": size})
            for d in cur:
                _id = d['_id']
                dist = d['dist'].copy()
                disease = d['disease']
                median, up_qt = self.find_median_upper_qt(d)
                # use accumulate count, with score sorted reversely,
                # and change key/score format
                # cut out scores below median
                dist_new = self.transform_dist(d)
                self.null_manager.update({"_id": d['_id']}, {
                    "$set": {
                        "dist": dist_new,
                        "up_date": datetime.datetime.utcnow()
                    }    
                })
                self.null_manager.update({"type": "meta", "key": "stats", "query_size": size}, {
                    "$push": {
                        "value": dict(zip(keys, [_id, disease, median, up_qt]))
                    },
                    "$set": {
                        "up_date": datetime.datetime.utcnow()
                    }
                }, upsert=True)
        except Exception as e:
            self.stop_event.set()
            self.logger.exception("alteration of size `{}` failed\n message {}".format(size, str(e)))
            return False
        return True

    def transform_dist(self, d):
        scr_w_cnt = sorted(d['dist'].items(), key=lambda t: float(t[0].replace('_', '.')), reverse=True)
        cnt_sorted = map(lambda t: t[1], scr_w_cnt)
        scr_w_cnt_accum = [[t[0], 0] for t in scr_w_cnt]
        for i in xrange(len(scr_w_cnt)):
            scr_w_cnt_accum[i][1] = sum(cnt_sorted[:i+1], 0)
        data = {}
        for scr_s, acc_cnt in scr_w_cnt_accum:
            if acc_cnt > d['n_sample'] / 2:
                acc_cnt = d['n_sample'] / 2
                data[scr_s] = acc_cnt
                break
            data[scr_s] = acc_cnt
        return data

    def find_median_upper_qt(self, d):
        scr_w_cnt = map(lambda t: (float(t[0].replace('_', '.')), t[1]), d['dist'].items())
        scr_w_cnt.sort(key=lambda t: t[0], reverse=True)
        cnt_sorted = map(lambda t: t[1], scr_w_cnt)
        scr_w_cnt_accum = [[t[0], 0] for t in scr_w_cnt]
        for i in xrange(len(scr_w_cnt)):
            scr_w_cnt_accum[i][1] = sum(cnt_sorted[:i+1], 0)
        for k,v in scr_w_cnt_accum:
            if v > d['n_sample'] / 2:
                median = k
                break
        for k,v in scr_w_cnt_accum:
            if v > d['n_sample'] / 4:
                upper_qt = k
                break
        return median, upper_qt


if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    # alter(db)
    alter_dist = AlterDist(db[null_dist])
    alter_dist.run()