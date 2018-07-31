/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _ELEM_H_
#define _ELEM_H_

#include "graphtypes.h"

#define ELEM_IDX(n) ((n)->idx)
#define ELEM_DATA(n) ((n)->data)

extern void initElem( elem *e);
elem *makeElem ();
extern void freeElem( elem *e);

#endif
