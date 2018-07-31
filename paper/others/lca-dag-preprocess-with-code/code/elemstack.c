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
#include "elemstack.h"

int isElemstackEmpty( elemstack *s){

	int result = 0;

	if( ELEMSTACK_CURR(s) == NULL){
		result = 1;
	}

	return result;

}

void initElemstack( elemstack *s){

	ELEMSTACK_CURR(s) = NULL;
	ELEMSTACK_NEXT(s) = NULL;

}

elemstack *makeElemstack (){
	elemstack *estack = (elemstack *) malloc ( sizeof (elemstack));
	initElemstack (estack);
	return estack;
}

void pushElemstack( elemstack **s, elem *e){

	elemstack *top = (elemstack *) malloc( sizeof( elemstack));
	ELEMSTACK_CURR( top) = e;
	ELEMSTACK_NEXT( top) = *s;
	*s = top;

}

elem* popElemstack( elemstack **s){

	elemstack *top = NULL;
	elem *e;

	if( *s == NULL){
		printf ( "Trying to pop from empty elemstack\n");
		exit(-1);
	}
	else{
		top = *s;
		*s = ELEMSTACK_NEXT( top);
	}

	e = ELEMSTACK_CURR( top);
	free( top);

	return e;

}
