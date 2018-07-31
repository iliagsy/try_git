### phenomizer匹配分数的算法实现

论文: [Clinical Diagnostics in Human Genetics with Semantic Similarity Searches in Ontologies](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2756558/)

#### 数据

* [phenotype_annotation.tab](http://compbio.charite.de/jenkins/job/hpo.annotations/lastStableBuild/)

* [hp.obo](http://purl.obolibrary.org/obo/hp.obo)

#### 效果评估

* 44疾病分别生成100模拟病人，跑phenomizer，统计正确结果的排名（最好时：1）

* p-value相同时，用raw similarity score排序

* [44种复杂疾病的表型&频率](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2756558/bin/mmc1.pdf)

##### 索引
* `db.hpo_mica.createIndex({hpo1:1, hpo2:1}, background=true)`
* `db.hpo_sim.createIndex({hpo1:1, hpo2:1}, background=true)`
