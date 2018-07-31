from argparse import ArgumentParser
import datetime
import json
import numpy as np
import matplotlib.pyplot as plt
from pymongo import MongoClient


######################
bm_res = 'benchmark_result'
##########################


def plot(res, fn=''):
    '''
    :param res: list of ranks-of-correct-result on benchmark data
    '''
    res = map(lambda x: min(x, 200), res)
    res.sort()

    sr = np.array(res)
    fig = plt.figure()
    plt.hist(sr, bins=100, cumulative=True)

    plt.show()
    # fig.savefig("eval/{}-{}.jpg".format(fn, datetime.datetime.now()))


def plot_top_match(res, fn=""):
    res = map(lambda x: min(x, 200), res)
    res.sort()

    sr = np.array(res)
    fig = plt.figure()
    plt.boxplot(sr)

    plt.show()
    # fig.savefig("eval/{}-{}.jpg".format(fn, datetime.datetime.now()))


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument('-l', '--log')
    parsed = arg_parser.parse_args()

    match, rank = parse_file('log/{}.log'.format(parsed.log))
    plot_top_match(match, 'top_match.{}'.format(parsed.log))
    plot(rank, 'result_rank.{}'.format(parsed.log))


def parse_file(filepath):
    with open(filepath) as fh:
        lines = fh.readlines()
    match = []
    rank = []
    for l in lines:
        if l.startswith("rank"):
            rank.append(int(l.split()[-1])) 
        elif l.startswith("disease"):   
            match.append(float(l.split()[-1]))
    return match, rank


if __name__ == "__main__":
    main()