from collections import Counter
import json
from pymongo import MongoClient
import pandas
import numpy as np
import matplotlib.pyplot as plt

###############################
null_dist = 'score_distribution_copy'
##################################


db = MongoClient("localhost", 27017)['phenomizer']
null_manager = db[null_dist]

data = null_manager.find_one({"disease": 0, "query_size": 20})
data.pop('_id')
data.pop("up_date")
print json.dumps(data, ensure_ascii=False, indent=4)

dist = [(float(k.replace('_', '.')),v) for k,v in data['dist'].items()]

c = Counter(dict(dist))
df = pandas.Series(list(c.elements()))

fig = plt.figure()
plt.xlim([0,4])
plt.ylim([0,5])
df.plot.hist(bins=50, density=True, align='mid')
plt.show()
