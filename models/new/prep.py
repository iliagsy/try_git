from __future__ import absolute_import


from collections import deque
import itertools
from pymongo import MongoClient
import re


from .globals import Globals as _G


#####collection names####
hpo_anc = 'hpo_anc'
hpo_tree = 'hpo_tree'
prep_anno = 'disease_full_hpo'
anno = 'hpo_disease'
######################


def _parse_frequency(s):
    s = s or 'default'

    hpo_freq_map = _G.hpo_freq_map
    if s in hpo_freq_map:
        return sum(hpo_freq_map[s]) / len(hpo_freq_map[s])
    res = re.findall(r'^(\d+)/(\d+)$', s)
    if len(res) > 0:
        return float(res[0][0]) / float(res[0][1])
    res = re.findall(r'^(\w+)\%$', s)
    assert len(res) > 0
    return int(res[0]) / 100.0


def prep(db, drop=False):
    print "prep", _G()
    if drop:
        db.drop_collection(prep_anno)
        db[prep_anno].create_index([("disease", 1)], unique=True, background=True)
    cur = db[anno].aggregate([
        {"$group": {
            "_id": "$disease",
            "hpoEvidence": {"$push": {"e": "$evidence", "h": "$hpo"}}
        }}
    ], cursor={})
    for d in cur:
        hpos = []
        wgt = []
        for _d in d['hpoEvidence']:
            hpos.append(_d['h'])
            wgt.append(_G.EWMapping[_d['f']])
        hpos, wgt = induce_anc_weight(db, hpos[:], wgt[:])
        hpos_w_wgt = []
        for h,w in zip(hpos, wgt):
            hpos_w_wgt.append({"id": h, "weight": w})
        db[prep_anno].insert({
            "disease": d['_id'],
            "hpos": hpos_w_wgt
        })


def prep_freq(db, drop=False):
    if drop:
        db.drop_collection(prep_anno)
        db[prep_anno].create_index([("disease", 1)], unique=True, background=True)
    cur = db[anno].aggregate([
        {"$group": {
            "_id": "$disease",
            "hpoFreq": {"$push": {"f": "$frequency", "h": "$hpo"}}
        }}
    ], cursor={})
    for d in cur:
        hpos = []
        wgt = []
        for _d in d['hpoFreq']:
            hpos.append(_d['h'])
            wgt.append(_parse_frequency(_d['f']))
        hpos, wgt = induce_anc_weight(db, hpos[:], wgt[:])
        hpos_w_wgt = []
        for h,w in zip(hpos, wgt):
            hpos_w_wgt.append({"id": h, "weight": w})
        db[prep_anno].insert({
            "disease": d['_id'],
            "hpos": hpos_w_wgt
        })


def induce_anc_weight(db, hpos, wgt):
    dpt = [0 for e in hpos]
    for hpo,ori_wgt in zip(hpos[:], wgt[:]):
        for d_anc in db[hpo_anc].find_one({"hpo": hpo})['ancestors']:
            if d_anc['hpo'] in hpos:
                idx = hpos.index(d_anc['hpo'])
                if dpt[idx] <= d_anc['depth']:
                    continue
                del hpos[idx]
                del wgt[idx]
                del dpt[idx]
            hpos.append(d_anc['hpo'])
            wgt.append(ori_wgt * (_G.Lambda_disease ** d_anc['depth']))
            dpt.append(d_anc['depth'])
    return hpos, wgt


def _add_level_to_hpo(db):
    cur = db[prep_anno].find()
    for d in cur:
        hpos = map(lambda _d: _d['id'], d['hpos'])
        wgts = map(lambda _d: _d['weight'], d['hpos'])
        cur_ = db[hpo_tree].find({"hpo": {"$in": hpos}})
        level_d = {_d['hpo']: _d['level'] for _d in cur_}
        levels = map(level_d.get, hpos)
        hpos_ = []
        for t in zip(hpos, wgts, levels):
            hpos_.append(dict(zip(['id', 'weight', 'level'], t)))
        db[prep_anno].update({"_id":d['_id']}, {"$set": {"hpos": hpos_}})



if __name__ == "__main__":
    db = MongoClient('localhost', 27017)['phenomizer']
    prep_freq(db, drop=True)
    _add_level_to_hpo(db)




