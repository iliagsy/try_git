/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _DYNARRAY_H_
#define _DYNARRAY_H_

#include "graphtypes.h"

#define DYNARRAY_ELEMS(n) ((n)->elems)
#define DYNARRAY_ELEMS_POS(n,i) ((n)->elems[i])
#define DYNARRAY_TOTALELEMS(n) ((n)->totalelems)
#define DYNARRAY_ALLOCELEMS(n) ((n)->allocelems)

extern void initDynarray(dynarray *arrayd);
dynarray * makeDynarray ();
extern void freeElemArray( elem **e, int count);
extern void freeDynarray( dynarray *arrayd);
extern int addToArray( dynarray *arrayd, elem *item);
extern int indexExistsInArray( dynarray *arrayd, int idx);
int getPositionInArray( dynarray *arrayd, int idx);
extern int addToArrayAtPos( dynarray *arrayd, elem *item, int pos);
extern void merge( elem **elems, int lower, int upper, int desc);
extern void sortArray( elem **elems, int lower, int upper, int desc);
elem* getElemFromArray( dynarray *arrayd, int idx);
void freeDynarrayShallow( dynarray *arrayd);

#endif
