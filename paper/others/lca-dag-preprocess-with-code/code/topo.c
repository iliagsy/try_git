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
 * @file dfwalk.c
 *
 * prefix: TFTOP
 *
 * description: depth first walk of the subtyping hierarchy
 *
 *****************************************************************************/

#include "dfwalk.h"
#include "graphutils.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphtypes.h"
#include "topo.h"

/*
 * INFO structure
 * pre is the topological id for the vertices in the dependency graph.
 */

typedef struct INFO {
  int topo;
  vertices *head;
  vertices *list;
}info;

/*
 * INFO macros
 */
#define INFO_TOPO(n) n->topo
#define INFO_HEAD(n) n->head
#define INFO_LIST(n) n->list

/*
 * INFO functions
 */
static info *MakeInfo( void)
{
  info *result;
  result = (info *) malloc( sizeof( info));
  INFO_TOPO(result) = 1;
  INFO_HEAD(result) = NULL;
  INFO_LIST(result) = NULL;
  return result;

}

static info *FreeInfo( info *info)
{
  free( info);
  return info;
}

/** <!--********************************************************************-->
 *
 * @fn void TOPtravVertex( vertex *v, info *arg_info)
 *
 *   @brief
 *   We walk through the dependency graph here to check if the number of parents
 *   of a vertex equals the number of times its visited in the traversal of the
 *   DAG. If it is the same, then the topological number of the vertex can be
 *   done.
 *
 *   @param v
 *   @param arg_info
 *
 *   @return
 *
 *****************************************************************************/
void TOPtravVertex( vertex *v, info *arg_info)
{

  edges *children;

  children = VERTEX_CHILDREN (v);
  VERTEX_TOPO (v) = INFO_TOPO( arg_info)++;

  if( INFO_HEAD( arg_info) == NULL){

    /*
     * We also maintain a list of topologically sorted vertices for future
     * processing here.
     */

    INFO_HEAD( arg_info) = makevertices();
    INFO_LIST( arg_info) = INFO_HEAD( arg_info);

    VERTICES_CURR( INFO_HEAD( arg_info)) = v;

  } else if( VERTICES_NEXT( INFO_LIST( arg_info)) == NULL) {

    VERTICES_NEXT( INFO_LIST( arg_info)) = makevertices();
    INFO_LIST( arg_info) = VERTICES_NEXT( INFO_LIST( arg_info));
    VERTICES_CURR( INFO_LIST( arg_info)) = v;
    VERTICES_NEXT( INFO_LIST( arg_info)) = NULL;

  }

  while( children != NULL){

    /*
     * Check if the number of visits in the topological walk equals the number
     * of parents for the vertex. If so, topological numbering can proceed for
     * the vertex.
     */

    if( VERTEX_NUMPARENTS( EDGES_TARGET( children)) ==
        ++VERTEX_NUMTOPOVISITS( EDGES_TARGET( children))){
      TOPtravVertex (EDGES_TARGET( children), arg_info);
    }

    children = EDGES_NEXT(children);

  }

}

/** <!--********************************************************************-->
 *
 * @fn void TFTOPdoTopoSort( dag *g)
 *
 *   @brief  Inits the traversal for this phase
 *
 *   @param g
 *   @return
 *
 *****************************************************************************/
void TOPdoTopoSort(dag *g)
{
  info *arg_info;

  arg_info = MakeInfo();

  TOPtravVertex( g->top, arg_info);
  if (g->info == NULL){
	  g->info = makecompinfo();
  }
  g->info->topolist = INFO_HEAD( arg_info);

  arg_info = FreeInfo(arg_info);

}

