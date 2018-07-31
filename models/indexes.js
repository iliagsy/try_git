db.hpo_tree.createIndex({"hpo":1}, {"unique": true, "background": true})
db.hpo_anc.createIndex({"hpo":1}, {"unique": true, "background": true})
db.disease_full_hpo.createIndex({"disease":1}, {"unique": true, "background": true})