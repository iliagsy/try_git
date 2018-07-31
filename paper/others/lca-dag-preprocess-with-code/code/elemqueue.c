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
#include "elemqueue.h"
#include "elemlist.h"

void EQenqueue( elemqueue *q, elem *e){

	elemlist *head, *el;

	head = ELEMQUEUE_HEAD( q);

	el = (elemlist *) malloc( sizeof ( elemlist));
	ELinit( el);

	ELEMLIST_CURR( el) = e;
	ELEMLIST_NEXT( el) = head;

	ELEMLIST_PREV( head) = el;

	ELEMQUEUE_HEAD( q) = el;

}

elem * EQdequeue( elemqueue *q){

	elemlist *tail, *prev;
	elem *e;

	tail = ELEMQUEUE_TAIL(q);
	prev = ELEMLIST_PREV( tail);

	ELEMLIST_NEXT( prev) = NULL;
	e = ELEMLIST_CURR( tail);

	tail = ELfreeNonRecursive( tail);

	return e;

}
