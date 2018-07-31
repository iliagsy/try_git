/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#include "main.h"
#include "graphtypes.h"
#include "dag.h"
#include "query.h"
#include "tfprintutils.h"
#include "dynarray.h"
#include "dynelem.h"
#include "dynmatrix.h"
#include "topo.h"
#include "dfwalk.h"
#include "classes.h"
#include <time.h>
#include <string.h>
#include "timing.h"
#include "classes.h"

void print_interlude(){
	printf("|-----------------------|------------------------------------------|--------------------------------------|\n");
}

void print_prelude(){
    print_interlude();
    printf("| Verts\t c_rat\t\t| ppc_cc (ms)\t ppc_fc (ms)\t ppc_rat   | qt_cc (us)\t qt_fc (us)\t qt_rat   |\n");
    print_interlude();
}

void print_res(tr* avg_res) {
    printf("| %d\t %.2f\t\t| %.2f ms\t %.2f ms\t %.2f\t   | %.2f us\t %.2f us\t %.2f     |\n",
		    avg_res->total, avg_res->cratio,
		    avg_res->ppt_adaptive*pow(10,3), avg_res->ppt_greedy*pow(10,3),
		    avg_res->ppt_ratio,
		    avg_res->qt_adaptive*pow(10,6), avg_res->qt_greedy*pow(10,6),
		    avg_res->qt_ratio);
}


void sanitizeDAG (dag *g) {

	int i;
	if(DAG_INFO(g) == NULL) return;
	dynarray *prearr = COMPINFO_PREARR (DAG_INFO (g));
	int total = DYNARRAY_TOTALELEMS (prearr);
	elem *e;
	vertex *v;
	edges *children;

	for (i=0; i<total; i++) {
		e = DYNARRAY_ELEMS_POS (prearr, i);
		v = (vertex *) ELEM_DATA (e);
		VERTEX_PRE (v) = 0;
		VERTEX_PREMAX (v) = 0;
		VERTEX_POST (v) = 0;
		VERTEX_DEPTH (v) = 0;
		VERTEX_TOPO (v) = 0;
		VERTEX_NUMTOPOVISITS (v) = 0;
		children = VERTEX_CHILDREN (v);
		while (children != NULL) {
			EDGES_EDGETYPE (children) = edgedefault;
			EDGES_WASCLASSIFIED (children) = false;
			children = EDGES_NEXT (children);
		}
	}
	freeDynarrayShallow (prearr);
	freevertices (g->info->topolist);

	COMPINFO_PREARR ( DAG_INFO (g)) = NULL;
	g->info->topolist = NULL;
	g->dirty = 1;
}

#define DRYRUNS -1

tr* test_iterative(dag* g, int iter){
    tr* avg_res = (tr*) malloc(sizeof(tr));
    tr *result;
    int i;
    init_result(avg_res);
    for (i = DRYRUNS; i<iter; i++) {
	    sanitizeDAG (g);
	    if(i<0) {time_difference (g);continue;}
	    result = time_difference (g);
	    avg_res->total = result->total;
	    avg_res->cratio += result->cratio;
	    avg_res->ppt_adaptive += result->ppt_adaptive;
	    avg_res->ppt_greedy += result->ppt_greedy;
	    avg_res->ppt_ratio += result->ppt_ratio;
	    avg_res->qt_adaptive += result->qt_adaptive;
	    avg_res->qt_greedy += result->qt_greedy;
	    avg_res->qt_ratio += result->qt_ratio;
    }
    avg_res->cratio/=(double)iter;
    avg_res->ppt_adaptive/=(double)iter;
    avg_res->ppt_greedy/=(double)iter;
    avg_res->ppt_ratio/=(double)iter;
    avg_res->qt_adaptive/=(double)iter;
    avg_res->qt_greedy/=(double)iter;
    avg_res->qt_ratio/=(double)iter;
    return avg_res;
}

tr* test(dag* g){
    tr* avg_res = (tr*) malloc(sizeof(tr));
    tr *result;
    init_result(avg_res);
    result = time_difference (g);
    avg_res->total = result->total;
    avg_res->cratio += result->cratio;
    avg_res->ppt_adaptive += result->ppt_adaptive;
    avg_res->ppt_greedy += result->ppt_greedy;
    avg_res->ppt_ratio += result->ppt_ratio;
    avg_res->qt_adaptive += result->qt_adaptive;
    avg_res->qt_greedy += result->qt_greedy;
    avg_res->qt_ratio += result->qt_ratio;
    return avg_res;
}


