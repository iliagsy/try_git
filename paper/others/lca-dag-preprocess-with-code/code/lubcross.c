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
 * @file lubcross.c
 *
 * prefix: LUB
 *
 * description:
 */

#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphutils.h"
#include "graphtypes.h"
#include "tfprintutils.h"
#include "lubtree.h"
#include "lubcross.h"
#include "binheap.h"
#include "query.h"

typedef struct POSTINFO{
	int iscsrc;
	int colidx;
	vertex *vertex;
}postinfo;

#define POSTINFO_ISCSRC( n) ( (n)->iscsrc)
#define POSTINFO_COLIDX( n) ( (n)->colidx)
#define POSTINFO_VERTEX( n) ( (n)->vertex)

typedef struct TOPOINFO{
	int colidx;
	vertex *vertex;
}topoinfo;

#define TOPOINFO_COLIDX( n) ( (n)->colidx)
#define TOPOINFO_VERTEX( n) ( (n)->vertex)

typedef struct PCPCINFO{
	dynarray *csrc;
	matrix *csrcmat;
	dynarray *noncsrc;
	matrix *noncsrcmat;
}pcpcinfo;

#define PCPCINFO_CSRC( n) ( (n)->csrc)
#define PCPCINFO_CSRCMAT( n) ( (n)->csrcmat)
#define PCPCINFO_NONCSRC( n) ( (n)->noncsrc)
#define PCPCINFO_NONCSRCMAT( n) ( (n)->noncsrcmat)


postinfo *makePostinfo () {
	postinfo *pi = (postinfo *) malloc (sizeof (postinfo));
	POSTINFO_ISCSRC (pi) = 0;
	POSTINFO_COLIDX (pi) = 0;
	POSTINFO_VERTEX (pi) = NULL;
	return pi;
}


void printTopoverts (dynarray *topoarr) {
	int i, total = DYNARRAY_TOTALELEMS (topoarr);
	elem *e;
	vertex *v;
	printf ("[");
	for (i=0; i<total; i++) {
		e = DYNARRAY_ELEMS_POS (topoarr, i);
		v = TOPOINFO_VERTEX (((topoinfo *) ELEM_DATA (e)));
		printf ("%d,", VERTEX_PRE (v));
	}
	printf("]\n");
}

matrix *LUBcreateReachMat( compinfo *ci){

	dynarray *csrc, *ctar, *prearr;
	int i, j;
	matrix *result;
	vertex *srcvert, *tarvert;
	elem *e;

	result = makeMatrix ();

	csrc = COMPINFO_CSRC( ci);
	ctar = COMPINFO_CTAR( ci);
	prearr = COMPINFO_PREARR( ci);

	for( i = 0; i < DYNARRAY_TOTALELEMS( csrc); i++){

		e = DYNARRAY_ELEMS_POS( prearr, ELEM_IDX(
				DYNARRAY_ELEMS_POS( csrc, i)) - 1);
		srcvert = ( vertex *) ELEM_DATA ( e);

		for( j = 0; j < DYNARRAY_TOTALELEMS( ctar); j++){

			e = DYNARRAY_ELEMS_POS( prearr, ELEM_IDX(
					DYNARRAY_ELEMS_POS( ctar, j)) - 1);
			tarvert = ( vertex *) ELEM_DATA ( e);

			if( GINisReachable( srcvert, tarvert, ci)){
				//printf ("%d reaches %d\n", srcvert->pre, tarvert->pre);
				setMatrixValue( result, j, i, 1);
			} else {
				setMatrixValue( result, j, i, 0);
			}

		}

	}

	return result;

}

