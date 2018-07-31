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
 * @file mineq.c
 *
 * prefix: TFMIN
 *
 * description: This file generates a minimum equivalent graph from the
 * subtyping relationships.
 *
 * TODO: This algorithm assumes a single edge between two vertices. If there are
 * multiple edges between two pairs of vertices, the algorithm may trip.
 *
 *****************************************************************************/

#include "graphtypes.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "mineq.h"
#include "graphutils.h"
/** <!--********************************************************************-->
 *
 * @fn void MINtravVertex( vertex *v)
 *
 *   @brief
 *   We walk through the dependency graph here. If we find that for a given
 *   edge, there is an alternate path from the source to the target of the edge,
 *   we identify the edge as a superfluous edge and remove it.
 *
 *   @param v
 *
 *   @return
 *
 *****************************************************************************/
void MINtravVertex( vertex *v){

  edges *parents_itr1, *parents_itr2, *edge;
  vertices *nl_next;
  int children_visited, total_children, inlist = 0;

  parents_itr1 = VERTEX_PARENTS(v);

  while( parents_itr1 != NULL){

    /*
     * Check if the parent belongs to the ancestor list of any of the other
     * parents of the vertex
     */

    parents_itr2 = VERTEX_PARENTS(v);

    while( parents_itr2 != NULL) {

      inlist = GUvertInList( EDGES_TARGET( parents_itr1),
	  		VERTEX_ANCESTORS( EDGES_TARGET( parents_itr2)));

      if( inlist) break;

      parents_itr2 = EDGES_NEXT(parents_itr2);

    }

    if( inlist){

      printf ( "Removing superfluous edge between %s and %s.\n",
		       	VERTEX_LABEL( EDGES_TARGET( parents_itr1)),
	  		VERTEX_LABEL(v));
      edge = parents_itr1;
      parents_itr1 = EDGES_NEXT( parents_itr1);
      GUremoveEdge( EDGES_TARGET( edge), v);

    } else {

      parents_itr1 = EDGES_NEXT( parents_itr1);

    }

  }

  /*
   * Now we should check to find if there are any parent vertices for whom all
   * its children have been visited. In this case, we can free the ancestor set
   * of the vertex.
   */

  parents_itr1 = VERTEX_PARENTS(v);

  while( parents_itr1 != NULL){

    /*
     * Increment the number of children visited during creation of the minimum
     * equivalent graph for each parent.
     */
    children_visited = VERTEX_MINEQCHILDVISITS( EDGES_TARGET( parents_itr1))++;
    total_children = VERTEX_NUMCHILDREN( EDGES_TARGET( parents_itr1));

    /*
     * Build the ancestor list of the node.
     */

    VERTEX_ANCESTORS(v) = GUmergeLists( VERTEX_ANCESTORS(v),
			VERTEX_ANCESTORS( EDGES_TARGET( parents_itr1)));

    nl_next = VERTEX_ANCESTORS(v);
    VERTEX_ANCESTORS (v) = (vertices *) malloc( sizeof(vertices));
    VERTICES_CURR (VERTEX_ANCESTORS (v)) = EDGES_TARGET( parents_itr1);
    VERTICES_NEXT (VERTEX_ANCESTORS (v)) = nl_next;


    /*
     * If the number of children visited equals the total number of children for
     * any of the parents, we are done with that parent and we can free its
     * ancestor list.
     */

    if( children_visited == total_children) {
		freevertices( VERTEX_ANCESTORS( EDGES_TARGET( parents_itr1)));
		VERTEX_ANCESTORS( EDGES_TARGET( parents_itr1)) = NULL;
    }

    parents_itr1 = EDGES_NEXT( parents_itr1);

  }

}


/** <!--********************************************************************-->
 *
 * @fn node *MINdoReduceGraph( dag *g)
 *
 *   @brief  Inits the traversal for this phase
 *
 *   @param g
 *   @return
 *
 *****************************************************************************/
void MINdoReduceGraph(dag *g)
{

  vertices *nl;
  compinfo *ci = DAG_INFO(g);

  if( ci != NULL) {
    nl = COMPINFO_TOPOLIST( ci);
    while(nl!=NULL){
      MINtravVertex (VERTICES_CURR(nl));
      nl = VERTICES_NEXT(nl);
    }
  }

}


