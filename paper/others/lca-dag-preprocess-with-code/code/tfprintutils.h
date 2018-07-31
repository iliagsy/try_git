/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _TFPRINTUTILS_H_
#define _TFPRINTUTILS_H_

extern void printDynarray( dynarray *arrayd);
extern void printMatrix( matrix *m);
extern void printMatrixInDotFormat( FILE *fp, matrix *m);
extern void printTransitiveLinkTable( dynarray *arrayd);
extern void printDepthAndPre( dynarray *d);
extern void printLubInfo( compinfo *ci);
void printTLCmatrix( matrix *tlc, dynarray *csrc, dynarray *ctar);
void printLUBMatrix( matrix *m);
void recursiveDump(FILE *fp, vertex *v, char *cross);
void dumpDAG(dag *g);

#endif
