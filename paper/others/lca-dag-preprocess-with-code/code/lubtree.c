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
 * @file lubtree.c
 *
 * prefix: LUB
 *
 * description: This file contains functions to compute Least Upper Bound (LUB)
 * a.k.a Lowest Common Ancestor (LCA) of two types in the spanning tree of the
 * type dependency graph.
 *
 * For each dependency graph, we maintain 4 data structures:
 *
 * 1. The euler tour of the graph: This holds information about the order in
 * which the vertices are visited in the euler tour of the DAG. We use the
 * "dynarray" data structure for this purpose. Therein, "elem" structure holds
 * depth of each vertex in the spanning tree as well as the preorder number of
 * the vertex.
 *
 * 2. Intra-block matrices: Each euler tour is sub-divided into intra blocks
 * (refer to the literature on LCA computation in trees). For each intra-block,
 * the intra-block matrices are precomputed for answering LCA queries on
 * vertices contained in the intra-block. The matrix entries in this case are
 * just numbers which enable us to index the euler tour array and obtain the
 * pre-order number of the result.
 *
 * 3. Intra-block minimum array: While we can now answer intra-block queries, we
 * need a mechanism to answer queries where the two vertices do not belong to
 * the same block. This is achieved in a two step process. In the first step, we
 * store the depth and pre-order numbers of the vertex with the lowest depth in
 * any given intra-block.
 *
 * 4. Inter-block matrix: In the second step, we process the intra-block minimum
 * array to pre-compute queries spanning multiple blocks. How this is done
 * exactly can be found in the literature. In particular, the inquisitive minds
 * should read papers on LCA by Berkman and Vishkin, Michael Bender and Johannes
 * Fischer.
 *
 * In to compute the LCA of two vertices, we need to know the position of their
 * first occurence in the euler tour and then apply range minimum query
 * techniques to get the pre-order number of the LCA.
 */
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphutils.h"
#include "graphtypes.h"
#include "tfprintutils.h"
#include "lubtree.h"

/** <!--********************************************************************-->
 *
 * @fn
 *
 *   @brief
 *
 *   @param
 *   @return
 *
 *****************************************************************************/

void LUBsetBlockIds( dynarray *eulertour, int blocksize){

	int i, j, prevdepth, currdepth, blockid = 0;

	for( i = 0; i < DYNARRAY_TOTALELEMS( eulertour); i = i + blocksize){

		prevdepth = ELEM_IDX( DYNARRAY_ELEMS_POS( eulertour, i));

		for( j = i + 1 ; j < i + blocksize; j++){

			if( j < DYNARRAY_TOTALELEMS( eulertour)){

				currdepth = ELEM_IDX( DYNARRAY_ELEMS_POS( eulertour, j));

				/*
				 * Since, the adjacent depth values differ by either +1 (if the depth
				 * increases) or -1 (if the depth decreases), a block can be represented
				 * by a bit vector where a 1 denotes an increase in depth and a 0
				 * denotes a decrease in depth. The blockid is just the decimal
				 * representation of this bit vector.
				 */

				if( prevdepth > currdepth){
					blockid += pow( 2, ( blocksize - 2 - (j - ( i + 1))));
				}

				prevdepth = currdepth;

			} else {

				blockid *= 2;

			}

		}

		for( j = i; j < i + blocksize; j++){

			if( j < DYNARRAY_TOTALELEMS( eulertour)){
				( ( int *)ELEM_DATA( DYNARRAY_ELEMS_POS( eulertour, j)))[1] = blockid;
			}

		}

		blockid = 0;

	}

}