matrix* LUBcreatePCPTMat( matrix *reachmat, compinfo *ci){

	matrix *pcptmat;
	elemstack *stk;
	elem *e;
	dynarray *csrc, *ctar;

	csrc = COMPINFO_CSRC( ci);
	ctar = COMPINFO_CTAR( ci);

	int i, j;
	int prev_lower = -1;

	stk = (elemstack *) malloc( sizeof( elemstack));
	initElemstack(stk);

	pcptmat = makeMatrix ();

	for( i = 0; i < DYNARRAY_TOTALELEMS( ctar); i++){

		prev_lower = -1;

		for( j = 0; j < DYNARRAY_TOTALELEMS( csrc) + 1; j++){

			e = (elem *) malloc( sizeof( elem));
			ELEM_IDX(e) = j;
			ELEM_DATA(e) = malloc( 2 * sizeof( int));
			((int *)ELEM_DATA(e))[0] = prev_lower;

			pushElemstack( &stk, e);

			if ( j >= DYNARRAY_TOTALELEMS( csrc)) {
				break;
			}

			if( getMatrixValue( reachmat, i, j) == 1){

				while( !isElemstackEmpty(stk)){

					e = popElemstack( &stk);
					/*
					 * Store the preorder number of a cross edge source reaching a
					 * particular cross edge target here. The property of this cross edge
					 * source is that its pre-order number should be less than or equal to
					 * the current cross edge source under examination (indexed by j).
					 */

					((int *)ELEM_DATA(e))[1] = ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, j));
					setMatrixElem( pcptmat, i, ELEM_IDX(e), e);

				}

				prev_lower = ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, j));

			}

		}

		/*
		 * We have examined all cross edge sources now for a given cross edge
		 * target. It may so happen that for the last few cross edge sources, there
		 * may not be any cross edge source which has a higher preorder number and
		 * reached that cross edge target currently under consideration!
		 *
		 * Pop rest of the elements off the stack now. Its time to store a null (
		 * signified by -1) vertex preoder number as the cross edge source reaching
		 * the cross edge target currently under consideration (indexed by i).
		 */

		while( !isElemstackEmpty( stk)){

			e = popElemstack( &stk);
			((int *)ELEM_DATA(e))[1] = -1;
			setMatrixElem( pcptmat, i, ELEM_IDX(e), e);

		}

	}

	return pcptmat;

}


/** <!--********************************************************************-->
 *
 * @fn dynarray * LUBsortInPostorder( compinfo *ci)
 *
 *   @brief
 *   This function uses the list of vertices in preorder sequence and the
 *   list of cross edge sources as inputs. It then sorts the cross edge
 *   sources in post order sequence, stores this information in a new dynamic
 *   array and returns the array. In this array, we additionally store the
 *   preorder numbers of the cross edge sources.
 *
 *   @param ci
 *
 *   @return result
 *
 *****************************************************************************/
dynarray * LUBsortInPostorder( compinfo *ci){

	dynarray *result, *prearr, *csrc;
	int i;
	elem *e;
	postinfo *data;
	vertex *v;
	int prenum;

	prearr = COMPINFO_PREARR( ci);
	csrc = COMPINFO_CSRC( ci);

	if (!(prearr != NULL && csrc != NULL)) {
		printf ("Incompatible arguments passed to LUBsortInPostorder");
		exit(-1);
	}

	result = (dynarray *) malloc( sizeof( dynarray));
	initDynarray( result);

	for( i = 0; i < DYNARRAY_TOTALELEMS( csrc); i++){

		e = (elem *) malloc( sizeof( elem));

		prenum = ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, i));
		v = (vertex *) ELEM_DATA( DYNARRAY_ELEMS_POS( prearr, prenum - 1));

		ELEM_IDX(e) = VERTEX_POST( v);
		ELEM_DATA(e) = makePostinfo();

		data = ( postinfo *)( ELEM_DATA(e));

		POSTINFO_ISCSRC( data) = 1;
		POSTINFO_COLIDX( data) = i;
		POSTINFO_VERTEX( data) = v;

		addToArray( result, e);

	}

	sortArray( DYNARRAY_ELEMS( result), 0,
			DYNARRAY_TOTALELEMS( result) - 1, 0);

	return result;

}

