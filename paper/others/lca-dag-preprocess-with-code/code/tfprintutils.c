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
#include "dynarray.h"
#include "dynmatrix.h"
#include "tfprintutils.h"
#include <string.h>

void printDynarray( dynarray *arrayd){

	int i;

	printf( "[");

	for( i = 0; i < DYNARRAY_TOTALELEMS( arrayd); i++){
		printf( "%d,", ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, i)));
	}

	printf( "]\n");

}

void printEuler( dynarray *arrayd){

	int i;

	printf( "[");

	for( i = 0; i < DYNARRAY_TOTALELEMS( arrayd); i++){
		printf( "(%d,",i);
		printf( "%d,", ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, i)));
		printf( "%d)", *(int *)ELEM_DATA( DYNARRAY_ELEMS_POS( arrayd, i)));
	}

	printf( "]\n");

}



void printLUBMatrix( matrix *m){

	int i, j;
	dynarray *arrayd;

	dynarray **array2d = MATRIX_ARRAY2D(m);

	printf("\n");

	for( i = 0; i < MATRIX_TOTALROWS(m); i++){

		arrayd = array2d[i];

		if( arrayd != NULL){

			for( j = 0; j < DYNARRAY_TOTALELEMS( arrayd); j++){

				if( DYNARRAY_ELEMS_POS( arrayd, j) != NULL){
					printf( "%d\t", ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, j)));
					fflush(stdout);
				} else printf("-\t");

			}

			for( j = DYNARRAY_TOTALELEMS(arrayd); j < MATRIX_TOTALCOLS(m); j++){
				printf("-\t");
			}

		} else {

			for( j = 0; j < MATRIX_TOTALCOLS(m); j++) printf("-\t");

		}

		printf("\n");

	}

}

void printMatrix( matrix *m){

	int i, j;
	dynarray *arrayd;

	dynarray **array2d = MATRIX_ARRAY2D(m);

	printf("\n");

	for( i = 0; i < MATRIX_TOTALROWS(m); i++){

		arrayd = array2d[i];

		if( arrayd != NULL){

			for( j = 0; j < DYNARRAY_TOTALELEMS( arrayd); j++){

				if( DYNARRAY_ELEMS_POS( arrayd, j) != NULL){
					printf( "%d,", ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, j)));
					fflush(stdout);
				} else printf("-,");

			}

			for( j = DYNARRAY_TOTALELEMS(arrayd); j < MATRIX_TOTALCOLS(m); j++){
				printf("-,");
			}

		} else {

			for( j = 0; j < MATRIX_TOTALCOLS(m); j++) printf("-,");

		}

		printf("\n");

	}

}

void printMatrixInDotFormat( FILE *outfile, matrix *m){

	int i, j;
	static int id = 0;
	dynarray **array2d = MATRIX_ARRAY2D(m);
	dynarray *arrayd;

	fprintf( outfile, "struct%d [label=\"", id++);

	for(i = 0; i < MATRIX_TOTALROWS(m); i++){

		arrayd = array2d[i];

		if(arrayd != NULL){

			fprintf( outfile,"{");

			for(j = 0; j < DYNARRAY_TOTALELEMS(arrayd); j++){

				if( DYNARRAY_ELEMS_POS( arrayd, j) != NULL){

					fprintf( outfile, "%d", ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, j)));

				} else {

					fprintf( outfile, "-");

				}

				if(j != DYNARRAY_TOTALELEMS( arrayd) - 1) printf("|");

			}

			for( j = DYNARRAY_TOTALELEMS( arrayd); j < MATRIX_TOTALCOLS(m); j++){

				fprintf( outfile, "-");
				if( j != MATRIX_TOTALCOLS(m) - 1) fprintf( outfile, "|");

			}

		} else {

			for( j = 0; j < MATRIX_TOTALCOLS(m); j++){

				fprintf(outfile, "-");
				if( j != MATRIX_TOTALCOLS(m) - 1) fprintf( outfile, "|");

			}

		}

		fprintf( outfile, "}");

		if( i != MATRIX_TOTALROWS(m) - 1) fprintf( outfile, "|");

	}

	fprintf( outfile, "\"];\n");

}