matrix* LUBcomputeIntraTable( dynarray *eulertour, int start, int end){

	if (!( start <= end && eulertour != NULL)) {
		printf ("Incompatible arguments passed to LUBcomputeIntraTable");
		exit(-1);
	}

	int i, j, minvalue, minindex, currdepth;
	matrix *result = makeMatrix ();

	for( i = 0; i <= end - start + 1; i++){

		/*
		 * Pick an element from the array
		 */

		if( start + i < DYNARRAY_TOTALELEMS( eulertour)){
			minvalue = ELEM_IDX( DYNARRAY_ELEMS_POS( eulertour, start + i));
			minindex = start + i;

			for( j = i; j <= end - start; j++){

				/*
				 * Get the result of RMQ( x, y) where x is the element that was picked
				 * in the previous step and y belongs to the elements in the euler tour
				 * from the picked element to the last element.
				 */

				if( start + j < DYNARRAY_TOTALELEMS( eulertour)){

					currdepth = ELEM_IDX( DYNARRAY_ELEMS_POS( eulertour, start + j));

					if( minvalue > currdepth){
						minvalue = currdepth;
						minindex = start + j;
					}

				}

				setMatrixValue( result, i, j, minindex - start);
				setMatrixValue( result, j, i, minindex - start);

			}

		}

	}

	return result;

}

dynarray* LUBcomputePerBlockMin( dynarray *eulertour, int blocksize){

	if (!( blocksize > 0 && eulertour != NULL)) {
		printf ("Incompatible arguments passed to LUBcomputePerBlockMin");
		exit(-1);
	}

	dynarray *result;
	int mindepth, currdepth, minindex, i, j;
	elem *e;

	result =  makeDynarray();

	for( i = 0; i < DYNARRAY_TOTALELEMS( eulertour); i = i + blocksize){

		mindepth = ELEM_IDX( DYNARRAY_ELEMS_POS( eulertour, i));
		minindex = i;

		for( j = i + 1 ; j < i + blocksize; j++){

			if( j < DYNARRAY_TOTALELEMS( eulertour)){

				currdepth = ELEM_IDX( DYNARRAY_ELEMS_POS( eulertour, j));

				if( mindepth > currdepth){
					mindepth = currdepth;
					minindex = j;
				}

			}

		}

		e = makeElem ();
		ELEM_IDX( e) = mindepth;
		ELEM_DATA(e) = malloc( sizeof( int));

		*(int *) ELEM_DATA(e) = minindex;

		addToArray( result, e);

	}

	return result;

}

matrix * LUBprocessBlockMinArray ( dynarray *a){

	if (!( a != NULL && DYNARRAY_TOTALELEMS(a) > 0)) {
		printf ("Incompatible arguments passed to LUBprocessBlockMinArray");
		exit(-1);
	}

	int i, j, halfstep, fullstep, totalelems;
	matrix *m = makeMatrix ();

	totalelems = DYNARRAY_TOTALELEMS(a);

	if( totalelems == 1 ){
		setMatrixValue( m, 0, 0, 0);
		return m;
	}

	for( j = 0; j < ceil( log2( totalelems)); j++){

		setMatrixValue( m, totalelems - 1, j, totalelems - 1);

	}

	for( j = 0; j < ceil( log2( totalelems)); j++){

		for( i = 0; i < totalelems - 1; i++){

			if( j == 0){

				if( ELEM_IDX( DYNARRAY_ELEMS_POS( a, i)) <
						ELEM_IDX( DYNARRAY_ELEMS_POS( a, i + 1))){

					setMatrixValue( m, i, 0, i);

				} else {

					setMatrixValue( m, i, 0, i + 1);

				}

			} else {

				halfstep = getMatrixValue( m, i, j - 1);

				if( i + (int)pow( 2, j - 1) < totalelems){

					fullstep = getMatrixValue( m, i + (int)pow( 2, j - 1), j - 1);

				} else {

					fullstep = getMatrixValue( m, totalelems - 1, j - 1);
				}

				if( ELEM_IDX( DYNARRAY_ELEMS_POS( a, halfstep)) <
						ELEM_IDX( DYNARRAY_ELEMS_POS( a, fullstep))) {

					setMatrixValue( m, i, j, halfstep);

				} else {

					setMatrixValue( m, i, j, fullstep);

				}

			}

		}

	}

	return m;

}

int LUBgetBlockId( dynarray *eulertour, int index){

	elem *e = DYNARRAY_ELEMS_POS( eulertour, index);

	return ((( int *)ELEM_DATA(e))[1]);

}

