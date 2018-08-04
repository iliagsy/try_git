# coding: utf8
from argparse import ArgumentParser
import datetime
import json
import logging
import logging.config
from multiprocessing.pool import ThreadPool
from pymongo import MongoClient
import sys
from threading import Event


######################
dag_lca = 'hpo_tree_lca'
anno = 'hpo_disease'
hpo_IC = 'hpo_IC'
disease_sim = 'hpo_disease_sim_'
####################


logging_conf_d = json.load(open("logging_conf.json", 'rb'))
logging.config.dictConfig(logging_conf_d)
logger = logging.getLogger(__name__)


class StoreDiseaseSim(object):
    def __init__(self, disease_sim_manager, anno_manager, lca_manager, IC_manager, start_hpo1, end_hpo1, pool_size=4):
        self.disease_sim_manager = disease_sim_manager
        self.anno_manager = anno_manager
        self.lca_manager = lca_manager
        self.IC_manager = IC_manager
        self.start_hpo1 = start_hpo1
        self.end_hpo1 = end_hpo1
        self.pool_size = pool_size

        self.stop_event = Event()

    def run(self):
        self.disease_sim_manager.create_index([("disease",1), ("hpo",1)])
        self.disease_sim_manager.create_index([("info.hpo1",1), ("info.range",1)])
        self.disease_sim_manager.create_index([("up_date",-1)])
        self.disease_sim_manager.create_index([("ct_date", -1)])
        self.disease_sim_manager.create_index([("type",1), ("key",1)])
        self.lca_manager.create_index([("traversed", 1)])

        doc = self.disease_sim_manager.find_one({"type": "meta", "key": "all_hpos_used"})
        if doc is None:
            self.all_hpos = self.lca_manager.find_one({"type": "meta", "key": "all_hpos_used"})['value']
            self.disease_sim_manager.update({"type": "meta", "key": "all_hpos_used"}, {"$set": {"value": self.all_hpos}}, upsert=True)
        else:
            self.all_hpos = doc['value']
        doc = self.disease_sim_manager.find_one({"type": "meta", "key": "all_diseases_used"})
        if doc is None:
            self.all_dis = self.anno_manager.distinct("disease")
            self.all_dis.sort()
            self.disease_sim_manager.update({"type": "meta", "key": "all_diseases_used"}, {"$set": {"value": self.all_dis}}, upsert=True)
        else:
            self.all_dis = doc['value']

        cur = self.IC_manager.find()
        self.IC_map = {d['hpo']:d['IC'] for d in cur}
        hpo_anno = self.anno_manager.distinct("hpo")
        self.hpo_anno_ids = map(self.all_hpos.index, hpo_anno)

        ranges = self.lca_manager.find({"hpo1": {"$gte": self.start_hpo1, "$lt": self.end_hpo1}}).distinct("range")
        pool = ThreadPool(processes=self.pool_size)
        pool.map(self.store, ranges)
        pool.close()
        pool.join()

        if self.stop_event.is_set():
            logger.warning("operation aborted halfway")

    def store(self, range_):
        try:
            if self.stop_event.is_set():
                return False
            cur = self.lca_manager.find({"traversed": {"$ne": True}, "range": range_, "hpo1": {"$gte": self.start_hpo1, "$lt": self.end_hpo1}, "type": "data"}, no_cursor_timeout=True
                                        ).sort([("hpo1", 1)]
                                        ).batch_size(1)
            c1 = cur.count()
            c2 = self.lca_manager.find({"range": range_, "hpo1": {"$gte": self.start_hpo1, "$lt": self.end_hpo1}, "type": "data"}).count()
            logger.debug("not traversed {c1} all {c2}".format(c1=c1,c2=c2))
            for d in cur:
                hpo1_id = d['hpo1']
                hpo2_id = hpo1_id + d['range']
                if hpo1_id not in self.hpo_anno_ids and hpo2_id not in self.hpo_anno_ids:
                    logger.debug("hpo1_id {} hpo2_id {}".format(hpo1_id, hpo2_id))
                    continue
                lca_id = d['lca']
                lca = self.all_hpos[lca_id]
                IC = self.IC_map[lca]
                hpo1, hpo2 = map(self.all_hpos.__getitem__, [hpo1_id, hpo2_id])
                diseases1 = db[anno].find({"hpo": hpo1}).distinct("disease")
                dis1_ids = map(self.all_dis.index, diseases1)
                diseases2 = db[anno].find({"hpo": hpo2}).distinct("disease")
                dis2_ids = map(self.all_dis.index, diseases2)
                for dids, hid in [(dis1_ids, hpo2_id), (dis2_ids, hpo1_id)]:
                    for did in dids:
                        db[disease_sim].update({"disease": did, "hpo": hid}, {
                            "$set": {
                                "info": {"hpo1": hpo1_id, "range": d['range']},
                                "type": "data",
                                "up_date": datetime.datetime.utcnow()
                            },
                            "$max": {
                                "score": IC
                            }
                        }, upsert=True)
                self.lca_manager.update({"_id": d['_id']}, {"$set": {"traversed": True}})
        except Exception as e:
            logger.exception("range `{}` failed,\n message: {}".format(range_, str(e)))
            return False
        return True


