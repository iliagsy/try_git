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
 * @file dag.c
 *
 * prefix: DAG
 *
 * description:
 *
 *****************************************************************************/

#include "graphtypes.h"
#include "dfwalk.h"
#include "dynelem.h"
#include "elemstack.h"
#include "dynarray.h"
#include "dynmatrix.h"
#include "dfwalk.h"
#include "topo.h"
#include "mineq.h"
#include "ctransitive.h"
#include "reachlabel.h"
#include "lub.h"
#include "tfprintutils.h"
#include "lubtree.h"
#include "lubcross.h"
#include "binheap.h"
#include "query.h"
#include <string.h>


dag* DAGgenGraph( void){
  dag *g;
  g = makedag();
  g->dirty = 1;
  return g;
}

static bool vlookup(dag *g, char *label){
  vertices *vs = g->vs;
  if (vs == NULL) return false;
  while( vs != NULL ){
    if( strcmp((vs->curr)->label, label) == 0 )
    	return true;
    vs = vs->next;
  }
  return false;
}


vertex* getVertexFromLabel (dag *g, char *label){
  vertices *vs = g->vs;
  if (vs == NULL) return false;
  while( vs != NULL ){
    if( strcmp((vs->curr)->label, label) == 0 )
    	return vs->curr;
    vs = vs->next;
  }
  return NULL;
}

vertex* DAGaddVertex(dag *g, char *label){

	if (vlookup (g, label)) {
		printf ("Vertex %s already exists\n", label);
		//exit(-1);
		return NULL;
	}

	vertex *v = makevertex ();
	strcpy (v->label, label);

	if( g->vs == NULL){
		g->vs = makevertices ();
		(g->vs)->curr = v;
	}
	else {
		vertices *vs = g->vs;
		while (vs->next!=NULL){
			vs = vs->next;
		}
		vs->next = makevertices ();
		vs = vs->next;
		vs->curr = v;
	}

	g->dirty = 1;

	return v;

}

static bool edgeExists (vertex *super, vertex *sub){

	edges *children = super->children;
	while (children!=NULL){
		if (children->target->label == sub->label)
			return true;
		children = children->next;
	}
	return false;

}

static void createEdge (vertex *super, vertex *sub){

	edges *children, *parents;

	if (edgeExists (super, sub)) {
		//fprintf (stderr, "Edge exists between %s and %s\n",
		//		super->label, sub->label);
	} else {
		children = super->children;
		if (children == NULL){
			super->children = makeedges ();
			super->children->target = sub;
		} else {
			while( children->next != NULL){
				children = children->next;
			}
			children->next = makeedges ();
			children = children->next;
			children->target = sub;
		}
		super->numchildren++;

		parents = sub->parents;
		if( parents == NULL){
			sub->parents = makeedges ();
			sub->parents->target = super;
		} else {
			while (parents->next != NULL){
				parents = parents->next;
			}
			parents->next = makeedges ();
			parents = parents->next;
			parents->target = super;
		}
		sub->numparents++;
	}

}

void DAGaddEdge (dag *g, vertex *from, vertex *to){

	bool s_exists = vlookup (g, from->label);
	bool t_exists = vlookup (g, to->label);

	if (!s_exists || !t_exists) {
		printf ("Source or target vertex non-existent\n");
		exit(-1);
	} else {
		createEdge(from, to);
		g->dirty = 1;
	}

}

void preprocessDAG( dag *g){

  int root_count=0;

  /* Assign a root node for the DAG */
  vertices *vs = DAG_VS(g);
  while( vs != NULL){
    if (VERTEX_PARENTS (VERTICES_CURR (vs)) == NULL)
    	root_count++;
    if( root_count > 1) {
    	printf ("DAG has multiple roots\n");
    	exit (-1);
    }
    /*else DAG_TOP (g) = VERTICES_CURR (vs);*/
    vs = VERTICES_NEXT (vs);
  }

  /* preprocess the dag */

  //topological walk and dfwalk now called in main function

  //TOPdoTopoSort(g);
  /*In a->b and a->c->b, MINdoReduceGraph is removing c->b
   * which is a cross edge. We dont want this, hence commenting out.
   * */
  //MINdoReduceGraph(g);
  //DFWdoDFWalk(g);
  CTRdoCrossClosure(g);
  RCHdoReachabilityAnalysis(g);
  PLBdoLUBPreprocessing(g);

  g->dirty = 0;

}

vertex* DAGgetLub(dag *g, vertex *v1, vertex *v2){
  if(DAG_DIRTY(g)) preprocessDAG(g);
  vertex *res = GINlcaFromNodes(v1, v2, DAG_INFO(g));
  return res;
}
