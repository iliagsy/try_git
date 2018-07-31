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
#include "graphtypes.h"

dag* makedag (){
	dag *g = (dag *) malloc (sizeof (dag));
	g->dirty = 1;
	g->vs = NULL;
	g->top = NULL;
	g->bottom = NULL;
	g->info = NULL;
	return g;
}

vertex* makevertex (){
	vertex *v = (vertex *) malloc (sizeof (vertex));
	v->label[0] = '\0';
	v->pre = 0;
	v->premax = 0;
	v->post = 0;
	v->topo = 0;
	v->eulerid = 0;
	v->depth = 0;
	v->numtopovisits = 0;
	v->mineqchildvisits = 0;
	v->numparents = 0;
	v->numchildren = 0;
	v->row = 0;
	v->reachcola = 0;
	v->reachcolb = 0;
	v->lubcol = 0;
	v->distcol = 0;
	v->isCompRoot = false;
	v->isTopoVisited = false;
	v->isDFSVisited = false;
	v->isRchColaMarked = false;
	v->isRchColbMarked = false;
	v->isRowMarked = false;
	v->isDotVisited = false;
	v->ancestors = NULL;
	v->parents = NULL;
	v->children = NULL;
	return v;
}

vertices* makevertices (){

	vertices *vs = (vertices *) malloc (sizeof (vertices));
	vs->curr = NULL;
	vs->next = NULL;
	return vs;

}

void freevertices (vertices *vs) {
	vertices *vs1 = vs, *vs2=vs;
	/*
	 * Note: shallow free
	 */
	while (vs1!=NULL) {
		vs2=vs1;
		vs1=vs1->next;
		free(vs2);
	}
	//free (vs1);
}

edges* makeedges (){

	edges *es = (edges *) malloc (sizeof (edges));
	es->edgetype = edgedefault;
	es->wasClassified = false;
	es->target = NULL;
	es->next = NULL;
	return es;

}

compinfo* makecompinfo (){
	compinfo *ci = (compinfo *) malloc (sizeof (compinfo));
	ci->csrc = NULL;
	ci->ctar = NULL;
	ci->tltable = NULL;
	ci->eulertour = NULL;
	ci->prearr = NULL;
	ci->crossclos = NULL;
	ci->tlc = NULL;
	ci->lub = NULL;
	ci->dist = NULL;
	ci->topolist = NULL;
	return ci;
}

lubinfo* makelubinfo (){
	lubinfo *li = (lubinfo *) malloc (sizeof (lubinfo));
	li->numintra = 0;
	li->blocksize = 0;
	li->blockmin = NULL;
	li->intermat = NULL;
	li->intramats = NULL;
	li->pcptmat = NULL;
	li->pcpcmat = NULL;
	return li;
}


edges *freeCurrentEdge ( edges *e){
	edges *to_free = e;
	e = e->next;
	free (to_free);
	return e;
}

void freeLubInfo( lubinfo *linfo){

	int i;

	if( linfo != NULL) {

		if( LUBINFO_BLOCKMIN( linfo) != NULL){
			freeDynarray( LUBINFO_BLOCKMIN( linfo));
		}

		if( LUBINFO_INTRAMATS( linfo) != NULL){

			for( i = 0; i < LUBINFO_NUMINTRA( linfo); i++){

				if( LUBINFO_INTRAMATS_POS( linfo, i) != NULL){
					freeMatrix( LUBINFO_INTRAMATS_POS( linfo, i));
				}

			}

		}

		if( LUBINFO_INTERMAT( linfo) != NULL){
			freeMatrix( LUBINFO_INTERMAT( linfo));
		}

		if( LUBINFO_PCPTMAT( linfo) != NULL){
			freeMatrix( LUBINFO_PCPTMAT( linfo));
		}

		if( LUBINFO_PCPCMAT( linfo) != NULL){
			freeMatrix( LUBINFO_PCPCMAT( linfo));
		}

		free( linfo);

	}

}

void freeCompInfo( compinfo *ci){
	vertices *vs;

	if( ci != NULL){

		if( COMPINFO_CSRC( ci)!=NULL){
			freeDynarray( COMPINFO_CSRC( ci));
		}

		if( COMPINFO_CTAR( ci)!=NULL){
			freeDynarray( COMPINFO_CTAR( ci));
		}

		if( COMPINFO_TLTABLE( ci)!=NULL){
			freeDynarray( COMPINFO_TLTABLE( ci));
		}

		if( COMPINFO_PREARR( ci)!=NULL){
			//freeDynarray( COMPINFO_PREARR( ci));
		}

		if( COMPINFO_EULERTOUR( ci)!=NULL){
			freeDynarray( COMPINFO_EULERTOUR( ci));
		}

		if( COMPINFO_CROSSCLOS( ci)!=NULL){
			freeMatrix( COMPINFO_CROSSCLOS( ci));
		}

		if ( COMPINFO_TLC( ci) != NULL){
			freeMatrix( COMPINFO_TLC( ci));
		}

		if ( COMPINFO_LUB( ci) != NULL){
			freeLubInfo( COMPINFO_LUB( ci));
		}

		if ( COMPINFO_DIST(ci) != NULL){
			freeMatrix( COMPINFO_DIST( ci));
		}

		while( COMPINFO_TOPOLIST( ci) != NULL){
			vs = COMPINFO_TOPOLIST( ci);
			COMPINFO_TOPOLIST( ci) =
					VERTICES_NEXT( COMPINFO_TOPOLIST( ci));
			free( vs);
		}

		free( ci);

	}

}

