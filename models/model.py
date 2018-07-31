from pymongo import MongoClient
from Queue import Queue
import random
import threading
import time
import os


from util import setupLogger


logger = setupLogger(__name__, 'log/{}.log'.format("models.model"))



class BaseManager(object):
    collection_name = ''

    def __init__(self, db):
        self.collection = db[self.collection_name]

    def find(self, *args, **kwargs):
        return self.collection.find(*args, **kwargs)

    def find_one(self, *args, **kwargs):
        return self.collection.find_one(*args, **kwargs)

    def distinct(self, *args, **kwargs):
        return self.collection.distinct(*args, **kwargs)



class SimManager(BaseManager):
    collection_name = 'hpo_sim'

    def simScoreSet(self, qSet, tSet):
        cur = self.find({"$or": [
            {"hpo1": {"$in": tSet}, "hpo2": {"$in": qSet}},
            {"hpo1": {"$in": qSet}, "hpo2": {"$in": tSet}}
        ]})
        max_d = {}
        for d in cur:
            hpo1, hpo2, score = d['hpo1'], d['hpo2'], d['score']
            if hpo1 in qSet:
                max_d[hpo1] = max(max_d.get(hpo1, -1), score)
            else:
                max_d[hpo2] = max(max_d.get(hpo2, -1), score)
        scores = max_d.values()
        mean_ = sum(scores) / len(scores)
        return mean_


class HpGraphManager(BaseManager):
    collection_name = 'hpo_tree'

    def clear_hpos_anc(self, hpos):
        cur = db['hpo_tree'].find({'hpo': {"$in": hpos}})
        parents = sum(map(lambda d: d['parents'], cur), [])
        return list(set(hpos) - set(parents))


class AnnoManager(BaseManager):
    collection_name = 'hpo_disease'

    def get_all_diseases(self):
        return self.distinct("disease")

    def get_hpo_by_disease(self, dis):
        return self.find({"disease": dis}).distinct("hpo")


def get_diseases(db, query):
    sManager = SimManager(db)
    anManager = AnnoManager(db)
    hpManager = HpGraphManager(db)
    query = hpManager.clear_hpos_anc(query)
    scores = []
    all_diseases = anManager.get_all_diseases()
    for dis in all_diseases:
        tSet = anManager.get_hpo_by_disease(dis)
        scr = sManager.simScoreSet(query, tSet)
        if scr < 0.01:
            continue
        scores.append((dis, scr))
        logger.debug("query {} dis {} score {}".format(query, dis, scr))

    scores.sort(key=lambda t: t[1], reverse=True)
    return scores


class Worker(threading.Thread):
    def __init__(self, db, queue, out_queue):
        threading.Thread.__init__(self)
        self.db = db
        self.queue = queue
        self.out_queue = out_queue

    def run(self):
        while True:
            d = self.queue.get()
            pid = d['pid']
            qSet = d['hpos']
            cor_dis = d['disease']
            if 'MIM' not in cor_dis:
                continue
            res = get_diseases(self.db, qSet)
            res = map(lambda t:t[0], res)
            try:
                rank = res.index(cor_dis) + 1
            except ValueError:
                rank = None
            self.out_queue.put(rank)
            self.queue.task_done()
            logger.debug("queue {} out_queue {}".format(self.queue.qsize(), self.out_queue.qsize()))


class StoreWorker(threading.Thread):
    def __init__(self, queue, fn):
        threading.Thread.__init__(self)
        self.queue = queue
        self.fn = fn

    def run(self):
        try:
            os.remove(self.fn)
        except:
            pass
        while True:
            while self.queue.qsize() == 0:
                time.sleep(1)
            with open(self.fn, 'a') as fh:
                rank = self.queue.get()
                fh.write('{}\n'.format(rank))
                self.queue.task_done()



def eval():
    db = MongoClient('localhost', 27017)['phenomizer']
    all_dis = db['hpo_disease'].distinct("disease")
    queue = Queue()
    store_queue = Queue()
    for i in xrange(5):
        worker = Worker(db, queue, store_queue)
        worker.start()
    storeWorker = StoreWorker(store_queue, 'eval/bmRes-sim.txt')
    storeWorker.start()

    cur = db['benchmark_patients'].find({"disease": {'$in': all_dis}})
    for d in cur:
        queue.put(d)

    queue.join()
    store_queue.join()

        


if __name__ == "__main__":
    # db = MongoClient('localhost', 27017)['phenomizer']
    # all_hpos = db['hpo_tree'].distinct("hpo")
    # qSize = 15
    # qSet = []
    # for i in xrange(qSize):
    #     qSet.append(random.choice(all_hpos))
    # for t in get_diseases(db, qSet):
    #     print t
    eval()