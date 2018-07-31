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
 * @file lub.c
 *
 * prefix: TFPLB
 *
 * description: This file calls functions to preprocess the tpye hierarchy graph
 * for answering least upper bound queries. This is done with the aid of a
 * compiler pass.
 *
 *****************************************************************************/
#include "dfwalk.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphtypes.h"
#include "dfwalk.h"
#include "tfprintutils.h"
#include "lubtree.h"
#include "lubcross.h"
#include "binheap.h"
#include "query.h"

int GINisReachable( vertex *n1, vertex *n2, compinfo *ci){

	int result = 0;
	int reachtree = 0, reachcross = 0;
	int cola, colb, row;
	int reaching_csrc_after_pre;
	int reaching_csrc_after_premax;

	if(VERTEX_PRE(n1) == VERTEX_PRE(n2)) return 1;

	if( VERTEX_POST( n1) > VERTEX_POST( n2)){

		if( ( VERTEX_PRE( n2) >= VERTEX_PRE( n1) &&
				VERTEX_PRE( n2) < VERTEX_PREMAX( n1))){

			reachtree = 1;

		}

		cola = VERTEX_REACHCOLA( n1);
		colb = VERTEX_REACHCOLB( n1);
		row = VERTEX_ROW( n2)-1;

/*		printf("n1 = %d, n2 = %d\n", n1->pre, n2->pre);
		printf ("cola = %d, colb = %d, row = %d\n", cola, colb, row);
		printTLCmatrix (COMPINFO_TLC(ci), COMPINFO_CSRC(ci), COMPINFO_CTAR(ci));*/

		if( !VERTEX_ISROWMARKED( n2)){

			reachcross = 0;

		} else {

			if( !VERTEX_ISRCHCOLAMARKED( n1) || cola == -1){
				reaching_csrc_after_pre = 0;
			} else {
				reaching_csrc_after_pre = getMatrixValue( COMPINFO_TLC( ci),
						cola, row);
			}

			if( !VERTEX_ISRCHCOLBMARKED( n1) || colb == -1){
				reaching_csrc_after_premax = 0;
			} else {
				reaching_csrc_after_premax = getMatrixValue( COMPINFO_TLC( ci),
						colb, row);
			}

			if( reaching_csrc_after_pre - reaching_csrc_after_premax > 0){
				reachcross = 1;
			} else {
				reachcross = 0;
			}

		}

		if( reachtree || reachcross){
			result = 1;
			//printf ("%d reaches %d\n", n1->pre, n2->pre);
		}

	}

	return result;

}
/*

static void GINreorderVerticesInDAG( vertex *n1, vertex *n2){

	vertex *temp;


	 * Check whether n2 can reach n1 through a tree or cross edge based path.


	if( VERTEX_POST(n1) > VERTEX_POST(n2)){
		temp = n1;
		n1 = n2;
		n2 = temp;
	}

}
*/

