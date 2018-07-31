/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#include "graphtypes.h"
#include "graphutils.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "reachhelper.h"
#include "tfprintutils.h"

dynarray* buildTransitiveLinkTable( dynarray *arrayd, dynarray *csrc,
		dynarray *ctar, dynarray *prearr){

	/*
	 * Typical elements of arrayd here are of the form x->[y,z)
	 * We store x as the idx of elem and y,z as the associated data.
	 *
	 * x is pre-order of a cross edge source and y is pre-order of
	 * a cross-edge target. z is the max pre-order amongst children
	 * of y plus 1.
	 */

	int i, j, k, l, lower, upper, source, target;
	elem *e;
	//int i_idx, k_idx, j_data, k_data;
	int s, t, idx, data;
	matrix *adjmat;
	dynarray *result;
	vertex *v;

	adjmat = makeMatrix();

	for( i = 0 ; i < DYNARRAY_TOTALELEMS( arrayd) ; i++){
		idx = ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, i));
		data = *(int*) ELEM_DATA( DYNARRAY_ELEMS_POS( arrayd, i));
		s = getPositionInArray (csrc, idx);
		t = getPositionInArray (ctar, data);
		setMatrixValue( adjmat, s, t, 1);
	}

	/*
	 * We have the adjacency matrix now. Update it to get the transitive closure.
	 */

	for( i = 0 ; i < MATRIX_TOTALROWS( adjmat) ; i++){

		for( j = 0 ; j < DYNARRAY_TOTALELEMS( MATRIX_ARRAY2D( adjmat)[i]) ; j++){
			source = ELEM_IDX ( DYNARRAY_ELEMS_POS( csrc, i));
			target = ELEM_IDX ( DYNARRAY_ELEMS_POS( ctar, j));
			if (source == target) {
				setMatrixValue( adjmat, i, j, 1);
			}

			/*
			 * first close sources reaching sources through the tree
			 */
			if (getMatrixValue(adjmat, i, j) == 1) {
				for (k = 0; k < DYNARRAY_TOTALELEMS( csrc); k++) {

					lower = ELEM_IDX ( DYNARRAY_ELEMS_POS( csrc, k));
					v = (vertex *) ELEM_DATA (DYNARRAY_ELEMS_POS( prearr, lower-1));
					upper = VERTEX_PREMAX (v);

					source = ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, i));

					if( lower <= source && source < upper){
						setMatrixValue( adjmat, k, j, 1);
					}
				}

			}

			if (getMatrixValue(adjmat, i, j) == 1) {
				for (k = 0; k < DYNARRAY_TOTALELEMS( ctar); k++) {

					lower = ELEM_IDX ( DYNARRAY_ELEMS_POS( ctar, j));
					v = (vertex *) ELEM_DATA (DYNARRAY_ELEMS_POS( prearr, lower-1));
					upper = VERTEX_PREMAX (v);

					target = ELEM_IDX( DYNARRAY_ELEMS_POS( ctar, k));

					if( lower <= target && target < upper){
						setMatrixValue( adjmat, i, k, 1);
					}
				}

			}

			/*
			 * now close sources reaching targets reaching sources reaching targets
			 */

			if( getMatrixValue( adjmat, i, j) == 1){

				lower = ELEM_IDX ( DYNARRAY_ELEMS_POS( ctar, j));
				v = (vertex *) ELEM_DATA (DYNARRAY_ELEMS_POS( prearr, lower-1));
				upper = VERTEX_PREMAX (v);

				for( k = 0 ; k < DYNARRAY_TOTALELEMS( csrc) ; k++){

					source = ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, k));

