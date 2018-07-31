/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
/*****************************************************************************
 *
 * file:   lubtree.h
 *
 * description:
 *   header file for lubtree.c
 *
 *****************************************************************************/

#ifndef _LUBTREE_H_
#define _LUBTREE_H_

extern lubinfo * LUBcreatePartitions( dynarray *eulertour);
extern int LUBgetLowestFromCandidates( dynarray *d, int indices[4]);
extern vertex *LUBtreeLCAfromNodes( vertex *n1, vertex *n2, compinfo *ci);

#endif /* _LUBTREE_H_ */
