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
 * prefix: TFDFW
 *
 * description: depth first walk of the subtyping hierarchy
 *
 *****************************************************************************/
#include "graphtypes.h"
#include "dfwalk.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "graphtypes.h"
#include "dfwalk.h"
#include "tfprintutils.h"

/*
 * INFO structure
 * pre is the depth first walk id for the nodes in the dependency
 * graph. premax is the maximum value of the pre of the tree
 * decendants of a node
 */
typedef struct INFO {
  int pre;
  int post;
  dynarray *prearr;
}info;

/*
 * INFO macros
 */
#define INFO_PRE(n) n->pre
#define INFO_POST(n) n->post
#define INFO_PREARR(n) n->prearr

/*
 * INFO functions
 */
static info *MakeInfo( void)
{
  info *result;

  result = (info *) malloc( sizeof( info));
  INFO_PRE(result) = 1;
  INFO_POST(result) = 1;
  INFO_PREARR( result) = NULL;

  return result;

}

static info *FreeInfo( info *info)
{

  free( info);

  return info;
}

/** <!--********************************************************************-->
 *
 * @fn void DFWtravVertex( vertex *v, info *arg_info)
 *
 *   @brief
 *   We walk through the dependency graph here. If the node has not
 *   been visited i.e. its pre is 0, we update the pre of
 *   the node. Then, we check the children (subtypes) of the def and if
 *   they are not visited, we visit them.
 *
 *   @param v
 *   @param arg_info
 *
 *   @return
 *
 *****************************************************************************/
void DFWtravVertex( vertex *v, info *arg_info)
{

  edges *children;
  elem *e;

  children = VERTEX_CHILDREN( v);
  VERTEX_PRE(v) = INFO_PRE( arg_info)++;

  if( INFO_PREARR( arg_info) == NULL){
    INFO_PREARR( arg_info) = makeDynarray();
  }

  e = (elem *) malloc( sizeof( elem));
  ELEM_IDX(e) = VERTEX_PRE(v);
  ELEM_DATA(e) = v;
  addToArray( INFO_PREARR( arg_info), e);

  while( children != NULL){

    if( VERTEX_PRE( EDGES_TARGET( children)) == 0){
      /*
       * Tree branch
       */
      EDGES_EDGETYPE( children) = edgetree;
      EDGES_WASCLASSIFIED( children) = 1;

      VERTEX_DEPTH( EDGES_TARGET( children))
      =  VERTEX_DEPTH( v) + 1;

      DFWtravVertex( EDGES_TARGET( children), arg_info);

    }

    children = EDGES_NEXT(children);

  }

  /*
   * We have traversed all descendants of this node. Its time to
   * update the value of premax which is the maximum value of the
   * pre of all tree descendant of this node
   */

  VERTEX_PREMAX( v) = INFO_PRE( arg_info);
  VERTEX_POST( v) = INFO_POST( arg_info)++;

}


/** <!--********************************************************************-->
 *
 * @fn void DFWdoDFWalk( dag *g)
 *
 *   @brief  Inits the traversal for this phase
 *
 *   @param g
 *   @return
 *
 *****************************************************************************/
void DFWdoDFWalk(dag *g)
{
  info *arg_info;

  arg_info = MakeInfo();

  /*
   * First label nodes for tree reachability
   */

  INFO_PREARR( arg_info) = NULL;

  DFWtravVertex( DAG_TOP(g), arg_info);

  COMPINFO_PREARR( DAG_INFO(g))
    = INFO_PREARR( arg_info);

  arg_info = FreeInfo(arg_info);

}
