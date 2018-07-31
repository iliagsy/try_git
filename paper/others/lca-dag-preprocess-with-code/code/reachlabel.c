/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
/** <!--********************************************************************-->
 *
 * @file preprocess_graph.c
 *
 * prefix: TFRCH
 *
 * description: We label vertices for answering reachability queries in this files. The
 * labeling scheme is based on the paper "Dual Labeling: Answering Graph Reachability
 * Queries in Constant Time" by Haixun Wang et. al. that appeared in ICDE '06.
 *
 *****************************************************************************/

#include "graphtypes.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "reachlabel.h"

/*
 * INFO structure
 *
 * collabel is a variable used to label vertices with numbers to index the columns of cross
 * edge based reachability matrix. This is referred to the transitive link matrix in "Dual
 * Labeling: Answering Graph Reachability Queries in Constant Time".
 *
 * totalcols is the the total number of cross edge sources.
 */

typedef struct INFO {
	int collabel;
	int totalcols;
	int lubcol;
	dynarray *csrc;
	dynarray *ctar;
	dynarray *prearr;
	elemstack **estack;
} info;

/*
 * INFO macros
 */
#define INFO_COLLABEL(n) n->collabel
#define INFO_TOTALCOLS(n) n->totalcols
#define INFO_LUBCOL(n) n->lubcol
#define INFO_CSRC(n) n->csrc
#define INFO_CTAR(n) n->ctar
#define INFO_PREARR(n) n->prearr
#define INFO_ESTACK(n) n->estack


/*
 * INFO functions
 */

static info *MakeInfo( void)
{
	info *result;
	result = (info *) malloc( sizeof( info));
	INFO_COLLABEL( result) = 0;
	INFO_TOTALCOLS( result) = 0;
	INFO_LUBCOL( result) = 0;
	INFO_CSRC( result) = NULL;
	INFO_CTAR( result) = NULL;
	INFO_PREARR (result) = NULL;
	INFO_ESTACK( result) = NULL;
	return result;
}

static info *FreeInfo( info *info)

{
	free( info);
	info = NULL;

	return info;
}


/** <!--********************************************************************-->
 *
 * @fn void RCHtravVertex( vertex *v, info *arg_info)
 *
 *   @brief
 *   We walk through the dependency graph here. If the node has not
 *   been visited i.e. its pre is 0, we update the pre of
 *   the node. Then, we check the subs (subtypes) of the def and if
 *   they are not visited, we visit them.
 *
 *   @param v
 *   @param arg_info
 *
 *   @return
 *
 *****************************************************************************/

