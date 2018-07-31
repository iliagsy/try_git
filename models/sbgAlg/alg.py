# coding: utf-8
from pymongo import MongoClient
import random


from tool import Tool


'''
输入DAG
1.由一组节点生成子图（最低共同祖先）
2.两个DAG的节点交集
3.交集节点的IC之和 / 并集节点的IC之和
'''


class Alg(object):
    def __init__(self, db):
        self.db = db
        self.tool = Tool(db)
        self.query = None
        self._query_dag = None

    @property
    def query_dag(self):
        if self.query is None:
            return None
        if self._query_dag is not None:
            return self._query_dag
        self._query_dag = self.get_subgraph(self.query)
        return self._query_dag

    def lca_multi_nodes_w_subgraph(self, hpos):
        '''get LCA of multiple nodes recursively, each time calculating 2 nodes'''
        if len(hpos) == 0:
            return None
        hpos_ = hpos[:]
        subgraph = set(hpos)
        while len(hpos_) > 1:
            hpos__ = []
            while len(hpos_) > 1:
                hpo1 = hpos_.pop()
                hpo2 = hpos_.pop()
                lca, nodes = self.lca_w_subgraph(hpo1, hpo2)
                subgraph |= set(nodes)
                hpos__.append(lca)
            if len(hpos_) > 0:
                hpo = hpos_.pop()
                hpos__.append(hpo)
            hpos_ = hpos__[:]
        return hpos_[0], list(subgraph)

    def _shortest_path_lca_two_desc(self, lca, hpo1, hpo2):
        pass

    def lca_w_subgraph(self, hpo1, hpo2):
        return self.tool.MICA_w_subgraph(hpo1, hpo2)

    def get_subgraph(self, hpos):
        '''
        :return: 以输入hpo为“边界”的dag的节点
        # 限制：len(hpos) >= 2
        '''
        if len(hpos) == 0:
            return []
        lca, subgraph = self.lca_multi_nodes_w_subgraph(hpos)
        if lca is None:
            raise Exception("no lca in hpo tree found for {}".format(hpos))
        return subgraph

    def sub_dag_sim(self, nodes1, nodes2):
        # TODO: test
        intersection = list(set(nodes1) & set(nodes2))
        if len(intersection) == 0:
            return 0.0
        itsc_IC = sum(map(lambda n: self.tool.IC(n), intersection))
        total_IC = sum(map(lambda n: self.tool.IC(n), set(nodes1) | set(nodes2)))
        return itsc_IC / total_IC

    def query_gene_sim(self, query, gene_hpos, **kw):
        '''
        :param query: a list of hpos
        :param gene_hpos: a list of hpos
        '''
        # TODO: test
        q_dag = self.query_dag or self.get_subgraph(query)
        d_dag = None
        gene = kw.get("gene")
        if gene:
            d_dag = (self.db['gene_subgraph'].find_one({"gene": gene}) or {}).get(u'subgraph')
        if d_dag is None:
            d_dag = self.get_subgraph(gene_hpos)
        return self.sub_dag_sim(q_dag, d_dag)

    def _get_hpo_by_name(self, name):
        return (self.db['knowledge_chpo'].find_one({"name": name}) or {}).get(u'hpoId')


class Pre(object):
    def __init__(self, db):
        self.db = db
        self.alg = Alg(db)

    def prep_disease_subgraph(self):
        all_diseases = self.db['hpo_disease'].distinct('disease')
        for g in all_diseases:
            hpos = db['hpo_disease'].find({"disease": g}).distinct("hpo")
            subg = list(self.alg.get_subgraph(hpos))
            if len(subg) == 0:
                continue
            self.db['disease_subgraph'].update({"disease":g}, {
                "$set": {
                    "subgraph": subg
                }
            }, upsert=True)
            print 'storing sub_g for {}'.format(g)


if __name__ == '__main__':
    db = MongoClient('localhost', 27017)['phenomizer']
    '''2'''
    pre = Pre(db)
    pre.prep_disease_subgraph()
