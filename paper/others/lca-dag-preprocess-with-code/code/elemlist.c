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
#include "elemlist.h"


void ELinit( elemlist *el){

  ELEMLIST_CURR( el) = NULL;
  ELEMLIST_PREV( el) = NULL;
  ELEMLIST_NEXT( el) = NULL;

}

elemlist *ELfreeNonRecursive( elemlist *el){

  if( el != NULL){
    free( el);
    el = NULL;
  }
  
  return el;

}

elemlist *ELfreeRecursive( elemlist *el){

  if( ELEMLIST_CURR( el) == NULL){
    freeElem( ELEMLIST_CURR( el));
  }

  el = ELfreeNonRecursive( el);

  return el;

}