void printTransitiveLinkTable( dynarray *arrayd){

	int i;

	for( i = 0; i < DYNARRAY_TOTALELEMS( arrayd); i++){

		printf( "%d->[%d,%d)\n", ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd, i)),
				*(( int *) ELEM_DATA( DYNARRAY_ELEMS_POS( arrayd, i))),
				*(( int *) ELEM_DATA( DYNARRAY_ELEMS_POS( arrayd, i)) + 1));

	}

}

void printDepthAndPre( dynarray *d){

	if (d==NULL) {
		printf ("Cannot print information for a NULL array");
		exit(-1);
	}

	int i;

	printf("\n----------\n");

	for( i = 0 ; i < DYNARRAY_TOTALELEMS(d); i++){
		printf("{%d,", *(int *)ELEM_DATA( DYNARRAY_ELEMS_POS( d ,i)));
		printf("%d} ", ELEM_IDX( DYNARRAY_ELEMS_POS( d ,i)));
	}

	printf("\n----------\n");

}

void printPCPTMat( matrix *pcptmat, dynarray *csrc, dynarray *ctar){

	printf( "\n");
	printf( "PCPT Matrix \n");
	printf( "----------- \n");

	int i,j;
	elem *e;
	int lower, upper;

	for( i = -1; i < DYNARRAY_TOTALELEMS( ctar); i++) {

		if( i == -1){

			printf( "| \t");

			for( j = -1; j < DYNARRAY_TOTALELEMS( csrc); j++){
				if (j==-1){
					printf( "|1\t\t");
				} else {
					printf( "| %d\t\t",
						ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, j)));
				}
			}

		}
		else {

			for( j = -1; j < DYNARRAY_TOTALELEMS( csrc) + 1; j++){

				if( j == -1) {

					printf( "| %d\t",
							ELEM_IDX( DYNARRAY_ELEMS_POS( ctar, i)));

				} else {

					e = getMatrixElem( pcptmat, i, j);

					lower = ((int *)ELEM_DATA( e))[0];
					upper = ((int *)ELEM_DATA( e))[1];

					printf( "| (%d, %d)\t", lower, upper);

				}

			}

		}

		printf( "|\n");

	}

}

void printPCPCMat( matrix *pcpcmat, dynarray *ctar){

	int i,j;

	printf( "\n");
	printf( "PCPC Matrix \n");
	printf( "----------- \n");

	for( i = -1; i < DYNARRAY_TOTALELEMS( ctar); i++) {

		if( i == -1){

			printf( "| \t");

			for( j = 0; j < DYNARRAY_TOTALELEMS( ctar); j++){

				printf( "| %d\t",
						ELEM_IDX( DYNARRAY_ELEMS_POS( ctar, j)));

			}

		} else {

			for( j = -1; j < DYNARRAY_TOTALELEMS( ctar); j++){

				if( j == -1) {

					printf( "| %d\t",
							ELEM_IDX( DYNARRAY_ELEMS_POS( ctar, i)));

				} else {

					printf( "| %d\t",
							getMatrixValue( pcpcmat, i, j));

				}

			}

		}

		printf( "|\n");

	}

}

/*
 * TLC matricx is stored as rows indicating cross sources and columns
 * indicating cross targets. But here we try to print in a transverse
 * manner.
 */