void LUBorColumnsAndUpdate( matrix *m1, int colidx1,
		matrix *m2, int colidx2,
		matrix *result, int rescolidx){

	if (!( MATRIX_TOTALROWS( m1) == MATRIX_TOTALROWS( m2))) {
		printf ("The two matrices in LUBorColumnsAndAppend do "
				"not have the same row count");
		exit (-1);
	}

	if (!(result != NULL)) {
		printf ("Result matrix cannot be empty");
		exit (-1);
	}

	int i, value;

	for( i = 0; i < MATRIX_TOTALROWS( m1); i++){

		/*
		 * At the moment we should refrain from using a bitwise OR operation below
		 * because the matrix cells are by default initialized to -1.
		 */

		if( getMatrixValue( m1, i, colidx1) == 1 ||
				getMatrixValue( m2, i, colidx2) == 1) {
			value = 1;
		} else {
			value = 0;
		}

		setMatrixValue( result, i, rescolidx, value);

	}

}

int LUBisNodeCsrc( vertex *n, dynarray *csrc){

	int i, result = 0;

	for( i = 0; i < DYNARRAY_TOTALELEMS( csrc); i++){

		if( VERTEX_PRE(n) == ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, i))){
			result = 1;
			break;
		}

	}

	return result;

}

dynarray *LUBrearrangeCsrcOnTopo( dynarray *csrc, dynarray *prearr){

	dynarray *result;
	vertex *v;
	int i;
	elem *e, *currpre, *currcsrc;

	result = (dynarray *) malloc( sizeof( dynarray));
	initDynarray( result);

	for( i = 0; i < DYNARRAY_TOTALELEMS( csrc); i++){

		currcsrc = DYNARRAY_ELEMS_POS( csrc, i);
		currpre = DYNARRAY_ELEMS_POS( prearr, ELEM_IDX( currcsrc) - 1);
		v = ((vertex *) (ELEM_DATA( currpre)));

		e = (elem *) malloc( sizeof( elem));
		ELEM_IDX( e) = VERTEX_TOPO( v);
		ELEM_DATA( e) = malloc( sizeof( topoinfo));

		TOPOINFO_COLIDX( (topoinfo *) ELEM_DATA( e)) = i;
		TOPOINFO_VERTEX( (topoinfo *) ELEM_DATA( e)) = v;

		addToArray( result, e);

	}

	sortArray( DYNARRAY_ELEMS( result), 0, DYNARRAY_TOTALELEMS( result) - 1, 0);

	return result;

}

dynarray *LUBrearrangeNoncsrcOnTopo( dynarray *noncsrc){

	dynarray *result;
	int i;
	elem *e1, *e2;
	vertex *vertex;

	result = (dynarray *) malloc( sizeof( dynarray));
	initDynarray( result);

	for( i = 0; i < DYNARRAY_TOTALELEMS( noncsrc); i++){

		e1 = DYNARRAY_ELEMS_POS( noncsrc, i);
		vertex = POSTINFO_VERTEX( ( postinfo *) ELEM_DATA( e1));

		e2 = (elem *) malloc( sizeof( elem));
		ELEM_IDX( e2) = VERTEX_TOPO( vertex);
		ELEM_DATA( e2) = malloc( sizeof( topoinfo));

		TOPOINFO_COLIDX( (topoinfo *) ELEM_DATA( e2)) =
				POSTINFO_COLIDX( ( postinfo *) ELEM_DATA( e1));
		TOPOINFO_VERTEX( (topoinfo *) ELEM_DATA( e2)) = vertex;

		addToArray( result, e2);

	}

	sortArray( DYNARRAY_ELEMS( result), 0, DYNARRAY_TOTALELEMS( result) - 1, 0);

	return result;

}