lubinfo * LUBcreatePartitions( dynarray *eulertour){

	int i, j, totalelems, blocksize, oldsize, index;
	lubinfo *lub = makelubinfo();

	totalelems = DYNARRAY_TOTALELEMS( eulertour);

	/*
	 * We need to ensure that the blocksize is at least 1.
	 */

	if( totalelems == 1){
		blocksize = 1;
	} else {
		blocksize = log2( totalelems) / 2.0;
	}

	LUBINFO_BLOCKSIZE( lub) = blocksize;

	//printf ( "Size of block for LCA query on spanning tree is %d\n",
	//		blocksize);

	LUBsetBlockIds( eulertour, blocksize);

	for( i = 0; i < totalelems; i += blocksize) {

		oldsize = LUBINFO_NUMINTRA( lub);

		index = LUBgetBlockId( eulertour, i);

		/*
		 * check if the index falls within the currently allocated size. If not,
		 * then reallocate memory to include the index.
		 */

		if( index > oldsize - 1){

			void *_temp = realloc( LUBINFO_INTRAMATS( lub),
					( (index + 1) * sizeof( matrix *))/*,
					( oldsize * sizeof( matrix *))*/);
			if ( !_temp){
				printf ( "LUBcreatePartitions couldn't realloc memory!\n");
				exit(-1);
			}
			//TODO:check suspected shallow free

			/*freeMatrix( LUBINFO_INTRAMATS( lub));*/
			LUBINFO_INTRAMATS( lub) = ( matrix **)_temp;
			LUBINFO_NUMINTRA( lub) = index + 1;

			for( j = oldsize /*- 1*/; j < LUBINFO_NUMINTRA( lub); j++){
				LUBINFO_INTRAMATS_POS( lub, j) = NULL;
			}

		}

		if( LUBINFO_INTRAMATS_POS( lub, index) == NULL){
			LUBINFO_INTRAMATS_POS( lub, index) =
					LUBcomputeIntraTable( eulertour, i, i + blocksize - 1);
		}

	}

	LUBINFO_BLOCKMIN( lub) = LUBcomputePerBlockMin( eulertour, blocksize);

	LUBINFO_INTERMAT( lub) =
			LUBprocessBlockMinArray( LUBINFO_BLOCKMIN( lub));

	return lub;

}

int LUBgetLowestFromCandidates( dynarray *d, int indices[4]){

	int i, result;
	int mindepth;

	mindepth = ELEM_IDX( DYNARRAY_ELEMS_POS( d, indices[0]));
	result = *( int *) ELEM_DATA( DYNARRAY_ELEMS_POS( d, indices[0]));

	for( i = 1; i < 4; i++){
		if( mindepth >= ELEM_IDX( DYNARRAY_ELEMS_POS( d, indices[i] ))){
			mindepth = ELEM_IDX( DYNARRAY_ELEMS_POS( d, indices[i] ));
			result = *( int *) ELEM_DATA( DYNARRAY_ELEMS_POS( d, indices[i]));
		}
	}

	return result;

}

