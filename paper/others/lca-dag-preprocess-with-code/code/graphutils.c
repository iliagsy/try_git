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
 * @file graphutils.c
 *
 * description: some utility functions for vertices and list of vertices
 *
 *****************************************************************************/

#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphtypes.h"
#include "graphutils.h"

bool GUvertInList( vertex *n, vertices *nl) {

	while( nl!=NULL){

		if( VERTICES_CURR(nl) == n){
			return 1;
		}

		nl = VERTICES_NEXT(nl);

	}

	return 0;

}

vertices* GUmergeLists( vertices *nla, vertices *nlb){

	vertices *nlx, *itr_nlx = NULL, *itr_nla, *itr_nlb;

	itr_nla = nla;
	itr_nlb = nlb;

	nlx = NULL;

	while( itr_nla != NULL){

		/*
		 * First check whether the nodes in nla are in nlb. If not, add the nodes to
		 * the combined list.
		 */

		if( !GUvertInList( VERTICES_CURR( nla), nlb)) {

			if( nlx == NULL){

				nlx = (vertices *) malloc( sizeof( vertices));
				itr_nlx = nlx;

			} else {

				VERTICES_NEXT( itr_nlx) =  (vertices *) malloc( sizeof( vertices));
				itr_nlx = VERTICES_NEXT( itr_nlx);

			}

			VERTICES_CURR( itr_nlx) = VERTICES_CURR( itr_nla);
			VERTICES_NEXT( itr_nlx) = NULL;

		}

		itr_nla = VERTICES_NEXT( itr_nla);

	}

	/*
	 * Now add the nodes in nlb to nlx
	 */

	itr_nlb = nlb;

	while( itr_nlb !=NULL){

		if( nlx == NULL){

			nlx = (vertices *) malloc( sizeof( vertices));
			itr_nlx = nlx;

		} else {

			VERTICES_NEXT( itr_nlx) = (vertices *) malloc( sizeof( vertices));
			itr_nlx = VERTICES_NEXT( itr_nlx);

		}

		VERTICES_CURR( itr_nlx) = VERTICES_CURR( itr_nlb);
		VERTICES_NEXT( itr_nlx) = NULL;

		itr_nlb = VERTICES_NEXT( itr_nlb);

	}

	return nlx;

}

void GUremoveEdge( vertex *src, vertex *tar){

	edges *prev_itr, *curr_itr;

	/*
	 * First remove vertices from the children list of the edge source
	 */

	prev_itr = NULL;

	curr_itr = VERTEX_CHILDREN( src);

	while( curr_itr != NULL){

		if( EDGES_TARGET(curr_itr) == tar){

			if( prev_itr == NULL){

				/*
				 * The first node in the list is a match.
				 */

				VERTEX_CHILDREN( src) = freeCurrentEdge ( VERTEX_CHILDREN( src));
				curr_itr = VERTEX_CHILDREN( src);

			} else {

				EDGES_NEXT( prev_itr) = freeCurrentEdge ( curr_itr);
				curr_itr = EDGES_NEXT( prev_itr);

			}

			continue;

		}

		prev_itr = curr_itr;
		curr_itr = EDGES_NEXT( curr_itr);

	}

	/*
	 * Now remove vetices from the lists of parents of tar
	 */

	prev_itr = NULL;

	curr_itr = VERTEX_PARENTS( tar);

	while( curr_itr != NULL){

		if( EDGES_TARGET(curr_itr) == src){

			if( prev_itr == NULL){

				/*
				 * The first node in the list is a match.
				 */

				VERTEX_PARENTS( tar) = freeCurrentEdge ( VERTEX_PARENTS( tar));
				curr_itr = VERTEX_PARENTS( src);

			} else {

				EDGES_NEXT( prev_itr) = freeCurrentEdge ( curr_itr);
				curr_itr = EDGES_NEXT( prev_itr);

			}

			continue;

		}

		prev_itr = curr_itr;
		curr_itr = EDGES_NEXT( curr_itr);

	}

}