/*					printf("l = %d, u = %d, s = %d\n",
							lower, upper, source);*/

					if( lower <= source && source < upper){

						/*
						 * source idx k is reachable from target idx j
						 * therefore any tar that k reaches must also be reachable
						 * from i
						 */

						for ( l = 0; l < DYNARRAY_TOTALELEMS (MATRIX_ARRAY2D (adjmat)[k]); l++){
							if (getMatrixValue( adjmat, k, l) == 1)
								setMatrixValue( adjmat, i, l, 1);
						}
						//printf("yes\n");
					}

				}

			}

		}

	}

	result = makeDynarray ();

	for( i = 0 ; i < MATRIX_TOTALROWS( adjmat) ; i++){

		for( j = 0 ; j < DYNARRAY_TOTALELEMS( MATRIX_ARRAY2D( adjmat)[i]) ; j++){

			if( /*i != j &&*/ getMatrixValue( adjmat, i, j) == 1){
				e = (elem *) malloc( sizeof( elem));
				ELEM_IDX(e) = ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, i));
				ELEM_DATA(e) = malloc(2 * sizeof(int));

				/*
				 * Tha data bit in the csrc elems contains pre-max values
				 */

				*(int *) ELEM_DATA(e) = ELEM_IDX( DYNARRAY_ELEMS_POS( ctar, j));
				elem *e_v = DYNARRAY_ELEMS_POS (prearr, *(int *) ELEM_DATA(e));
				vertex *v = (vertex *) ELEM_DATA (e_v);
				((int *) ELEM_DATA(e))[1] = VERTEX_PREMAX (v);
				addToArray( result, e);
			}

		}

	}

	freeMatrix( adjmat);
	return result;

}



void setSrcTarArrays( dynarray *arrayd, dynarray** arrX, dynarray** arrY){

	int a;
	elem *e;
	dynarray *arraydX, *arraydY;

	arraydX = (dynarray *) malloc( sizeof( dynarray));
	initDynarray( arraydX);
	arraydY = (dynarray *) malloc( sizeof( dynarray));
	initDynarray( arraydY);

	for( a = 0 ; a < DYNARRAY_TOTALELEMS( arrayd) ; a++){

		if( !indexExistsInArray( arraydX, ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, a)))){
			e = (elem *) malloc( sizeof( elem));
			ELEM_IDX( e) = ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, a));
			addToArray( arraydX, e);
		}

		if( !indexExistsInArray(arraydY,
				*((int *)ELEM_DATA(DYNARRAY_ELEMS_POS(arrayd,a))))){
			e = (elem *) malloc( sizeof( elem));
			ELEM_IDX(e) = *((int *)ELEM_DATA( DYNARRAY_ELEMS_POS( arrayd, a)));
			ELEM_DATA(e) = malloc( sizeof( int));
			/*
			 * the following initialisation is used for TLCmatrix computation
			 */
			*((int *)ELEM_DATA(e)) = 1;
			addToArray( arraydY, e);
		}
	}

	sortArray( DYNARRAY_ELEMS( arraydX), 0, DYNARRAY_TOTALELEMS( arraydX) - 1, 0);
	sortArray( DYNARRAY_ELEMS( arraydY), 0, DYNARRAY_TOTALELEMS( arraydY) - 1, 0);

	*arrX = arraydX;
	*arrY = arraydY;

}


matrix* computeTLCMatrix( dynarray *tltable, dynarray* csrc, dynarray* ctar){

	int i, j, a, i_pos, j_pos;
	matrix *tlc;

	tlc = makeMatrix();

	sortArray( DYNARRAY_ELEMS( tltable), 0, DYNARRAY_TOTALELEMS( tltable) - 1, 1);

	for( a = 0 ; a < DYNARRAY_TOTALELEMS( tltable) ; a++){
		i = ELEM_IDX (DYNARRAY_ELEMS_POS (tltable, a));
		j = *((int *)ELEM_DATA( DYNARRAY_ELEMS_POS( tltable, a)));
		i_pos = getPositionInArray (csrc, i);
		j_pos = getPositionInArray (ctar, j);
		setMatrixValue( tlc, i_pos, j_pos,
				(*(int *)ELEM_DATA(DYNARRAY_ELEMS_POS(ctar,j_pos)))++);


	}

	for (i= DYNARRAY_TOTALELEMS (csrc) - 2; i>=0; i-- ) {
		for (j=0; j<DYNARRAY_TOTALELEMS (ctar); j++) {
			if (getMatrixValue (tlc, i, j) == -1) {
				setMatrixValue (tlc, i, j, getMatrixValue (tlc, i+1, j));
			}
		}

	}

	return tlc;

}