void printTLCmatrix( matrix *tlc, dynarray *csrc, dynarray *ctar){

	int i,j;

	printf( "\n");
	printf( "TLC Matrix \n");
	printf( "----------- \n");

	for( i = -1; i < DYNARRAY_TOTALELEMS( ctar); i++) {

		if( i == -1){

			printf( "| \t");

			for( j = 0; j < DYNARRAY_TOTALELEMS( csrc); j++){

				printf( "| %d\t",
						ELEM_IDX( DYNARRAY_ELEMS_POS( csrc, j)));

			}

		} else {

			for( j = -1; j < DYNARRAY_TOTALELEMS( csrc); j++){

				if( j == -1) {

					printf( "| %d\t",
							ELEM_IDX( DYNARRAY_ELEMS_POS( ctar, i)));

				} else {

					printf( "| %d\t",
							getMatrixValue( tlc, j, i));

				}

			}

		}

		printf( "|\n");

	}

}

void printLubInfo( compinfo *ci){

	lubinfo *linfo;

	int i;

	if (COMPINFO_TLTABLE (ci) != NULL) {

	printTransitiveLinkTable (COMPINFO_TLTABLE (ci));

	printTLCmatrix (COMPINFO_TLC (ci), COMPINFO_CSRC( ci),
			COMPINFO_CTAR( ci));
	}

	if (COMPINFO_EULERTOUR (ci) != NULL) printEuler (COMPINFO_EULERTOUR (ci));


	linfo = COMPINFO_LUB( ci);

	if( linfo != NULL) {

		if( LUBINFO_BLOCKMIN( linfo) != NULL){
			printDepthAndPre( LUBINFO_BLOCKMIN( linfo));
		}

		if( LUBINFO_INTRAMATS( linfo) != NULL){

			for( i = 0; i < LUBINFO_NUMINTRA( linfo); i++){

				if( LUBINFO_INTRAMATS_POS( linfo, i) != NULL){
					printMatrix( LUBINFO_INTRAMATS_POS( linfo, i));
					printf("--\n");
				}

			}

		}

		if( LUBINFO_INTERMAT( linfo) != NULL){
			printMatrix( LUBINFO_INTERMAT( linfo));
		}

		if( LUBINFO_PCPTMAT( linfo) != NULL){
			printPCPTMat( LUBINFO_PCPTMAT( linfo), COMPINFO_CSRC( ci),
					COMPINFO_CTAR( ci));
		}

		if( LUBINFO_PCPCMAT( linfo) != NULL){
			printPCPCMat( LUBINFO_PCPCMAT( linfo), COMPINFO_CTAR( ci));
		}

	}
}


/*
 * We dump cross edges at the very end;
 */

void recursiveDump(FILE *fp, vertex *v, char *cross){
	if (VERTEX_ISDOTVISITED(v))  return;
	edges *children = VERTEX_CHILDREN (v);
	char str[80];
	while (children!=NULL) {
		if (EDGES_EDGETYPE(children) != edgetree) {
			sprintf(str, "\"%d, %d\" -> \"%d, %d\" [style=dotted];\n",
					VERTEX_PRE(v), VERTEX_TOPO(v),
					VERTEX_PRE (EDGES_TARGET (children)),
					VERTEX_TOPO (EDGES_TARGET (children)));
			//printf ("%s\n",str);
			strcat (cross, str);
		} else {
			fprintf(fp, "\"%d, %d\" -> \"%d, %d\";\n", VERTEX_PRE(v),
					VERTEX_TOPO(v), VERTEX_PRE (EDGES_TARGET (children)),
					VERTEX_TOPO (EDGES_TARGET (children)));
			recursiveDump(fp, EDGES_TARGET(children), cross);
		}
		children = EDGES_NEXT (children);
	}
	VERTEX_ISDOTVISITED (v) = true;
}


void dumpDAG(dag *g){
	FILE *fp = fopen("dotfile", "w");
	char cross[100000];
	cross[0] = '\0';
	fprintf (fp, "digraph dag{\n");
	recursiveDump (fp, DAG_TOP(g), cross);
	//printf ("%s\n", cross);
	if (cross[0] != '\0')
		fprintf (fp, "%s}\n", cross);
	else
		fprintf(fp, "}\n");
	fclose (fp);

}

