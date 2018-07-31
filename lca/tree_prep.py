from collections import deque
from pymongo import MongoClient


####################
dag = 'hpo_tree'
split_tree = 'hpo_split_tree'
meta = 'meta'
euler_tour_key = "split_tree_euler_tour"
####################


def _euler_tour_r(db, node):
    lvl = db[split_tree].find_one({'hpo': node})['level']
    children = db[split_tree].find_one({"hpo": node})[u'children']
    euler_tour = [(node, lvl)]
    for c in children:
        c_euler_tour = _euler_tour_r(db, c)
        euler_tour += c_euler_tour
        euler_tour += [(node, lvl)]
    return euler_tour


def store_euler_tour(db):
    '''DFS访问节点顺序'''
    root = db[split_tree].find_one({"parents": {"$size": 0}})['hpo']
    euler_tour = _euler_tour_r(db, root)
    euler_tour = map(lambda t: {"hpo": t[0], "level": t[1]}, euler_tour)
    db[meta].update({"key": euler_tour_key}, {
        "$set": {
            "value": euler_tour
        }
    }, upsert=True)


def store_level_in_tree(db):
    root = db[split_tree].find_one({"parents": {"$size": 0}})['hpo']
    q = deque([(root, 0)])
    while len(q) > 0:
        hpo, depth = q.popleft()
        db[split_tree].update({"hpo": hpo}, {
            "$set": {
                "level": depth
            }
        }, upsert=True)
        children = db[split_tree].find_one({"hpo": hpo})[u'children']
        for c in children:
            q.append((c, depth + 1)) 


def store_index_in_euler_tour(db):
    all_hpos = db[split_tree].distinct("hpo")
    euler_tour = db[meta].find_one({"key": euler_tour_key})['value']
    euler_tour = map(lambda d: d['hpo'], euler_tour)
    db[split_tree].update({"euler_idx": {"$exists": False}}, {"$set": {"euler_idx": -1}}, multi=True)
    for hpo in all_hpos:
        idx = euler_tour.index(hpo)
        db[split_tree].update({"hpo": hpo}, {"$set": {"euler_idx": idx}}, w=1)


def prep(db):
    store_level_in_tree(db)
    store_euler_tour(db)
    store_index_in_euler_tour(db)


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    prep(db)
