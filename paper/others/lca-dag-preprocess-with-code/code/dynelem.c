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
#include "dynelem.h"

void initElem( elem *e){
 
  ELEM_IDX(e) = 0;
  ELEM_DATA(e) = NULL;

}

elem *makeElem () {
	elem *e = (elem *) ( malloc (sizeof (elem)));
	initElem (e);
	return e;
}

void freeElem( elem *e){

  if( e != NULL){
  
    if( ELEM_DATA(e) != NULL){
    
      free( ELEM_DATA(e));
      ELEM_DATA(e) = NULL;
    
    }
  
    free(e);
    e = NULL;
  
  }

}

