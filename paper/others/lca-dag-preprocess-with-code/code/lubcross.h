/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _LUBCROSS_H_
#define _LUBCROSS_H_

matrix* LUBcreateReachMat( compinfo *ci);

matrix* LUBcreatePCPTMat( matrix *reachmat, compinfo *ci);

dynarray * LUBsortInPostorder( compinfo *ci);

void LUBorColumnsAndUpdate( matrix *m1, int colidx1, 
    			    matrix *m2, int colidx2, 
			    matrix *result, int rescolidx);

int LUBisNodeCsrc( vertex *n, dynarray *csrc);

dynarray *LUBrearrangeCsrcOnTopo( dynarray *csrc, dynarray *prearr);

dynarray *LUBrearrangeNoncsrcOnTopo( dynarray *noncsrc);

matrix *LUBrearrangeMatOnTopo( dynarray *topoarr, matrix *mat);

matrix* LUBcreatePCPCMat( matrix *reachmat, dynarray *postarr, compinfo *ci);

void LUBincorporateCrossEdges( compinfo *ci);

#endif
