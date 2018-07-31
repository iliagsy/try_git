import datetime
import itertools
import logging
from multiprocessing.pool import ThreadPool
from pymongo import MongoClient
import sys
from threading import Event


###########################
null_dist = 'score_distribution_copy'
##############################


logging.basicConfig(stream=sys.stdout, format="%(message)s")
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class ChangePrec(object):
    def __init__(self, null_manager, pool_size=4):
        self.null_manager = null_manager
        self.pool_size = pool_size

        self.stop_event = Event()

    def run(self):
        sizes = self.null_manager.find({"type": "data"}).distinct('query_size')

        pool = ThreadPool(processes=self.pool_size)
        pool.map(self.alter, sizes)
        pool.close()
        pool.join()

        if self.stop_event.is_set():
            logger.warning("operation aborted halfway")

    def alter(self, size):
        try:
            if self.stop_event.is_set():
                return False
            cur = self.null_manager.find({"type": "data", "query_size": size})
            for d in cur:
                logger.debug("{} {}".format(d['disease'], d['query_size']))
                new_dist = self.reduce_precision(d['dist'])
                self.null_manager.update({"_id": d['_id']}, {
                    "$set": {
                        "dist": new_dist,
                        "up_date": datetime.datetime.utcnow()
                    }
                })
        except Exception as e:
            logger.exception("altering size `{}` failed,\n message {}".format(size, str(e)))
            return False
        return True

    def reduce_precision(self, dist):
        if self.stop_event.is_set():
            return False
        scr_w_cnt = map(lambda t: (float(t[0].replace("_", '.')), t[1]), dist.items())
        scr_w_cnt.sort(key=lambda t: t[0])
        new_dist = {}
        for k,v in itertools.groupby(scr_w_cnt,
                                    lambda t: "{:.3f}".format(t[0]).replace('.', '_')
                                    ):
            v = list(v)

            sum_ = 0
            for t in v:
                sum_ += t[1]
            new_dist[k] = sum_
        assert sum(new_dist.values()) == sum(dist.values())
        return new_dist


if __name__ == "__main__":
    db = MongoClient('localhost', 27017)['phenomizer']
    ChangePrec(db[null_dist], pool_size=4).run()