matrix *LUBcomputeMaximalWitness( pcpcinfo *ppi, vertex *top){

	matrix *result;
	matrix *csrcmax, *noncsrcmax;

	dynarray *csrc, *noncsrc;
	matrix *csrcmat, *noncsrcmat;
	int i, j, k, max = -1, idx;
	vertex *vertex_csrc, *vertex_noncsrc;

	csrc = PCPCINFO_CSRC( ppi);
	csrcmat = PCPCINFO_CSRCMAT( ppi);

	//printTopoverts (csrc);
	//printMatrix (csrcmat);

	csrcmax = makeMatrix ();

	for( i = 0; i < MATRIX_TOTALROWS( csrcmat); i++){

		for( j = 0; j < MATRIX_TOTALROWS( csrcmat); j++){

			for( k = 0; k < MATRIX_TOTALCOLS( csrcmat); k++){

				if( getMatrixValue( csrcmat, i, k) &&
						getMatrixValue(csrcmat, j, k)){
					max = k;
				}

			}

			setMatrixValue( csrcmax, i, j, max);
			max = -1;

		}

	}

	//printMatrix (csrcmax);

	if(PCPCINFO_NONCSRCMAT( ppi) == NULL) {
		for( i = 0; i < MATRIX_TOTALROWS( csrcmax); i++) {

			for( j = 0; j < MATRIX_TOTALCOLS( csrcmax); j++){
				idx = getMatrixValue( csrcmax, i, j);
				if (idx != -1) {
					vertex_csrc = TOPOINFO_VERTEX( ( topoinfo *) ELEM_DATA(
									DYNARRAY_ELEMS_POS( csrc, idx)));
				} else {
					vertex_csrc = top;
				}
				setMatrixValue (csrcmax, i, j, VERTEX_PRE( vertex_csrc));
			}

		}
		return csrcmax;
	}

	noncsrc = PCPCINFO_NONCSRC( ppi);
	noncsrcmat = PCPCINFO_NONCSRCMAT( ppi);
	noncsrcmax = makeMatrix( );

	//printTopoverts (noncsrc);
	//printMatrix (noncsrcmat);

	max = -1;

	for( i = 0; i < MATRIX_TOTALROWS( noncsrcmat); i++){

		for( j = 0; j < MATRIX_TOTALROWS( noncsrcmat); j++){

			for( k = 0; k < MATRIX_TOTALCOLS( noncsrcmat); k++){

				if( getMatrixValue( noncsrcmat, i, k) &&
						getMatrixValue( noncsrcmat, j, k)){
					max = k;
				}

			}

			setMatrixValue( noncsrcmax, i, j, max);
			max = -1;

		}

	}

	//printMatrix (noncsrcmax);

	/*
	 * We have two matrices now. Each cell in each matrix contains an index to the
	 * csrc and noncsrc arrays. These arrays hold vertices sorted in topological
	 * sequence. Now we compare the two matrices cell-wise and store the pre-order
	 * number of the vertex which has a higher topological number between the two
	 * cell entries.
	 */

	if (!( MATRIX_TOTALROWS( csrcmax) == MATRIX_TOTALROWS( noncsrcmax) &&
			MATRIX_TOTALCOLS( csrcmax) == MATRIX_TOTALCOLS( noncsrcmax))) {
		printf ("Matrix shape mismatch while building PC-PC matrix.");
		exit(-1);
	}

	result = makeMatrix();

	for( i = 0; i < MATRIX_TOTALROWS( csrcmax); i++) {

		for( j = 0; j < MATRIX_TOTALCOLS( csrcmax); j++){

			if (getMatrixValue( csrcmax, i, j) == -1) {
				vertex_csrc = top;
			} else {
				vertex_csrc = TOPOINFO_VERTEX( ( topoinfo *) ELEM_DATA(
									DYNARRAY_ELEMS_POS( csrc,
											getMatrixValue( csrcmax, i, j))));
			}

			if (getMatrixValue( noncsrcmax, i, j) == -1) {
				vertex_noncsrc = top;
			} else {
				vertex_noncsrc = TOPOINFO_VERTEX( ( topoinfo *) ELEM_DATA(
									DYNARRAY_ELEMS_POS( noncsrc,
											getMatrixValue( noncsrcmax, i, j))));
			}

			if( VERTEX_TOPO( vertex_csrc) > VERTEX_TOPO( vertex_noncsrc)){
				setMatrixValue( result, i, j, VERTEX_PRE( vertex_csrc));
			} else {
				setMatrixValue( result, i, j, VERTEX_PRE( vertex_noncsrc));
			}

		}

	}

	freeMatrix( csrcmax);
	freeMatrix( noncsrcmax);

	return result;

}

