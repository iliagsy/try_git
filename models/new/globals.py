import json

class Globals(object):

    hpo_freq_map = {
        "HP:0040283": [0.05, 0.29],
        "HP:0040284": [0.01, 0.04],
        "HP:0040280": [1.0],
        "HP:0040281": [0.8, 0.99],
        "HP:0040282": [0.3, 0.79],
        "HP:0040285": [0.0],
        "default": [0.3, 0.79]
    }
    EWMapping = {
        "IEA": 0.8,
        "PCS": 1.0,
        "ICE": 1.0,
        "ITM": 0.8,
        "TAS": 1.0
    }
    Lambda_disease = 0.5

    Lambda_query = 0.5

    prim_lvl_bnd = 4
    prim_dis_lim = 1000

    @classmethod
    def to_dict(cls):
        keys = ['EWMapping', 'Lambda_query', "Lambda_disease", 'prim_dis_lim', 'prim_lvl_bnd']
        d = {}
        for k,v in cls.__dict__.items():
            if k in keys:
                d[k] = v
        return d

    def __repr__(self):
        return json.dumps(self.to_dict(), ensure_ascii=False)