void RCHtravVertex( vertex *v, info *arg_info)
{

	edges *children, *parents;

	/*
	 * Assign non-tree labels for reachability now
	 */

	parents = VERTEX_PARENTS(v);

	int pop = 0, i, xpre = 0, idx;

	/*
	 * If the current vertex has an incoming cross edges, then we push that vertex onto the
	 * stack and set a flag to pop at the end of this function.
	 */

	while( parents != NULL){

		if( EDGES_EDGETYPE( parents) == edgecross){

			elem *e = (elem *) malloc( sizeof( elem));
			ELEM_DATA(e) = NULL;

			/*
			 * TODO: The following loop "may" be unnecessary. Instead we can use a
			 * variable like we use for collabel (see below).
			 */

			for( i = 0; i < DYNARRAY_TOTALELEMS( INFO_CTAR( arg_info)); i++){

				if( VERTEX_PRE(v) ==
						ELEM_IDX( DYNARRAY_ELEMS( INFO_CTAR( arg_info))[i])){
					ELEM_IDX(e) = i+1;
				}

			}

			pushElemstack( INFO_ESTACK( arg_info), e);
			pop = 1;
			break;

		}

		parents = EDGES_NEXT(parents);

	}

	/*
	 * We maintain a variable called xpre which is the preorder number of a vertex that has a
	 * preorder number higher than v and is a cross-edge source.
	 */

	if( INFO_COLLABEL( arg_info) < INFO_TOTALCOLS( arg_info)){
		idx = INFO_COLLABEL(arg_info);
	}
	else {
		idx = INFO_TOTALCOLS( arg_info) - 1;
	}
	xpre = ELEM_IDX( DYNARRAY_ELEMS( INFO_CSRC( arg_info))[idx]);

	if (VERTEX_PRE(v) == xpre+1){
		VERTEX_REACHCOLA(v) = ++INFO_COLLABEL( arg_info);
	} else {
		VERTEX_REACHCOLA(v) = INFO_COLLABEL( arg_info);
	}

	VERTEX_ISRCHCOLAMARKED(v) = 1;

	VERTEX_LUBCOL (v) = VERTEX_REACHCOLA(v);

	if (VERTEX_REACHCOLA(v) >= INFO_TOTALCOLS( arg_info)){
		VERTEX_REACHCOLA(v) = -1;
	}

	/*
	 * Call the children recursively
	 */

	children = VERTEX_CHILDREN(v);

	while( children != NULL){

		if( EDGES_EDGETYPE( children) == edgetree){
			RCHtravVertex( EDGES_TARGET( children), arg_info);
		}

		children = EDGES_NEXT( children);
	}


	/*
	 * Now we update the vertices with the cluster id. These clusters are sets of
	 * cross-edge-free regions.
	 */

	if( *(INFO_ESTACK( arg_info)) != NULL){

		if( ELEMSTACK_CURR( *(INFO_ESTACK(arg_info))) != NULL){
			VERTEX_ROW (v) = ELEM_IDX( ELEMSTACK_CURR(*(INFO_ESTACK( arg_info))));
			VERTEX_ISROWMARKED (v) = 1;
		}

	}

	/*
	 * Pop the vertex that is the root of the cluster now.
	 */

	if( pop == 1){
		freeElem (popElemstack (INFO_ESTACK( arg_info)));
	}

}



/** <!--********************************************************************-->
 *
 * @fn node *TFPPGdoPreprocessTFGraph( node *syntax_tree)
 *
 *   @brief  Inits the traversal for this phase
 *
 *   @param syntax_tree
 *   @return
 *
 *****************************************************************************/

void RCHdoReachabilityAnalysis( dag *g)
{
	info *arg_info;
	arg_info = MakeInfo();

	compinfo *ci;

	vertex *v_premax;
	elem *e;
	int premax,i;
	vertex *v;

	ci = DAG_INFO(g);

	if( ci != NULL && COMPINFO_TLTABLE( ci) != NULL){

		INFO_TOTALCOLS(arg_info) = DYNARRAY_TOTALELEMS( COMPINFO_CSRC( ci));
		INFO_CSRC(arg_info) = COMPINFO_CSRC( ci );
		INFO_CTAR(arg_info) = COMPINFO_CTAR( ci );
		INFO_ESTACK( arg_info) = (elemstack **) malloc( sizeof( elemstack *));
		*INFO_ESTACK( arg_info) = NULL;
		INFO_COLLABEL( arg_info) = 0;
		INFO_PREARR (arg_info) = COMPINFO_PREARR (ci);

		RCHtravVertex( DAG_TOP (g), arg_info);

		for (i=0;i<DYNARRAY_TOTALELEMS(INFO_PREARR(arg_info)); i++){
			v = (vertex *) ELEM_DATA(DYNARRAY_ELEMS_POS(INFO_PREARR(arg_info),i));
			premax = VERTEX_PREMAX (v);
			if (premax > DYNARRAY_TOTALELEMS(INFO_PREARR (arg_info))) {
				//premax --;
				VERTEX_REACHCOLB (v) = -1;
			} else {
				e = DYNARRAY_ELEMS_POS
						(INFO_PREARR(arg_info), (premax - 1));
				v_premax = (vertex *) ELEM_DATA (e);
				VERTEX_REACHCOLB (v) = VERTEX_REACHCOLA (v_premax);
			}
			VERTEX_ISRCHCOLBMARKED (v) = 1;
/*
			printf ("%d -- %d [%d, %d] %d\n",
					v->pre, v->premax, v->reachcola, v-> reachcolb, v->lubcol);
*/

		}



	}

	arg_info = FreeInfo(arg_info);
}