vertex *LUBtreeLCAfromNodes( vertex *n1, vertex *n2, compinfo *ci){

	if (!( n1 != NULL && n2 != NULL && ci != NULL)) {
		printf ("Incompatible arguments passed to LUBtreeLCAfromNodes");
		exit(-1);
	}

	vertex *result;
	int lblockid, lmatrow, lmatcol;
	int ublockid, umatrow, umatcol;
	int lowerid, upperid;
	int blocksize;
	int etindices[4] = {0, 0, 0, 0};
	int base, jump;
	int indexlower, indexupper;
	int resultidx;
	matrix *intermat;
	matrix **intramats;
	dynarray *blockmin;
	elem *e;

	lubinfo *lub = COMPINFO_LUB( ci);

	if (!(lub != NULL)){
		printf ("The type component graph lacks LCA info");
		exit(-1);
	}

	intramats = LUBINFO_INTRAMATS( lub);

	if (!(intramats != NULL)) {
		printf ("No intra matrices found");
		exit(-1);
	}

	blocksize = LUBINFO_BLOCKSIZE( lub);

	if ( !(blocksize > 0)) {
		printf ("Blocksize should be a positive integer");
		exit(-1);
	}

	if( VERTEX_EULERID( n1) < VERTEX_EULERID( n2)){
		lowerid = VERTEX_EULERID( n1);
		upperid = VERTEX_EULERID( n2);
	} else {
		lowerid = VERTEX_EULERID( n2);
		upperid = VERTEX_EULERID( n1);
	}

	/*
	 * Check if the vertices belong to the same intra-block
	 */

	lblockid = LUBgetBlockId( COMPINFO_EULERTOUR( ci), lowerid);
	ublockid = LUBgetBlockId( COMPINFO_EULERTOUR( ci), upperid);


	if( upperid/blocksize == lowerid/blocksize){
		lmatrow = lowerid % blocksize;
		lmatcol = upperid % blocksize;

		indexlower = ( lowerid/blocksize) * blocksize +
				getMatrixValue( intramats[lblockid], lmatrow, lmatcol);

		e = DYNARRAY_ELEMS_POS( COMPINFO_EULERTOUR( ci), indexlower);

		etindices[0] = indexlower;
		etindices[1] = indexlower;
		etindices[2] = indexlower;
		etindices[3] = indexlower;

	} else {
		/*
		 * The two vertices do not belong to the same intra-block
		 */

		lmatrow = lowerid % blocksize;
		lmatcol = blocksize - 1;

		indexlower = ( lowerid/blocksize) * blocksize +
				getMatrixValue( intramats[lblockid], lmatrow, lmatcol);

		etindices[0] = indexlower;

		umatrow = 0;
		umatcol = upperid % blocksize;

		indexupper = ( upperid/blocksize) * blocksize +
				getMatrixValue( intramats[ublockid], umatrow, umatcol);

		etindices[3] = indexupper;


		intermat = LUBINFO_INTERMAT( lub);
		if (!( intermat != NULL)) {
			printf ("No inter-block query matrix found");
			exit (-1);
		}

		blockmin = LUBINFO_BLOCKMIN( lub);
		if (! (blockmin != NULL)) {
			printf ("No block minimum array found");
			exit (-1);
		}

		if( (upperid/blocksize) > (lowerid/blocksize + 1)){

			jump = ceil( log2( upperid/blocksize - lowerid/blocksize - 1)) - 1;

			if(jump>=0) {
			base = lowerid/blocksize + 1;
			e =  DYNARRAY_ELEMS_POS( blockmin,
					getMatrixValue( intermat, base, jump));
			etindices[1] = *(int *) ELEM_DATA(e);

			//printf("range 1: [%d, %d] et_1 = %d\n", base, jump, etindices[1]);

			base = upperid/blocksize - 1 - pow( 2, jump);
			e =  DYNARRAY_ELEMS_POS( blockmin,
					getMatrixValue( intermat, base, jump));
			etindices[2] = *(int *) ELEM_DATA(e);

			//printf("range 2: [%d, %d] et_2 = %d\n", base, jump, etindices[2]);
			} else {
				e =  DYNARRAY_ELEMS_POS( blockmin, lowerid/blocksize+1);
				etindices[1] = *(int *) ELEM_DATA(e);
				etindices[2] = etindices[1];
			}

		} else {

			/*
			 * performing a "suspect hack" below for the case when
			 * upperid/blocksize = lowerid/blocksize + 2 (or 1)
			 *
			 * etindices are the indices of the vertices in the euler tour of
			 * the spanning tree.
			 */

			etindices[1] = etindices[0] + 1;
			etindices[2] = etindices[3] - 1;

		}

	}


	//printf("et = {%d, %d, %d, %d}\n", etindices[0], etindices[1], etindices[2], etindices[3]);
	//printf("lblockid=%d, ublockid=%d, jump = %d\n", lowerid/blocksize, upperid/blocksize, jump);

	resultidx = LUBgetLowestFromCandidates( COMPINFO_EULERTOUR(ci),
			etindices);

	//printf ("resultidx:%d\n",resultidx);

	e = DYNARRAY_ELEMS_POS( COMPINFO_PREARR(ci), resultidx - 1);

	result = (vertex *) ELEM_DATA(e);

	return result;

}
