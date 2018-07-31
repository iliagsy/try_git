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
 * @file classifyedges.c
 *
 * prefix: TFCTR
 *
 * description: In this file, we first classify the edges in subtyping hierarchy DAG.
 * Then, we build a transitive link table for each DAG under consideration.
 *
 * The transitive link table holds information about which cross edge sources
 * reach what cross edge target vertices. More details can be found in the paper
 * on reachability analysis using dual labeling.
 *
 *****************************************************************************/

#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphtypes.h"
#include "ctransitive.h"
#include "reachhelper.h"

/*
 * INFO structure
 */
typedef struct INFO {
	dynarray *tltable;
	dynarray *arrX;
	dynarray *arrY;
}info;

/*
 * INFO macros
 */
#define INFO_TLTABLE(n) n->tltable

/*
 * INFO functions
 */
static info *MakeInfo( void)
{
	info *result;

	result = (info *) malloc( sizeof( info));
	INFO_TLTABLE(result) = NULL;

	return result;

}

static info *FreeInfo( info *info)
{

	free (info);
	info = NULL;

	return info;
}

/** <!--********************************************************************-->
 *
 * @fn void CTRtravVertex (vertex *v, info *arg_info)
 *
 *   @brief
 *   We walk through the dependency graph here.
 *   If the edge has not been classified, we classify the edge based
 *   on the pre and post order numbers of the source and target
 *   vertices for the edge.
 *
 *   @param v
 *   @param arg_info
 *
 *   @return
 *
 *****************************************************************************/

void CTRtravVertex( vertex *v, info *arg_info)
{

	edges *children, *parents;
	int pre_parent, pre_child, post_parent, post_child, premax_child;

	children = VERTEX_CHILDREN (v);
	pre_parent = VERTEX_PRE (v);
	post_parent = VERTEX_POST (v);

	while( children != NULL){

		if( !EDGES_WASCLASSIFIED( children)){

			/* Tree edges have already been classified during depth first walk of the
			 * graph.
			 */

			pre_child = VERTEX_PRE( EDGES_TARGET( children));
			premax_child = VERTEX_PREMAX( EDGES_TARGET( children));
			post_child = VERTEX_POST( EDGES_TARGET( children));

			if( pre_parent < pre_child && post_child < post_parent){

				/*
				 * This is a forward edge. Since back and forward edges are
				 * disallowed, throw an error here.
				 *
				 * TODO: Dont throw an error. Instead, ignore the forward edge and show
				 * a warning.
				 */

				printf ( "Forward edge found in subtyping hierarchy between %s and %s\n",
						VERTEX_LABEL(v), VERTEX_LABEL(EDGES_TARGET(children)));
				exit(-1);

			} else if( pre_child < pre_parent && post_parent < post_child){

				/*
				 * This is a back edge. Since back and forward edges are
				 * disallowed, throw an error here.
				 */

				printf ( "Back edge found in subtyping hierarchy\n");
				exit(-1);

			} else if(pre_child < pre_parent && post_child < post_parent){

				/*
				 * This must be a cross edge. Add this to the transitive
				 * link table
				 */

				EDGES_EDGETYPE( children) = edgecross;

				/*
				 * Set the parent relationship to be a cross edge as well.
				 * This will be used in the non-tree labeling
				 */

				parents = VERTEX_PARENTS( EDGES_TARGET( children));

				while( parents != NULL){
					if( EDGES_TARGET( parents) == v){
						EDGES_EDGETYPE( parents) = edgecross;
					}
					parents = EDGES_NEXT( parents);
				}

				/*
				 * Now that we have discovered a cross edge, we must add it
				 * to the transitive link table.
				 */

				if( INFO_TLTABLE( arg_info) == NULL){
					INFO_TLTABLE( arg_info) = makeDynarray ();
				}

				/*
				 * The transitive link table is actually a list that consists of three
				 * integers a,b and c in the form a->[b,c) which signifies that a vertex
				 * with a preorder number of "a" reaches another vertex with a preorder
				 * number "b" and "c" is the maximum preorder number of the children of "b"
				 * increased by 1. Hence, the open interval ")".
				 */

				elem *e = (elem *) malloc( sizeof( elem));
				ELEM_DATA(e) = malloc( 2*sizeof( int));
				ELEM_IDX(e) = pre_parent;
				*((int *)ELEM_DATA(e)) = pre_child;
				*((int *)ELEM_DATA(e) + 1) = premax_child;

				addToArray( INFO_TLTABLE( arg_info),e);

			} else {

				printf ("Unclassifiable edge found in subtyping hierarchy\n");
				exit(-1);

			}

			EDGES_WASCLASSIFIED( children) = 1;

		} else {

			CTRtravVertex (EDGES_TARGET (children), arg_info);

		}

		children = EDGES_NEXT(children);

	}

}


/** <!--********************************************************************-->
 *
 * @fn void CTRdoClassifyEdges( dag* g)
 *
 *   @brief  Inits the traversal for this phase
 *
 *   @param g
 *   @return
 *
 *****************************************************************************/
void CTRdoCrossClosure (dag *g)
{
	info *arg_info = MakeInfo();

	compinfo *ci;

	CTRtravVertex( DAG_TOP (g), arg_info);

	/*
	 * Do the following if we have at least one cross edge in the DAG.
	 */

	if( INFO_TLTABLE (arg_info) != NULL){
		/*
		 * We maintain a list of all cross edge sources and all cross edge
		 * targets in a DAG.
		 */
		ci = DAG_INFO (g);
		setSrcTarArrays( INFO_TLTABLE(arg_info),
				&( COMPINFO_CSRC( ci)),
				&( COMPINFO_CTAR( ci)));

		/*
		 * For each cross edge source, compute all the source edge targets that
		 * it can potentially reach transitively. This will add a few more
		 * entries in the transitive link table.
		 */


		if( DYNARRAY_TOTALELEMS( INFO_TLTABLE( arg_info)) > 0){
			COMPINFO_TLTABLE(ci) =
					buildTransitiveLinkTable( INFO_TLTABLE( arg_info),
					COMPINFO_CSRC( ci), COMPINFO_CTAR( ci), COMPINFO_PREARR(ci));

			COMPINFO_TLC( ci)= computeTLCMatrix( COMPINFO_TLTABLE( ci),
					COMPINFO_CSRC( ci),
					COMPINFO_CTAR( ci));
		}

		//freeDynarray( INFO_TLTABLE( arg_info));

	}

	arg_info = FreeInfo(arg_info);

}


