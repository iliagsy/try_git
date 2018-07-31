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
#include "binheap.h"
#include "tfprintutils.h"

/*
 * A priority queue is implemented as a dynarray here.
 */

void PQinsertElem( elem *e, dynarray *q) {

  int i, mid;


  i = DYNARRAY_TOTALELEMS(q);
  addToArray( q, e);

  while (1) {

    mid = (i-1)/2;

    if( i==mid ||
    		ELEM_IDX( DYNARRAY_ELEMS_POS( q, mid)) <= ELEM_IDX(e)) {
      break;
    }

    //e = DYNARRAY_ELEMS_POS( q, i);
    DYNARRAY_ELEMS_POS( q, i) = DYNARRAY_ELEMS_POS( q, mid);
    //DYNARRAY_ELEMS_POS( q, mid) = e;

    i = mid;

  }

  DYNARRAY_ELEMS_POS( q, i) = e;

}

void PQinsert( int x, dynarray *q) {

  elem *e;

  e = (elem *) malloc (sizeof( elem));
  ELEM_DATA(e) = NULL;
  ELEM_IDX(e) = x;

  PQinsertElem( e, q);

}

void PQdeleteMin( dynarray *q) {

  int i, child;
  elem *last;

  if (!(DYNARRAY_TOTALELEMS(q) > 0)) {
	  printf ("PQdeleteMin : Priority queue is empty\n");
	  exit(-1);
  }

  last = DYNARRAY_ELEMS_POS( q, DYNARRAY_TOTALELEMS(q) - 1);

  for (i = 0; i * 2 < DYNARRAY_TOTALELEMS(q) - 2; i = child) {

    child = i * 2 + 1;

    if( ELEM_IDX( DYNARRAY_ELEMS_POS( q, child + 1)) <
	ELEM_IDX( DYNARRAY_ELEMS_POS( q, child ))) {
      child++;
    }

    if( ELEM_IDX( last) > ELEM_IDX( DYNARRAY_ELEMS_POS( q, child ))) {

      if( i == 0) {
    	  freeElem( DYNARRAY_ELEMS_POS( q, i));
      }

      DYNARRAY_ELEMS_POS( q, i) = DYNARRAY_ELEMS_POS( q, child);

    } else {

      break;

    }

  }

  DYNARRAY_ELEMS_POS( q, i) = last;
  DYNARRAY_ELEMS_POS( q, --DYNARRAY_TOTALELEMS(q)) = NULL;

}

elem* PQgetMinElem( dynarray *q) {

  if (!(DYNARRAY_TOTALELEMS(q) > 0)) {
	  printf ("PQgetMinElem : Priority queue is empty\n");
	  exit(-1);
  }

  elem *result;
  result = DYNARRAY_ELEMS_POS( q, 0);

  return result;

}

int PQgetMin( dynarray *q) {

  if (!(DYNARRAY_TOTALELEMS(q) > 0)) {
	  printf ("PQgetMin : Priority queue is empty\n");
	  exit(-1);
  }

  int result;
  result = ELEM_IDX( PQgetMinElem(q));

  return result;

}

void PQprint( dynarray *q){

  printDynarray(q);

}
