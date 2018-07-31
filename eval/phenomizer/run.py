import argparse
import datetime
from pymongo import MongoClient
from pymongo.errors import CursorNotFound
import re
from selenium.common.exceptions import StaleElementReferenceException, TimeoutException
from selenium import webdriver


from page import MainPage

##############
bm_db = MongoClient('localhost', 27017)['phenomizer']
benchmark = 'benchmark'
patients = 'benchmark_patients'
benchmark_result = 'benchmark_result'
hpo_tree = 'hpo_tree'
##############


def eval(db, drop=False, start_pid=1):
    if drop:
        db.drop_collection(benchmark_result)
    URL = 'http://compbio.charite.de/phenomizer/'
    client = webdriver.Firefox()
    while True:
        try:
            client.get(URL)
        except:
            continue
        break
    mainPage = MainPage(client)

    all_hpos = set(db[hpo_tree].distinct('hpo'))
    while True:
        try:
            mxPid = start_pid
            cur = bm_db[patients].find({"pid": {"$gte": mxPid}}
                                       ).sort([("pid", 1), ("disease", 1)])
            for d in cur:
                if len(re.findall(r'(\w+):(\d+)', d['disease'])) == 0:
                    continue
                doc__ = db[benchmark_result].find_one({"disease": d['disease'], "pid": d['pid']})
                if doc__ is not None and len(doc__['result']) > 10:
                    continue
                query = d['hpos']
                query = list(set(query) & all_hpos)
                mainPage.get_diseases(query)
                web_result = []
                mainPage.LIMIT = 1000
                for data in mainPage.result:
                    did = data[1]
                    if did.split(':')[0] == 'ORPHANET':
                        did.replace('ORPHANET', 'ORPHA')
                    web_result.append({"disease": did, "p_value": data[0]})
                db[benchmark_result].update({"disease": d['disease'], "pid": d['pid']}, {
                    "$set": {
                        "result": web_result,
                        "up_date": datetime.datetime.utcnow()
                    }
                }, multi=True, upsert=True)
        except CursorNotFound as e:
            continue
        except StaleElementReferenceException as e:
            while True:
                try:
                    mainPage.client.refresh()
                    mainPage = MainPage(client)
                except:
                    continue
                break
            continue
        except TimeoutException as e:
            while True:
                try:
                    mainPage.client.refresh()
                    mainPage = MainPage(client)
                except:
                    continue
                break
            continue
        break



def setupArgParser():
    ap = argparse.ArgumentParser()
    ap.add_argument("-p", '--pid', type=int)
    return ap


if __name__ == '__main__':
    parsed = setupArgParser().parse_args()
    pid = parsed.pid
    db = bm_db
    eval(db, start_pid=pid)


# File "eval/phenomizer/run.py", line 77, in <module>
#     pid = parsed.pid
#   File "eval/phenomizer/run.py", line 55, in eval
#     web_result = []
#   File "/Users/gaoshiyu/Desktop/genedock/phenomizer/eval/phenomizer/page.py", line 53, in result
#     lambda c: self.disRes[0].text.strip() != ''
#   File "/Users/gaoshiyu/Desktop/genedock/phenomizer/venv-dev/lib/python2.7/site-packages/selenium/webdriver/support/wait.py", line 71, in until
#     value = method(self._driver)
#   File "/Users/gaoshiyu/Desktop/genedock/phenomizer/eval/phenomizer/page.py", line 53, in <lambda>
#     lambda c: self.disRes[0].text.strip() != ''
#   File "/Users/gaoshiyu/Desktop/genedock/phenomizer/eval/phenomizer/element.py", line 91, in __get__
#     lambda c: c.find_elements(*self.locator2))
#   File "/Users/gaoshiyu/Desktop/genedock/phenomizer/venv-dev/lib/python2.7/site-packages/selenium/webdriver/support/wait.py", line 80, in until
#     raise TimeoutException(message, screen, stacktrace)
# selenium.common.exceptions.TimeoutException: Message: 


# [7]   Exit 1                  python eval/phenomizer/run.py -p 25




# Bad Request

# Your browser sent a request that this server could not understand.
# Apache/2.4.10 (Debian) Server at compbio.charite.de Port 80