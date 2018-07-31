from pymongo import MongoClient
import random
import timeit
import unittest


from ..new.model import get_diseases, HPGraphManager, HPAncManager, AnnoManager
from ..new.prep import EWMapping
from util import setupLogger


logger = setupLogger(__name__, 'log/models.tests.test_new_model.log')


class NewModelTestCase(unittest.TestCase):
    prep_anno = 'disease_full_hpo'
    anno = 'hpo_disease'

    @classmethod
    def setUpClass(cls):
        cls.db = MongoClient('localhost', 27017)['phenomizer']
        cls.all_dis = cls.db[cls.anno].distinct("disease")
        cls.ancManager = HPAncManager(cls.db)
        cls.annManager = AnnoManager(cls.db)
        cls.hpManager = HPGraphManager(cls.db)

    def get_hpos_by_disease(self, dis, border_only=False):
        query_d = {"disease": dis}
        if border_only:
            query_d.update({"$or": [{"true_path": False},
                                    {"true_path": {"$exists": False}}]})
        cur = self.db[self.anno].find(query_d)
        hpos = set()
        for d in cur:
            hpos.add(d['hpo'])
        return list(hpos)

    def method(self, *args, **kwargs):
        return get_diseases(self.db, *args, **kwargs)

    def run_method(self):
        disease = random.choice(self.all_dis)
        hpos = self.get_hpos_by_disease(disease)
        result = self.method(hpos)
        res_hpos = map(lambda t: t[0], result)
        rank = res_hpos.index(disease)
        return rank

    def _test_1(self):
        disease = 'ORPHA:96176'
        hpos = self.get_hpos_by_disease(disease)
        result = self.method(hpos)

    def test_disease_as_query(self):
        err = 0
        N = 100
        for i in xrange(N):
            disease = random.choice(self.all_dis)
            hpos = self.get_hpos_by_disease(disease)
            result = self.method(hpos)
            res_hpos = map(lambda t: t[0], result)
            res_scr = map(lambda t: t[1], result)
            try:
                idx = res_hpos.index(disease)
            except:
                err += 1
                logger.debug("disease {} not returned at all".format(disease))
                continue
            scr = res_scr[idx]
            if scr != res_scr[0]:
                logger.debug("returned {} correct {}".format(res_hpos[0], disease))
                err += 1
        logger.debug('error rate {}'.format(err / float(N)))


        


if __name__ == '__main__':
    unittest.main()