def store_disease_sim(db, drop=False, start_hpo1=1, end_hpo1=14000):
    if drop:
        db.drop_collection(disease_sim)
        db[disease_sim].create_index([("disease",1), ("hpo",1)])
        db[disease_sim].create_index([("info.hpo1",1), ("info.range",1)])
        db[disease_sim].create_index([("up_date",-1)])
        db[disease_sim].create_index([("ct_date", -1)])
        db[disease_sim].create_index([("type",1), ("key",1)])

    doc = db[disease_sim].find_one({"type": "meta", "key": "all_hpos_used"})
    if doc is None:
        all_hpos = db[dag_lca].find_one({"type": "meta", "key": "all_hpos_used"})['value']
        db[disease_sim].update({"type": "meta", "key": "all_hpos_used"}, {"$set": {"value": all_hpos}}, upsert=True)
    else:
        all_hpos = doc['value']
    doc = db[disease_sim].find_one({"type": "meta", "key": "all_diseases_used"})
    if doc is None:
        all_dis = db[anno].distinct("disease")
        all_dis.sort()
        db[disease_sim].update({"type": "meta", "key": "all_diseases_used"}, {"$set": {"value": all_dis}}, upsert=True)
    else:
        all_dis = doc['value']

    cur = db[hpo_IC].find()
    IC_map = {d['hpo']:d['IC'] for d in cur}

    hpo_anno = db[anno].distinct("hpo")
    hpo_anno_ids = map(all_hpos.index, hpo_anno)

    cur = db[dag_lca].find({"hpo1": {"$gte": start_hpo1, "$lt": end_hpo1}, "type": "data"}, no_cursor_timeout=True
                            ).sort([("hpo1",1), ("range",1)]
                            ).batch_size(1)
    for d in cur:
        hpo1_id = d['hpo1']
        hpo2_id = hpo1_id + d['range']
        if hpo1_id not in hpo_anno_ids and hpo2_id not in hpo_anno_ids:
            continue
        lca_id = d['lca']
        lca = all_hpos[lca_id]
        IC = IC_map[lca]
        hpo1, hpo2 = map(all_hpos.__getitem__, [hpo1_id, hpo2_id])
        diseases1 = db[anno].find({"hpo": hpo1}).distinct("disease")
        dis1_ids = map(all_dis.index, diseases1)
        diseases2 = db[anno].find({"hpo": hpo2}).distinct("disease")
        dis2_ids = map(all_dis.index, diseases2)
        for dids, hid in [(dis1_ids, hpo2_id), (dis2_ids, hpo1_id)]:
            for did in dids:
                db[disease_sim].update({"disease": did, "hpo": hid}, {
                    "$set": {
                        "info": {"hpo1": hpo1_id, "range": d['range']},
                        "type": "data",
                        "up_date": datetime.datetime.utcnow()
                    },
                    "$max": {
                        "score": IC
                    }
                }, upsert=True)



if __name__ == '__main__':
    db = MongoClient("localhost", 27017)['phenomizer']
    parser = ArgumentParser()
    parser.add_argument("-s", '--start', type=int, default=1)
    parser.add_argument("-e", '--end', type=int, default=14000)
    parsed = parser.parse_args()
    # store_disease_sim(db, False, start_hpo1=parsed.start, end_hpo1=parsed.end)
    store_disease_sim = StoreDiseaseSim(db[disease_sim], db[anno], db[dag_lca], db[hpo_IC], start_hpo1=parsed.start, end_hpo1=parsed.end)
    store_disease_sim.run()