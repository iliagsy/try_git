# coding: utf-8
'''hpo_tree, hpo_anc, annotation-text-file -> hpo_disease'''
from argparse import ArgumentParser, RawTextHelpFormatter
from bson import ObjectId
from collections import deque
import datetime
import itertools
import json
import logging
import logging.config
from pymongo import MongoClient
import random
import time


from gdconfig import ConfigManager


logging_conf_d = json.load(open("logging_conf.json", 'rb'))
logging.config.dictConfig(logging_conf_d)
logger = logging.getLogger(__name__)


class ImportAnnotation(object):
    hpo_tree = 'hpo_tree'
    hpo_disease = 'hpo_disease'
    hpo_anc = 'hpo_anc'
    freq_excl = 'HP:0040285'

    def __init__(self, db, filepath='data/phenotype.hpoa'):
        client = ConfigManager.ConfigManager().getdb().connection
        self.db = client[db]
        try:
            assert self.hpo_tree in self.db.collection_names()
            assert self.hpo_anc in self.db.collection_names()
        except Exception as e:
            logger.exception(str(e))
            logger.warning("`hpo_tree`, `hpo_anc` not imported yet. Better import them before importing annotation.")
            return
        self.filepath = filepath
        self.op_id = "{}-{}".format(time.time(), ObjectId())

    def run(self):
        try:        
            self.store_anno_raw()
            self.clear_anno_excluded()  # first
            self.set_true_path()        # second
        except Exception as e:
            logger.exception("operation aborted halfway, message\n {}".format(str(e)))
            logger.warning("removing all data inserted in this op")
            self.db[self.hpo_disease].remove({"op_id": self.op_id})

    def store_anno_raw(self):
        hpo_tree = self.hpo_tree
        hpo_disease = self.hpo_disease
        self.db[hpo_disease].create_index([("frequency",1)])
        self.db[hpo_disease].create_index([("hpo",1)])
        self.db[hpo_disease].create_index([("disease",1)])
        all_hpos = self.db[hpo_tree].distinct("hpo")
        all_alts = self.db[hpo_tree].distinct("alt_id")
        all_hpos = set(all_hpos)
        all_alts = set(all_alts)
        for data in self._gen_data():
            if data['hpo'] in all_hpos:
                pass
            elif data['hpo'] in all_alts:
                # alt_id的条目注释到主id上
                doc = self.db[hpo_tree].find_one({"alt_id": data['hpo']})
                data['hpo'] = doc['hpo']   
            else:
                continue
            data["op_id"] = self.op_id
            self.db[hpo_disease].insert(data)

    def _gen_data(self):
        keys = []
        with open(self.filepath) as fh:
            for line in fh.readlines():
                if line.startswith("#"):
                    data = line.lstrip('#').split()
                    keys = map(lambda s: s.lower(), data)
                    continue
                data = line.strip().split('\t')
                data__ = dict(zip(keys, data))
                if data__['qualifier'].lower() == 'not':
                    continue
                data__['disease'] = '{}:{}'.format(data__['db'], data__['db_object_id'])
                data__['hpo'] = data__['hpo_id']
                data__.pop('db'); data__.pop('db_object_id'); data__.pop('hpo_id')
                yield data__

    def clear_anno_excluded(self):
        '''remove annotation where frequency==excluded'''
        self.db[self.hpo_disease].remove({"frequency": self.freq_excl}, multi=True)

    def set_true_path(self):
        '''hpo_anc, hpo_disease -> hpo_disease
        祖先和后代同时注释的，祖先的条目设true_path=True
        '''
        hpo_disease = self.hpo_disease
        hpo_anc = self.hpo_anc
        all_hpos = self.db[hpo_disease].distinct("hpo")
        cur = self.db[hpo_disease].aggregate([
            {"$group": {"_id": "$hpo", "diseases": {"$push": "$disease"}}}
        ], cursor={})
        for d in cur:
            hpo = d['_id']
            diseases = list(set(d['diseases']))
            _l = self.db[hpo_anc].find_one({"hpo": hpo})['ancestors']
            anc = [d['hpo'] for d in _l if not d['hpo'] == hpo]
            self.db[hpo_disease].update({"hpo": {"$in": anc}, "disease": {"$in": diseases}}, {
                "$set": {
                    "true_path": True
                }
            })


def setupCommandLine():
    parser = ArgumentParser(
        description='[GeneDock] Import HP-Disease annotation from tsv file.\n\n'
                    '[*** Note ***]\n'
                    'Collections `hpo_tree` and `hpo_anc` must be present in the database supplied as arg',
        formatter_class=RawTextHelpFormatter
    )
    parser.add_argument("-d", "--db", required=True, help="database name to import data into; hpo_tree and hpo_anc should have been imported into the same db previously")
    parser.add_argument("-f", "--filepath", default="data/phenotype.hpoa", help="tsv annotation file downloaded from hpo official site")
    return parser


def main():
    parser = setupCommandLine()
    parsed = parser.parse_args()
    db_name = parsed.db
    filepath = parsed.filepath
    ImportAnnotation(db_name, filepath).run()


if __name__ == '__main__':
    main()