vertex *GINlcaFromNodes( vertex *v1, vertex *v2, compinfo *ci){

	matrix *pcpt_matrix, *pcpc_matrix;
	int pcpt_col, pcpt_row, pcpc_row, pcpc_col;
	elem *pcpt_elem;
	int lower_pcpt_pre, upper_pcpt_pre;
	vertex *lower_pcpt_node, *upper_pcpt_node;
	int pcpc_plca_pre;
	vertex *sptree_plca, *pcpt_plca1, *pcpt_plca2, *pcpc_plca;
	vertex *n1, *n2;

	if(VERTEX_POST (v1) > VERTEX_POST(v2)){
		n1=v2; n2=v1;
	} else{
		n1=v1; n2=v2;
	}
	/*
	 * Get the pcpt_matrix and pcpc_matrix. We will need this later on to find the
	 * pcpt_plca1, pcpt_plca2 and pcpc_plca.
	 */

	pcpt_matrix = LUBINFO_PCPTMAT( COMPINFO_LUB( ci));
	pcpc_matrix = LUBINFO_PCPCMAT( COMPINFO_LUB( ci));

	/*
	 * Potentially reorder vertices to correctly index the matrices.
	 */

	//GINreorderVerticesInDAG( &n1, &n2);
	sptree_plca = LUBtreeLCAfromNodes( n1, n2, ci);

	if (ci->tltable == NULL) return sptree_plca;

	pcpt_col = VERTEX_LUBCOL( n2);
	pcpt_row = VERTEX_ROW( n1)-1;

	pcpc_col = VERTEX_ROW( n2);
	pcpc_row = VERTEX_ROW( n1);

/*	printf("n1 = %d, n2 = %d\ntc_row = %d tc_col = %d\ncc_row = %d cc_col = %d\n",
			n1->pre, n2->pre, pcpt_row, pcpt_col, pcpc_row-1, pcpc_col-1);*/

	/*
	 * We have the row and column indices now, we can get the pcpt-matrix element
	 * for that row and column.
	 */

	if(pcpt_col == -1 || pcpt_row == -1) {
		lower_pcpt_pre = -1;
/*		lower_pcpt_node = ( vertex *) ELEM_DATA(
							DYNARRAY_ELEMS_POS( COMPINFO_PREARR( ci), 0));*/
		upper_pcpt_pre = -1;
/*		if ( COMPINFO_CSRC(ci) != NULL) {
			upper_pcpt_pre = ELEM_IDX( DYNARRAY_ELEMS( COMPINFO_CSRC(ci))[0]);
		}*/

	} else {
		pcpt_elem = getMatrixElem( pcpt_matrix, pcpt_row, pcpt_col);

		lower_pcpt_pre = ( (int *) ELEM_DATA( pcpt_elem))[0];

		upper_pcpt_pre = ( (int *) ELEM_DATA( pcpt_elem))[1];
	}


	if ( lower_pcpt_pre == -1){

		lower_pcpt_node = ( vertex *) ELEM_DATA(
				DYNARRAY_ELEMS_POS( COMPINFO_PREARR( ci), 0));

	} else {

		lower_pcpt_node = ( vertex *) ELEM_DATA(
				DYNARRAY_ELEMS_POS( COMPINFO_PREARR( ci), lower_pcpt_pre - 1));

	}

	if ( upper_pcpt_pre == -1){

		upper_pcpt_node = ( vertex *) ELEM_DATA(
				DYNARRAY_ELEMS_POS( COMPINFO_PREARR( ci), 0));

	} else {

		upper_pcpt_node = ( vertex *) ELEM_DATA(
				DYNARRAY_ELEMS_POS( COMPINFO_PREARR( ci), upper_pcpt_pre - 1));

	}

	/*
	 * now, we have two cross edge sources one with a topological number less than
	 * n1 and another with a preorder number greater than n1 and another with a
	 * preorder number less than n1 reaching n2 through cross edges. Its time to
	 * calculate two more potential LCAs based on this information.
	 */

	pcpt_plca1 = LUBtreeLCAfromNodes( lower_pcpt_node, n2, ci);
	pcpt_plca2 = LUBtreeLCAfromNodes( n2, upper_pcpt_node, ci);

	if(pcpc_matrix != NULL){
		if (pcpc_row==0 || pcpc_col==0) {
			pcpc_plca = NULL;
		} else {
			pcpc_plca_pre = getMatrixValue( pcpc_matrix, pcpc_row-1, pcpc_col-1);
			pcpc_plca = ( vertex *) ELEM_DATA(
					DYNARRAY_ELEMS_POS( COMPINFO_PREARR( ci), pcpc_plca_pre - 1));
		}
	} else {
		pcpc_plca =NULL;
	}

	/*
	 * Now we just need to find the plca that has the lowest topological number
	 * amongst sptree_plca, pcpt_plca1, pcpt_plca2 and pcpc_plca.
	 */

	vertex *n[4] = { sptree_plca, pcpt_plca1, pcpt_plca2, pcpc_plca};
	int i;
	vertex *result = sptree_plca;


	for( i = 0 ; i < 4; i++){
		if( n[i]!=NULL){
			//printf( "%d:%d\n", i, n[i]->pre);
		}
	}


	for( i = 1 ; i < 4; i++){
		if( n[i]!=NULL && VERTEX_TOPO( n[i]) > VERTEX_TOPO( result)){
			result = n[i];
		}
	}

	return result;

}
