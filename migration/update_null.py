from collections import Counter
import datetime
import logging
from multiprocessing.pool import ThreadPool
from pymongo import MongoClient
import sys
from threading import Event


##########################
null_inc = 'score_distribution_copy_'
null = 'score_distribution_copy'
###########################


logging.basicConfig(stream=sys.stdout, formatter="%(message)s")
logger = logging.getLogger(__name__)


class UpdateNull(object):
    null_inc = 'score_distribution_copy_'
    null = 'score_distribution_copy'

    def __init__(self, db, pool_size=4):
        self.db = db
        self.pool_size = pool_size

        self.stop_event = Event()

    def run(self):
        qss = db[null].find({"type": "data"}).distinct("query_size")
        pool = ThreadPool(processes=self.pool_size)
        pool.map(self.migrate, qss)
        pool.close()
        pool.join()

        if self.stop_event.is_set():
            logger.warning("operation aborted halfway")

    def migrate(self, query_size):
        try:
            if self.stop_event.is_set():
                return
            db = self.db
            null_inc = self.null_inc
            null = self.null
            cur = db[null_inc].find({"type": "data", "query_size": query_size})
            for d in cur:
                disease = d['disease']
                
                inc_d = {"n_sample": d['n_sample']}
                dist_inc_d = {"dist.{}".format(k):v for k,v in d['dist'].items()}
                inc_d.update(dist_inc_d)

                db[null].update({"type": "data", "query_size": query_size, "disease": disease}, {
                    "$inc": inc_d,
                    "$set": {
                        "up_date": datetime.datetime.utcnow()
                    }
                })
                db[null_inc].update({"_id": d['_id']}, {"$set": {"removed": True}})
        except Exception as e:
            logger.exception("query_size {} failed,\n message: {}".format(query_size))
            self.stop_event.set()
        finally:
            db[null_inc].remove({"type": "data", "query_size": query_size, "removed": True}, multi=True)
        return



def migrate(db):
    try:
        qss = db[null].find({"type": "data"}).distinct("query_size")
        for query_size in qss:
            cur = db[null_inc].find({"type": "data", "query_size": query_size})
            for d in cur:
                disease = d['disease']
                
                inc_d = {"n_sample": d['n_sample']}
                dist_inc_d = {"dist.{}".format(k):v for k,v in d['dist'].items()}
                inc_d.update(dist_inc_d)

                db[null].update({"type": "data", "query_size": query_size, "disease": disease}, {
                    "$inc": inc_d,
                    "$set": {
                        "up_date": datetime.datetime.utcnow()
                    }
                })
                db[null_inc].update({"_id": d['_id']}, {"$set": {"removed": True}})
    except Exception as e: 
        raise e
    finally:
        db[null_inc].remove({"type": "data", "removed": True}, multi=True)


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    # migrate(db)
    UpdateNull(db).run()