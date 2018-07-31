/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _DYNMATRIX_H_
#define _DYNMATRIX_H_

extern void initMatrix(matrix *m);
matrix *makeMatrix ();
extern void free2DArray(dynarray **d2, int count);
extern void freeMatrix(matrix *m);
extern void setMatrixValue(matrix *m, int x, int y, int value);
extern void setMatrixElem(matrix *m, int x, int y, elem *element);
extern elem *getMatrixElem(matrix *m, int x, int y);
extern int getMatrixValue(matrix *m, int x, int y);

#define MATRIX_ARRAY2D(n) ((n)->array2d)
#define MATRIX_TOTALROWS(n) ((n)->totalrows)
#define MATRIX_TOTALCOLS(n) ((n)->totalcols)

#endif 
