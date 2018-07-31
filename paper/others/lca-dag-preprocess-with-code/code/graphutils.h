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
 * file:   graphutils.h
 *
 * description:
 *   header file for graphutils.c
 *
 *****************************************************************************/

#ifndef _GRAPHUTILS_H_
#define _GRAPHUTILS_H_
#include <stdbool.h>
#include "graphtypes.h"

bool GUvertInList( vertex *n, vertices *nl);
vertices* GUmergeLists( vertices *nla, vertices *nlb);
void GUremoveEdge( vertex *src, vertex *tar);

#endif /* _GRAPHUTILS_H_ */