dag *makeTree (int n){
	int i;
	char str[15];
	dag *g = makedag ();
	dynarray *verts = makeDynarray ();
	vertex *v, *src, *sink;
	elem *e;
	int total = 1<<n;
	for (i=1; i< total; i++) {
		sprintf(str, "%d", i);
		v = DAGaddVertex (g, str);
		str[0]='\0';
		e = makeElem ();
		ELEM_IDX(e) = 0;
		ELEM_DATA(e) = v;
		addToArray(verts, e);
	}

	for (i=0; i< total-1; i++) {
		e = DYNARRAY_ELEMS_POS (verts, i);
		if ( ELEM_IDX(e) == 1) continue;
		ELEM_IDX(e) = 1;
		src = (vertex *) ELEM_DATA(e);

		if(i==0) DAG_TOP(g) = src;

		if(2*i+1 <= (total-2)){
			e = DYNARRAY_ELEMS_POS (verts, 2*i+1);
			//ELEM_IDX (e) = 1;
			sink = (vertex *) ELEM_DATA (e);
			DAGaddEdge (g, src, sink);
		}

		if(2*i+2 <= (total-2)){
			e = DYNARRAY_ELEMS_POS (verts, 2*i+2);
			//ELEM_IDX (e) = 1;
			sink = (vertex *) ELEM_DATA (e);
			DAGaddEdge (g, src, sink);
		}
	}

	return g;
}

void addCrossEdges (dag *g, float c_ratio) {
	dynarray *prearr = COMPINFO_PREARR (DAG_INFO (g));
	int i, j, r_src, r_tar, total = DYNARRAY_TOTALELEMS (prearr);
	int num_csrc = (c_ratio * (float)total);
	vertex *csrc, *ctar;
	elem *e;

	int num_tries = 30000;
	srand(time(NULL));
	/*pick a cross edge source*/
	for (i=0; i<num_csrc; i++) {
		r_src = rand() % total;
		e = DYNARRAY_ELEMS_POS (prearr, r_src);
		csrc = (vertex *) ELEM_DATA (e);
		j=0;
		do {
			j++;
			//srand(time(NULL));
			r_tar = rand() % total;
			e = DYNARRAY_ELEMS_POS (prearr, r_tar);
			ctar = (vertex *) ELEM_DATA (e);
		} while (j <= num_tries
				&& !(VERTEX_PRE (csrc) > VERTEX_PRE (ctar)
						&& VERTEX_POST (csrc) > VERTEX_POST (ctar)));

		if ((VERTEX_PRE (csrc) > VERTEX_PRE (ctar)
				&& VERTEX_POST (csrc) > VERTEX_POST (ctar))) {
			//printf ("%d, %d\n", VERTEX_PRE (csrc), VERTEX_PRE (ctar));
			DAGaddEdge (g, csrc, ctar);
		}
	}
}

void test_spectrum (int iter) {
	dag *g;
	tr* avg_res;
	float rat;
	int depth;
	double cumul = 0.0;
	print_prelude();
	for(depth = 4; depth <=8; depth ++ ){
		for (rat = 0.1; rat <= 0.9; rat +=0.1) {
			cumul=0.0;
			g = makeTree (depth);
			TOPdoTopoSort(g);
			DFWdoDFWalk(g);
			addCrossEdges (g, rat);
			avg_res = test_iterative(g, iter);
			print_res(avg_res);
		}
	print_interlude();
	}

}

#define MAXNUM 255

unsigned char mask[] = {128, 64, 32, 16, 8, 4, 2, 1};

void psetEdge(dag *g, int num){
	if (num > MAXNUM) {
		printf("cannot verify bit settings numbers greater than MAXNUM");
		exit(-1);
	}
	int i;
	char str1[15], *str2;
	sprintf(str1, "%d", num);
	for(i=0; i<log2(MAXNUM + 1); i++){
		if ((num & mask[i]) != 0){
			str2 = (char *) malloc(sizeof(char)*15);
			sprintf(str2, "%d", num - mask[i]);
			//printf("%s -> %s\n", str1, str2);
			createEdgeInHierarchy (g, str1, str2);
			psetEdge (g, num - mask[i]);
		}
	}
}