matrix *LUBrearrangeMatOnTopo( dynarray *topoarr, matrix *mat){

	matrix *result;
	topoinfo *ti;
	int i, j, value;

	result = makeMatrix();

	for( i = 0; i < DYNARRAY_TOTALELEMS( topoarr); i++){

		ti = (topoinfo *) ELEM_DATA( DYNARRAY_ELEMS_POS( topoarr, i));

		for( j = 0; j < MATRIX_TOTALROWS( mat); j++){
			value = getMatrixValue( mat, j, TOPOINFO_COLIDX( ti));
			setMatrixValue( result, j, i, value);
		}

	}

	return result;

}

matrix* LUBcreatePCPCMat( matrix *reachmat, dynarray *postarr, compinfo *ci){

	vertex *n1 = NULL, *n2, *treelca;
	matrix *result = NULL;
	matrix *currmat = NULL, *mat1, *mat2;
	postinfo *pi1, *pi2, *pi, *pi3;
	dynarray *noncsrc = NULL, *q = makeDynarray();
	elem *e, *e1, *e_min;
	int colidx=0, colidx_pi1, rescol = 0, i;
	pcpcinfo *ppi = NULL;
	vertex *top = NULL;

	for (i=0; i<DYNARRAY_TOTALELEMS (postarr); i++) {
		PQinsertElem (DYNARRAY_ELEMS_POS(postarr, i), q);
	}

	while( DYNARRAY_TOTALELEMS(q) > 0){

		//printDynarray (q);
		e_min = PQgetMinElem( q);
		pi1 = ( postinfo *) ELEM_DATA( e_min);
		n1 = POSTINFO_VERTEX( pi1);
		colidx_pi1 = POSTINFO_COLIDX( pi1);
		PQdeleteMin(q);
		//printDynarray (q);

		if( DYNARRAY_TOTALELEMS(q) == 0){
			break;
		} else {
			pi2 = ( postinfo *) ELEM_DATA( PQgetMinElem( q));
			n2 = POSTINFO_VERTEX( pi2);
			treelca = LUBtreeLCAfromNodes( n1, n2, ci);
			//printf("TREElca (%d, %d) = %d, post = %d\n",
			//		n1->post, n2->post, treelca->post, treelca->pre);
		}

		if (LUBisNodeCsrc( treelca, COMPINFO_CSRC( ci))) {
			if ( treelca == n1) {
				PQdeleteMin(q);
				e = (elem *) malloc( sizeof( elem));
				ELEM_IDX(e) = VERTEX_POST( n1);
				pi = makePostinfo();
				POSTINFO_ISCSRC( pi) = 1;
				POSTINFO_COLIDX( pi) = colidx_pi1;
				POSTINFO_VERTEX( pi) = n1;
				ELEM_DATA(e) = pi;
				PQinsertElem (e, q);
				continue;
			}
		}

		if( !LUBisNodeCsrc( treelca, COMPINFO_CSRC( ci))){

			/*
			 * Now, we update the reachable clusterheads for n1. Enqueue the newfound
			 * node in q here. Also add the newfound node to the array that hold
			 * pcpc-plcas that are not cross edge sources.
			 */
			if( noncsrc == NULL) {

				noncsrc = makeDynarray ();
				currmat = makeMatrix ();

			}

			/*
			 * Before inserting this vertex, we need to investigate whether it has
			 * already been discovered.
			 *
			 * TODO: The function indexExistsInArray(..) has linear complexity. We can
			 * write a function with a logarithmic complexity because q is a sorted
			 * array.
			 */

			if (!indexExistsInArray( noncsrc, VERTEX_POST( treelca))) {
				e1 = (elem *) malloc( sizeof( elem));
				ELEM_IDX(e1) = VERTEX_POST( treelca);
				pi3 = makePostinfo();
				POSTINFO_ISCSRC( pi3) = 0;
				POSTINFO_COLIDX( pi3) = colidx++;
				POSTINFO_VERTEX( pi3) = treelca;
				ELEM_DATA(e1) = pi3;

				addToArray( noncsrc, e1);
				//printDynarray (noncsrc);
			}

			if( !indexExistsInArray( q, VERTEX_POST( treelca))){
				//printf("inserting: %d\n", treelca->post);
				e = (elem *) malloc( sizeof( elem));
				ELEM_IDX(e) = VERTEX_POST( treelca);
				pi = makePostinfo();
				POSTINFO_ISCSRC( pi) = 0;
				POSTINFO_COLIDX( pi) = colidx-1;
				POSTINFO_VERTEX( pi) = treelca;
				ELEM_DATA(e) = pi;
				PQinsertElem( e, q);

			}

			e = getElemFromArray (noncsrc, VERTEX_POST(treelca));
			rescol = POSTINFO_COLIDX ((postinfo *) ELEM_DATA(e));


			if( LUBisNodeCsrc( n1, COMPINFO_CSRC( ci))){
				mat1 = reachmat;
			} else {
				mat1 = currmat;
			}

			if( LUBisNodeCsrc( n2, COMPINFO_CSRC( ci))){
				mat2 = reachmat;
			} else {
				mat2 = currmat;
			}

			//printf("copying: %d %d to %d\n", n1->pre, n2->pre ,treelca->pre);

			LUBorColumnsAndUpdate( mat1, colidx_pi1,
					mat2, POSTINFO_COLIDX(pi2),
					currmat, rescol/*colidx - 1*/);

		}

	}

	ppi = (pcpcinfo *) malloc( sizeof( pcpcinfo));

	PCPCINFO_CSRC( ppi) = LUBrearrangeCsrcOnTopo( COMPINFO_CSRC( ci),
			COMPINFO_PREARR( ci));
	PCPCINFO_CSRCMAT( ppi) = LUBrearrangeMatOnTopo( PCPCINFO_CSRC( ppi),
			reachmat);

	if( noncsrc != NULL){
		//printDynarray( noncsrc);
		//printMatrix( currmat);

		PCPCINFO_NONCSRC( ppi) = LUBrearrangeNoncsrcOnTopo( noncsrc);
		PCPCINFO_NONCSRCMAT( ppi) = LUBrearrangeMatOnTopo( PCPCINFO_NONCSRC( ppi),
				currmat);

	}

	top = (vertex *) ELEM_DATA (DYNARRAY_ELEMS_POS (COMPINFO_PREARR(ci), 0));

	result = LUBcomputeMaximalWitness( ppi, top);

	return result;

}

void LUBincorporateCrossEdges( compinfo *ci){

	matrix *reachmat;
	dynarray *postarr;

	if( COMPINFO_CSRC( ci) != NULL){

		reachmat = LUBcreateReachMat( ci);

		//printMatrix (reachmat);
		COMPINFO_CROSSCLOS (ci) = reachmat;
		postarr = LUBsortInPostorder( ci);

		LUBINFO_PCPTMAT( COMPINFO_LUB( ci)) = LUBcreatePCPTMat( reachmat, ci);
		LUBINFO_PCPCMAT( COMPINFO_LUB( ci)) = LUBcreatePCPCMat( reachmat,
				postarr,
				ci);

	}

}
