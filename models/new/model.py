# coding: utf-8
from __future__ import absolute_import
from .globals import Globals as _G
from util import profile


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


class HPGraphManager(BaseManager):
    collection_name = "hpo_tree"

    def __init__(self, db):
        super(HPGraphManager, self).__init__(db)

    def ext_border(self, **kw):
        '''
        :param hpos_w_weight: a list of nodes which constitute a subgraph
        '''
        hpos_wgt = kw.get('hpos_w_weight', [])

        hpos = map(lambda t: t[0], hpos_wgt)
        cur = self.find({'hpo': {"$in": hpos}})

        parents = set()
        for d in cur:  # bottleneck
            s_parents = set(d['parents'])
            parents |= s_parents

        border = []
        for id_, wgt in hpos_wgt:
            if id_ not in parents:
                border.append((id_, wgt))
        return border


class HPAncManager(BaseManager):
    collection_name = 'hpo_anc'

    def __init__(self, db):
        super(HPAncManager, self).__init__(db)

    def get_full_hpo(self, **kwargs):
        should_lst = kwargs.get('hpos', [])
        cur = self.find({"hpo": {"$in": should_lst}})
        result = []
        for _d in cur:
            result += map(lambda __d: __d['hpo'], _d['ancestors'])
        return list(set(result))

    def get_full_hpo_w_d(self, **kwargs):
        '''get full hpo with distance to leaves'''
        should_lst = kwargs.get('hpos', [])
        cur = self.find({"hpo": {"$in": should_lst}})
        result = []
        for _d in cur:
            result += _d['ancestors']
        return result


class AnnoManager(BaseManager):
    collection_name = 'disease_full_hpo'


def reduce_weight(weights):
    return sum(weights)


# @profile
def get_diseases(db, query):
    # print "model", _G()
    Lambda = _G.Lambda_query
    lvl_bnd = _G.prim_lvl_bnd
    dis_lim = _G.prim_dis_lim
    result = []
    ancManager = HPAncManager(db)
    hpManager = HPGraphManager(db)
    annManager = AnnoManager(db)
    full_hpo = {}
    for _d in ancManager.get_full_hpo_w_d(hpos=query):
        full_hpo[_d['hpo']] = _d['depth']
    cur = annManager.find({"hpos": {"$elemMatch": {
        "id": {"$in": full_hpo.keys()},
        "level": {"$gte": lvl_bnd}
    }}})
    if cur.count() > dis_lim:
        _l = list(cur)
        fhl = len(full_hpo)
        def sort_key(doc):
            hpos = map(lambda d: d['id'], doc['hpos'])
            return len(set(hpos) & set(full_hpo.keys())) / float(fhl)
        _l.sort(key=sort_key, reverse=True)
        _l = _l[:dis_lim]
    else:
        _l = cur
    for d in _l:
        disease = d['disease']
        itsc_id = []
        itsc_wgt = []
        for d_ in d['hpos']:
            id_, wgt = map(d_.get, ['id', 'weight'])
            if id_ not in itsc_id and id_ in full_hpo:
                itsc_id.append(id_)
                itsc_wgt.append(wgt * Lambda ** full_hpo[id_])
        itsc = zip(itsc_id, itsc_wgt)
        border_w_wgt = hpManager.ext_border(hpos_w_weight=itsc)
        score = reduce_weight(map(lambda t: t[1], border_w_wgt))
        result.append((disease, score))
    result.sort(key=lambda t: t[1], reverse=True)
    return result


