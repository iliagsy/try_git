# coding: utf-8
import bisect
import logging
from pymongo import MongoClient
import random
import sys
import timeit


logger = logging.getLogger(__name__)


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

    def aggregate(self, *args, **kwargs):
        return self.collection.aggregate(*args, **kwargs)


class PrepSimManager(BaseManager):
    collection_name = 'hpo_disease_sim'

    def __init__(self, *args, **kwargs):
        BaseManager.__init__(self, *args, **kwargs)
        PrepSimManager.all_hpos_used = self.find_one({"type": "meta", "key": "all_hpos_used"})['value']
        PrepSimManager.all_dis_used = self.find_one({"type": "meta", "key": "all_diseases_used"})['value']

    def get_diseases(self, hpos):
        all_hpos_used, all_dis_used = self.all_hpos_used, self.all_dis_used
        hpos_ = hpos
        hpos = map(all_hpos_used.index, hpos_)
        cur = self.aggregate([
            {"$match": {"hpo": {"$in": hpos}}}, 
            {"$group": {"_id": "$disease", "total_score": {"$sum": "$score"}}},
            {"$sort": {"total_score": -1}}
        ], cursor={})

        results = map(lambda d: (d['_id'], d['total_score']), cur)
        results = map(lambda t: (all_dis_used[t[0]], t[1] / len(hpos)), results)
        return results


class NullDistManager(BaseManager):
    collection_name = 'score_distribution'

    def __init__(self, *args, **kwargs):
        BaseManager.__init__(self, *args, **kwargs)

    def get_p_value(self, query_size, disease_score_d):
        #####################
        cand_lim = 500
        stats = self.find_one({"type": "meta", "key": "stats", "query_size": query_size})['value']
        cand = []
        for d in stats:
            disease = d['disease']
            scr = disease_score_d.get(disease, 0.0)
            median, upper_qt, _id = map(d.get, ['median', 'upper_qt', "_id"])
            wgt = scr - median
            if wgt > 0:
                cand.append((_id, wgt, scr))
        if len(cand) > cand_lim:
            cand.sort(key=lambda t: t[1], reverse=True)
            cand = cand[:cand_lim]
        #############################
        result = []
        for _id, wgt, scr in cand:
            d = self.find_one({"_id": _id})
            disease = d['disease']
            rounded_scr = round(scr, 4)
            items = [(float(k.replace('_', '.')), v) for k,v in d['dist'].items()]
            scr_w_cnt = sorted(items, key=lambda t: t[0])
            l_scr_sorted = map(lambda t: t[0], scr_w_cnt)
            idx = bisect.bisect_left(l_scr_sorted, rounded_scr)
            if idx < len(l_scr_sorted):
                cnt_sum = scr_w_cnt[idx][1]
            else:
                cnt_sum = 0
            p = cnt_sum / float(d['n_sample'])
            result.append([disease, p, scr])
        result.sort(key=lambda t: t[1])  # 按{p_value:1} 排序
        sorted_p_value = map(lambda t: t[1], result)
        # p_value multi-test correction
        for i in xrange(len(result)):
            p = result[i][1]
            rank = sorted_p_value.index(p)  # 矫正前p-value相同，矫正后也相同
            result[i][1] = p / (rank+1) * len(result)
        result.sort(key=lambda t: (t[1], -t[2]))
        return result


def get_diseases(db, hpos, null=True):
    prep_sim_manager = PrepSimManager(db)
    null_manager = NullDistManager(db)
    dis_w_raw_scr = prep_sim_manager.get_diseases(hpos)
    if not null:
        return dis_w_raw_scr
        
    dis_scr_d = dict(dis_w_raw_scr)
    q_size = min(len(hpos), 29)
    if q_size > 10:
        q_size = q_size / 10 * 10
    try:
        dis_w_p_value = null_manager.get_p_value(q_size, dis_scr_d)
    except Exception as e: 
        logger.exception("q_size = {}".format(q_size))
        raise e
    return dis_w_p_value



if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
