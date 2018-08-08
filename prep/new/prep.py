# coding: utf8
from __future__ import absolute_import


from argparse import ArgumentParser
import itertools
from pymongo import MongoClient
import re


from gdconfig import ConfigManager


#####collection names####
hpo_anc = 'hpo_anc'
hpo_tree = 'hpo_tree'
anno = 'hpo_disease'
prep_anno = 'disease_full_hpo'
######################


hpo_freq_map = {                    # frequency -> weight映射
    "HP:0040283": [0.05, 0.29],
    "HP:0040284": [0.01, 0.04],
    "HP:0040280": [1.0],
    "HP:0040281": [0.8, 0.99],
    "HP:0040282": [0.3, 0.79],
    "HP:0040285": [0.0],
    "default": [0.3, 0.79]
}
EWMapping = {                       # evidence -> weight映射
    "IEA": 0.8,
    "PCS": 1.0,
    "ICE": 1.0,
    "ITM": 0.8,
    "TAS": 1.0
}
Lambda_disease = 0.5                 # 父权值=子权值*Lambda_disease


class PreDiseaseSubgraph(object):
    def __init__(self, db, drop=True):
        client = ConfigManager.ConfigManager().getdb().connection
        self.db = client[db]
        if drop:
            self.db.drop_collection(prep_anno)

    def run(self):
        self.prep_freq()
        self.add_level_to_hpo()

    def prep(self):
        '''根据evidence设定分数'''
        self.db[prep_anno].create_index([("disease", 1)], unique=True, background=True)
        cur = self.db[anno].aggregate([
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
                wgt.append(EWMapping[_d['f']])
            hpos, wgt = self._induce_anc_weight(hpos[:], wgt[:])
            hpos_w_wgt = []
            for h,w in zip(hpos, wgt):
                hpos_w_wgt.append({"id": h, "weight": w})
            self.db[prep_anno].insert({
                "disease": d['_id'],
                "hpos": hpos_w_wgt
            })

    def prep_freq(self):
        '''用频率作为分数'''
        self.db[prep_anno].create_index([("disease", 1)], unique=True, background=True)
        cur = self.db[anno].aggregate([
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
                wgt.append(self._parse_frequency(_d['f']))
            hpos, wgt = self._induce_anc_weight(hpos[:], wgt[:])
            hpos_w_wgt = []
            for h,w in zip(hpos, wgt):
                hpos_w_wgt.append({"id": h, "weight": w})
            self.db[prep_anno].insert({
                "disease": d['_id'],
                "hpos": hpos_w_wgt
            })

    def _induce_anc_weight(self, hpos, wgt):
        '''从一套（hpo,weight)向上生长为到根子树(hpo,weight)'''
        dpt = [0 for e in hpos]
        for hpo,ori_wgt in zip(hpos[:], wgt[:]):
            for d_anc in self.db[hpo_anc].find_one({"hpo": hpo})['ancestors']:
                if d_anc['hpo'] in hpos:
                    idx = hpos.index(d_anc['hpo'])
                    if dpt[idx] <= d_anc['depth']:
                        continue
                    del hpos[idx]
                    del wgt[idx]
                    del dpt[idx]
                hpos.append(d_anc['hpo'])
                wgt.append(ori_wgt * (Lambda_disease ** d_anc['depth']))
                dpt.append(d_anc['depth'])
        return hpos, wgt

    def _parse_frequency(self, s):
        s = s or 'default'

        if s in hpo_freq_map:
            return sum(hpo_freq_map[s]) / len(hpo_freq_map[s])
        res = re.findall(r'^(\d+)/(\d+)$', s)
        if len(res) > 0:
            return float(res[0][0]) / float(res[0][1])
        res = re.findall(r'^(\w+)\%$', s)
        assert len(res) > 0
        return int(res[0]) / 100.0

    def add_level_to_hpo(self):
        cur = self.db[prep_anno].find()
        for d in cur:
            hpos = map(lambda _d: _d['id'], d['hpos'])
            wgts = map(lambda _d: _d['weight'], d['hpos'])
            cur_ = self.db[hpo_tree].find({"hpo": {"$in": hpos}})
            level_d = {_d['hpo']: _d['level'] for _d in cur_}
            levels = map(level_d.get, hpos)
            hpos_ = []
            for t in zip(hpos, wgts, levels):
                hpos_.append(dict(zip(['id', 'weight', 'level'], t)))
            self.db[prep_anno].update({"_id":d['_id']}, {"$set": {"hpos": hpos_}})


def setupCommandLine():
    parser = ArgumentParser(
        description='[GeneDock] prepare disease-subgraph data for DiseaseMap algorithm use\n'
                    '[*** Note ***]\n'
                    '`hpo_tree`(ontology), `hpo_anc`(ancestor), `hpo_disease`(annotation) must already be imported and be present in the database referred to.'
    )
    parser.add_argument("-d", "--db", required=True, help="database name to import data into")
    return parser


if __name__ == "__main__":
    parser = setupCommandLine()
    parsed = parser.parse_args()
    PreDiseaseSubgraph(parsed.db, drop=True).run()