dag *makePSetLat(int n){
	dag *g = makedag();
	unsigned int max = (1<<n) - 1;
	char top_label[15];
	sprintf(top_label, "%d", max);
	psetEdge(g, max);
	DAG_TOP(g) =  getVertexFromLabel (g, top_label);
	return g;
}

void test_PSetLat(int iter) {
	dag *g;
	tr *result;
	int i;
	print_prelude();
	for(i=2;i<=8;i++) {
		g = makePSetLat(i);
		//result = test(g);
		result=test_iterative(g, iter);
		print_res(result);
/*		printf("%d\t| %.2f\t| %.2f ms\t| %.2f ms\t| %.2f\t\t| %.2f us\t| %.2f us\t| %.2f\n",
				result->total, result->cratio,
				result->ppt_adaptive*pow(10,3), result->ppt_greedy*pow(10,3),
				result->ppt_ratio,
				result->qt_adaptive*pow(10,6), result->qt_greedy*pow(10,6),
				result->qt_ratio);*/
	}
	print_interlude();
}

void test_classhierar(int iter, char** filenames, int total_hierars) {
	dag *g;
	tr* result;
	int i;
	print_prelude();
	for(i=0; i<total_hierars; i++) {
		g = getClassHierarchy (filenames[i]);
		//result = test(g);
		result=test_iterative(g, iter);
		print_res(result);
/*		printf("%d\t| %.2f\t| %.2f ms\t| %.2f ms\t| %.2f\t\t| %.2f us\t| %.2f us\t| %.2f\n",
				result->total, result->cratio,
				result->ppt_adaptive*pow(10,3), result->ppt_greedy*pow(10,3),
				result->ppt_ratio,
				result->qt_adaptive*pow(10,6), result->qt_greedy*pow(10,6),
				result->qt_ratio);*/
	}
	print_interlude();
}

int main(int argc, char **argv) {
    int iter = 1;
    int totalfiles=6;
	char* allfiles[] =
	{
			"./benchmark/apachetomcat.html",
			"./benchmark/batik.html",
			"./benchmark/eclipse.html",
			"./benchmark/jython.html",
			"./benchmark/pmd.html",
			"./benchmark/xalan.html"
	};
	char* filename[10];
	if(argc < 2) {
		printf("Must specify at least one argument, try --help option\n");
		exit(0);
	}
	if(strcmp(argv[1],"--help")==0){
		printf("\n");
		printf("This is the adpative lattice pre-processor.\n\n");
		printf("Options for usage:\n");
		printf("--all\t\t\t run all tests\n");
		printf("--class [filepath]\t pre-process class hierarchies\n");
		printf("--pset\t\t\t pre-process powerset lattices\n");
		printf("--spectrum\t\t pre-process the entire spectrum of lattices\n");
		printf("\n\n");
	}
	if(strcmp(argv[1],"--all")==0){
		printf("Running all experiments...\n");
		printf("\n\nPSet tests...\n");
		test_PSetLat(iter);

		printf("\n\nClassHierar tests...\n");
		test_classhierar(iter, allfiles, totalfiles);

		printf("\n\nSpectrum tests...\n");
		test_spectrum(iter);
	}
	if(strcmp(argv[1],"--pset")==0){
		printf("\n\nPSet tests...\n");
		test_PSetLat(iter);
	}
	if(strcmp(argv[1],"--class")==0){
		if(argc>2) {
			filename[0] = argv[2];
			printf("\n\nPerforming ClassHierar test on %s...\n", filename[0]);
			totalfiles=1;
			test_classhierar(iter, filename, totalfiles);
		} else {
			printf("\n\nClassHierar tests...\n");
			test_classhierar(iter, allfiles, totalfiles);
		}
	}
	if(strcmp(argv[1],"--spectrum")==0){
		printf("\n\nSpectrum tests...\n");
		test_spectrum(iter);
	}
	printf("\n");
	return 0;
}



