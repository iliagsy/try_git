/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#include "graphutils.h"
#include "dynelem.h"
#include "dynarray.h"
#include "dynmatrix.h"

/* This file describes a generic array which can grow dynamically as
 * well as a generic stack.
 *
 * These data structure is used to compute the transitive link matrix
 * and the non-tree labels in preprocess_graph.c
 */

void initMatrix( matrix *m){

  MATRIX_ARRAY2D(m) = NULL;
  MATRIX_TOTALROWS(m) = 0;
  MATRIX_TOTALCOLS(m) = 0;

}

matrix *makeMatrix () {
	matrix *m = (matrix *) malloc (sizeof (matrix));
	initMatrix (m);
	return m;
}

void free2DArray( dynarray **d2, int count){

  if( d2 != NULL){

    int i;

    for( i = 0; i < count; i++){

      if( d2[i] != NULL) {

	freeDynarray( d2[i]);
	d2[i] = NULL;

      }

    }

    free(d2);
    d2=NULL;

  }

}

void freeMatrix( matrix *m){

  if( m != NULL){

    if( MATRIX_ARRAY2D(m) != NULL){

      free2DArray( MATRIX_ARRAY2D(m), MATRIX_TOTALROWS(m));
      MATRIX_ARRAY2D(m) = NULL;

    }

    free(m);
    m=NULL;

  }

}

void setMatrixElem( matrix *m, int x, int y, elem *element){

  int i, oldlength;

  oldlength = MATRIX_TOTALROWS(m);

  /*
   * Grow the matrix columnwise if necessary.
   */

  if( MATRIX_TOTALCOLS(m) < y + 1){
    MATRIX_TOTALCOLS(m) = y + 1;

    for( i = 0; i < MATRIX_TOTALROWS(m); i++){
      addToArrayAtPos( MATRIX_ARRAY2D(m)[i], NULL, MATRIX_TOTALCOLS(m) - 1);
    }

  }

  /*
   * Now grow the matrix rowwise if necessary.
   */

  if( MATRIX_TOTALROWS(m) < x + 1){
    MATRIX_TOTALROWS(m) = x + 1;

    void *_temp = realloc( MATRIX_ARRAY2D(m),
      ( MATRIX_TOTALROWS(m) * sizeof( dynarray *))/*,
      oldlength * sizeof( dynarray *)*/);

    if (!_temp){
      printf( "setMatrixValue couldn't realloc memory!\n");
      exit(-1);
    }

    /*free( MATRIX_ARRAY2D(m));*/
    MATRIX_ARRAY2D(m) = ( dynarray**)_temp;

  }

  for( i = oldlength; i < MATRIX_TOTALROWS(m); i++){
    MATRIX_ARRAY2D(m)[i] = makeDynarray();
    addToArrayAtPos( MATRIX_ARRAY2D(m)[i], NULL, MATRIX_TOTALCOLS(m) - 1);
  }

  addToArrayAtPos( MATRIX_ARRAY2D(m)[x], element, y);

}

void setMatrixValue( matrix *m, int x, int y, int value){

  elem *element = (elem *) malloc( sizeof( elem));
  ELEM_IDX( element) = value;
  ELEM_DATA( element) = NULL;

  setMatrixElem( m, x, y, element);

}

elem *getMatrixElem( matrix *m, int x, int y){

  dynarray *arrayd = MATRIX_ARRAY2D(m)[x];
  elem *e = DYNARRAY_ELEMS( arrayd)[y];
  return e;
}

int getMatrixValue( matrix *m, int x, int y){

  elem *e = getMatrixElem( m, x, y);
  if( e != NULL) return ELEM_IDX(e);
  else return -1;

}


