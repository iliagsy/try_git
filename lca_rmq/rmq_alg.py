import math
from pymongo import MongoClient



class RMQAlg(object):
    def __init__(self):
        pass

    @classmethod
    def one_pair_rmq(cls, rmq, values, idx1, idx2):
        _range = idx2 - idx1
        pow_ = int(math.log(_range, 2))
        encl_range = 2 ** pow_
        min_idx1 = RMQAlg._get_rmq_idx(rmq, idx1, encl_range)
        min_idx2 = RMQAlg._get_rmq_idx(rmq, idx2 - encl_range, encl_range)
        min_idx = min_idx1 if values[min_idx1] < values[min_idx2] else min_idx2
        return min_idx

    @classmethod
    def prepare_2_pow_rmq(cls, rmq, values):
        _range = 1
        while _range < len(values):
            for idx1 in xrange(0, len(values)):
                idx2 = idx1 + _range
                if idx2 >= len(values): break
                if _range == 1:
                    min_idx = idx1 if values[idx1] < values[idx2] else idx2
                else:
                    idx_mid = idx1 + _range / 2
                    min_idx1 = RMQAlg._get_rmq_idx(rmq, idx1, _range / 2)
                    min_idx2 = RMQAlg._get_rmq_idx(rmq, idx_mid, _range / 2)
                    min_idx = min_idx1 if values[min_idx1] < values[min_idx2] else min_idx2
                RMQAlg._store_rmq(rmq, idx1, _range, min_idx)
            _range *= 2

    @classmethod
    def _get_rmq_idx(cls, rmq, idx1, _range):
        return rmq.find_one({"idx1": idx1, "range": _range})[u'min_idx']

    @classmethod
    def _store_rmq(cls, rmq, idx1, _range, min_idx):
        idx2 = idx1 + _range
        rmq.update({"idx1": idx1, "range": _range}, {"$set": {"min_idx": min_idx}}, upsert=True)








if __name__ == '__main__':
    pass