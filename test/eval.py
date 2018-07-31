# coding: utf8
import bisect
import logging
from pymongo import MongoClient
import random
import timeit


from alg import get_diseases


logger = logging.getLogger(__name__)


##################
dag = 'hpo_tree'
anno = 'hpo_disease'
prep_sim = 'hpo_disease_sim'
null_dist = 'score_distribution'
bm_patients = 'benchmark_patients'
###################


def parse_low(fn='test/low.txt'):
    with open(fn, 'rb') as fh:
        lines = fh.readlines()
    blocks = []
    block = []
    for line in lines:
        line = line.strip()
        if len(line) == 0:
            continue
        block.append(line)
        if len(block) == 3:
            blocks.append(block)
            block = []
    for block in blocks:
        rank = int(block[1].split()[-1])
        if rank > 50:
            l = block[0].split()
            disease, pid = l[1], int(l[3])
            yield disease, pid


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    all_dis = db[anno].distinct("disease")
    all_hpos_used = db[prep_sim].find_one({"type": "meta", "key": "all_hpos_used"})['value']
    all_dis_used = db[prep_sim].find_one({"type": "meta", "key": 'all_diseases_used'})['value']

    err_cnt = 0
    i = 0
    for disease, pid in parse_low():
        print ">>>>>>>> disease {} pid {}".format(disease, pid)

        if disease not in all_dis_used:
            continue
        i += 1
        hpos_of_d = db[bm_patients].find_one({"disease": disease, "pid": pid})['hpos']
        hpos_of_d = list(set(all_hpos_used) & set(hpos_of_d))
        t = timeit.time.time()
        result = get_diseases(db, hpos_of_d)
        duration = timeit.time.time() - t
        result.sort(key=lambda t: (t[1], -t[2]))  # 排序根据(p_value, raw-score)
        # result.sort(key=lambda t: (-t[2], t[1]))  # 排序根据(raw-score, p-value)

        dis_ranked = map(lambda t: t[0], result)
        scr_ranked = map(lambda t: (t[1], t[2]), result)
        try:
            idx = dis_ranked.index(disease)
            rank = scr_ranked.index(scr_ranked[idx]) + 1
            print 'p_value of corr {} raw score of corr {}'.format(result[idx][1], result[idx][2])
        except:
            rank = 15000
        print 'disease {} disease id {} rank {}'.format(disease, all_dis_used.index(disease), rank)
        print '<<<<<<<'


'''
1.sort: {score: -1}, 正确结果在前20的比率：89.6% (query size >= 3)
2.sort: {p_value: 1, score: -1}
'''