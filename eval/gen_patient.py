import random
import itertools
from pymongo import MongoClient


def gen(db):
    keys = ['and', 'and/or', 'or']
    cur = db['benchmark'].find()
    for _d in cur:
        disease = _d[u'disease']
        for pid in xrange(1, 101):
            traits = []
            for doc in _d[u'hpo-freq']:
                freq = doc['frequency']
                hpo = doc['hpo']
                if isinstance(hpo, str) or isinstance(hpo, unicode):
                    if random.random() < freq:
                        traits.append(hpo)
                elif isinstance(hpo, dict):
                    if 'and' in hpo:
                        hpos_ = hpo.get('and')
                        if random.random() < freq:
                            traits += hpos_
                    elif 'or' in hpo:
                        hpos_ = hpo.get('or')
                        if random.random() < freq:
                            hpo__ = random.choice(hpos_)
                            traits.append(hpo__)
                    elif "and/or" in hpo:
                        hpos_ = hpo.get('and/or')
                        if random.random() < freq:
                            num = random.randint(1,len(hpos_))
                            hpos__ = random.choice(
                                list(itertools.combinations(hpos_, num))
                            )
                            traits += list(hpos__)
                else:
                    print hpo.__class__
                    raise Exception("hpo unknown form")
            yield disease, pid, list(set(traits))


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    keys = ['disease', 'pid', 'hpos']
    for data in gen(db):
        db['benchmark_patients'].update({"disease": data[0], "pid": data[1]}, {
            "$set": {
                "hpos": data[2]
            }
        }, upsert=True)
