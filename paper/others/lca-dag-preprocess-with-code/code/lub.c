/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
/** <!--********************************************************************-->
 *
 * @file lub.c
 *
 * prefix: TFPLB
 *
 * description: This file calls functions to preprocess the tpye hierarchy graph
 * for answering least upper bound queries. This is done with the aid of a
 * compiler pass.
 *
 *****************************************************************************/

#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphtypes.h"
#include "dfwalk.h"
#include "tfprintutils.h"
#include "lubtree.h"
#include "lubcross.h"
#include "binheap.h"
#include <time.h>

/*
 * INFO structure
 * pre is the depth first walk id for the nodes in the dependency
 * graph. premax is the maximum value of the pre of the tree
 * decendants of a node
 */
typedef struct INFO {
	dynarray *euler;
}info;

/*
 * INFO macros
 */
#define INFO_EULER(n) n->euler

/*
 * INFO functions
 */
static info *MakeInfo( void)
{
	info *result;

	result = (info *) malloc( sizeof( info));
	INFO_EULER( result) = NULL;
	return result;

}

static info *FreeInfo( info *info)
{

	free( info);
	info = NULL;

	return info;
}


/** <!--********************************************************************-->
 *
 * @fn void PLBtravVertex( vertex *v, info *arg_info)
 *
 *   @brief
 *
 *   @param v
 *   @param arg_info
 *
 *   @return
 *
 *****************************************************************************/
void PLBtravVertex( vertex *v, info *arg_info)
{

	edges *children;
	elem *e;

	children = VERTEX_CHILDREN (v);

	if( INFO_EULER( arg_info) == NULL){
		INFO_EULER( arg_info) = makeDynarray ();
	}

	e = makeElem ();
	ELEM_IDX(e) = VERTEX_DEPTH( v);

	/*
	 * ELEM_DATA(e) is a void pointer. So, we have to take this into account while
	 * initialising it.
	 */

	ELEM_DATA(e) = malloc( 2 * sizeof(int));
	((int *) ELEM_DATA(e))[0] = VERTEX_PRE (v);
	((int *) ELEM_DATA(e))[1] = 0;

	addToArray( INFO_EULER( arg_info), e);

	VERTEX_EULERID (v) = DYNARRAY_TOTALELEMS( INFO_EULER( arg_info)) - 1;

	while( children != NULL){

		if( EDGES_EDGETYPE( children) == edgetree){

			PLBtravVertex( EDGES_TARGET( children), arg_info);

			/*
			 * We add the parent vertex once again upon return from the traversal.
			 */

			e = makeElem ();
			ELEM_IDX(e) = VERTEX_DEPTH(v);

			ELEM_DATA(e) = malloc( 2 * sizeof(int));
			((int*) ELEM_DATA(e))[0] = VERTEX_PRE (v);
			((int*) ELEM_DATA(e))[1] = 0;

			addToArray( INFO_EULER( arg_info), e);

		}

		children = EDGES_NEXT(children);

	}

}


/** <!--********************************************************************-->
 *
 * @fn void PLBdoLUBPreprocessing( dag *g)
 *
 *   @brief  Inits the traversal for this phase
 *
 *   @param g
 *   @return
 *
 *****************************************************************************/
void PLBdoLUBPreprocessing(dag *g)
{
	info *arg_info;
	arg_info = MakeInfo();
	compinfo *ci;

	/*
	 * First label nodes for tree reachability
	 */

	INFO_EULER( arg_info) = NULL;

	PLBtravVertex ( DAG_TOP( g ), arg_info);

	ci = DAG_INFO (g);

	COMPINFO_EULERTOUR(ci) = INFO_EULER( arg_info);
	COMPINFO_LUB( ci) = LUBcreatePartitions( COMPINFO_EULERTOUR( ci));

	LUBincorporateCrossEdges( ci);

	//testlubtree( arg_node);
	//testPriorityQueue();


	arg_info = FreeInfo(arg_info);

}


void randNumGen( int max, int* testpre){

	testpre[0] = rand() % (max);
	testpre[1] = rand() % (max);

}

void testPriorityQueue( void) {

	int i, j, random, totalelements;
	dynarray *q;

	srand(time(NULL));

	for( j = 0; j < 10; j++){

		q = makeDynarray ();

		for( i = 0; i < 10; i++){
			random = rand() % 10 + 1;
			PQinsert( random, q);
		}

		PQprint(q);

		totalelements = DYNARRAY_TOTALELEMS(q);

		for( i = 0; i < totalelements; i++){
			printf( "%d,", PQgetMin(q));
			PQdeleteMin(q);
		}

		printf("\n-----------\n");

		freeDynarray(q);
		q = NULL;

	}

}

void testlubtree( dag *g){

	dynarray *prearr;
	int j, nodecount;
	int testpre[2];
	vertex *n1, *n2, *result;

	unsigned int iseed = (unsigned int)time(NULL);
	srand (iseed);

	prearr = COMPINFO_PREARR( DAG_INFO( g));
	nodecount = DYNARRAY_TOTALELEMS( prearr);
	printDepthAndPre (COMPINFO_EULERTOUR( DAG_INFO( g)));
	printLubInfo( DAG_INFO(g));

	for( j = 0; j < nodecount; j++){
		randNumGen( nodecount, testpre);
		n1 = (vertex *) ELEM_DATA( DYNARRAY_ELEMS_POS( prearr, testpre[0]));
		n2 = (vertex *) ELEM_DATA( DYNARRAY_ELEMS_POS( prearr, testpre[1]));
		printf("lub(%d,%d) = ", VERTEX_PRE( n1), VERTEX_PRE( n2));
		result = LUBtreeLCAfromNodes( n1, n2, DAG_INFO( g));
		printf("Result = %d \n", VERTEX_PRE( result));
		fflush(stdout);

	}

}
