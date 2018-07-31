/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _GRAPHTYPES_H_
#define _GRAPHTYPES_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

struct ELEM{
  int idx;
  void *data;
};

struct DYNARRAY{
  struct ELEM **elems;
  int totalelems;
  int allocelems;
};

struct MATRIX{
  struct DYNARRAY **array2d;
  int totalrows;
  int totalcols;
};

struct ELEMSTACK{
  struct ELEM *curr;
  struct ELEMSTACK *next;
};

struct ELEMLIST{
  struct ELEM *curr;
  struct ELEMLIST *prev;
  struct ELEMLIST *next;
};

struct ELEMQUEUE{
  struct ELEMLIST *head;
  struct ELEMLIST *tail;
};

struct LUBINFO{
  int numintra;
  int blocksize;
  struct DYNARRAY *blockmin;
  struct MATRIX *intermat;
  struct MATRIX **intramats;
  struct MATRIX *pcptmat;
  struct MATRIX *pcpcmat;
};

struct COMPINFO{
  struct DYNARRAY *csrc;
  struct DYNARRAY *ctar;
  struct DYNARRAY *tltable;
  struct DYNARRAY *eulertour;
  struct DYNARRAY *prearr;
  struct MATRIX *crossclos;
  struct MATRIX *tlc;
  struct LUBINFO *lub;
  struct MATRIX *dist;
  struct VERTICES *topolist;
};


typedef enum {
	edgetree,edgecross,edgeforward,edgeback, edgedefault
} graph_edgetype;
typedef enum {
	tree_labeling,nontree_labeling,edge_labeling
} graph_label_mode;

//typedef enum {vertices,edges} dot_output_mode;
typedef struct ELEM elem;
typedef struct DYNARRAY dynarray;
typedef struct MATRIX matrix;
typedef struct ELEMSTACK elemstack;
typedef struct ELEMLIST elemlist;
typedef struct ELEMQUEUE elemqueue;
typedef struct LUBINFO lubinfo;
typedef struct COMPINFO compinfo;

typedef struct VERTEX{
	char label[400];
	int pre;
	int premax;
	int post;
	int topo;
	int eulerid;
	int depth;
	int numtopovisits;
	int mineqchildvisits;
	int numparents;
	int numchildren;
	int row;
	int reachcola;
	int reachcolb;
	int lubcol;
	int distcol;
	bool isCompRoot;
	bool isTopoVisited;
	bool isDFSVisited;
	bool isRchColaMarked;
	bool isRchColbMarked;
	bool isRowMarked;
	bool isDotVisited;
	struct VERTICES *ancestors;
	struct EDGES *parents;
	struct EDGES *children;
} vertex;

typedef struct VERTICES{
	struct VERTEX *curr;
	struct VERTICES *next;
} vertices;

typedef struct EDGES {
	graph_edgetype edgetype;
	bool wasClassified;
	struct VERTEX *target;
	struct EDGES *next;
} edges;

typedef struct DAG{
	int dirty;
	vertices *vs;
	vertex *top;
	vertex *bottom;
	compinfo *info;
} dag;

#define VERTEX_LABEL(v) ((v)->label)
#define VERTEX_PRE(v) ((v)->pre)
#define VERTEX_PREMAX(v) ((v)->premax)
#define VERTEX_POST(v) ((v)->post)
#define VERTEX_TOPO(v) ((v)->topo)
#define VERTEX_EULERID(v) ((v)->eulerid)
#define VERTEX_DEPTH(v) ((v)->depth)
#define VERTEX_NUMTOPOVISITS(v) ((v)->numtopovisits)
#define VERTEX_MINEQCHILDVISITS(v) ((v)->mineqchildvisits)
#define VERTEX_NUMPARENTS(v) ((v)->numparents)
#define VERTEX_NUMCHILDREN(v) ((v)->numchildren)
#define VERTEX_ROW(v) ((v)->row)
#define VERTEX_REACHCOLA(v) ((v)->reachcola)
#define VERTEX_REACHCOLB(v) ((v)->reachcolb)
#define VERTEX_LUBCOL(v) ((v)->lubcol)
#define VERTEX_DISTCOL(v) ((v)->distcol)
#define VERTEX_ISCOMPROOT(v) ((v)->isCompRoot)
#define VERTEX_ISTOPOVISITED(v) ((v)->isTopoVisited)
#define VERTEX_ISDFSVISITED(v) ((v)->isDFSVisited)
#define VERTEX_ISRCHCOLAMARKED(v) ((v)->isRchColaMarked)
#define VERTEX_ISRCHCOLBMARKED(v) ((v)->isRchColbMarked)
#define VERTEX_ISROWMARKED(v) ((v)->isRowMarked)
#define VERTEX_ISDOTVISITED(v) ((v)->isDotVisited)
#define VERTEX_ANCESTORS(v) ((v)->ancestors)
#define VERTEX_PARENTS(v) ((v)->parents)
#define VERTEX_CHILDREN(v) ((v)->children)

#define EDGES_EDGETYPE(e) ((e)->edgetype)
#define EDGES_WASCLASSIFIED(e) ((e)->wasClassified)
#define EDGES_TARGET(e) ((e)->target)
#define EDGES_NEXT(e) ((e)->next)

#define	VERTICES_CURR(vs) ((vs)->curr)
#define	VERTICES_NEXT(vs) ((vs)->next)

#define DAG_VS(g) ((g)->vs)
#define DAG_TOP(g) ((g)->top)
#define DAG_BOTTOM(g) ((g)->bottom)
#define DAG_INFO(g) ((g)->info)
#define DAG_DIRTY(g) ((g)->dirty)

#define COMPINFO_CSRC(n) ((n)->csrc)
#define COMPINFO_CTAR(n) ((n)->ctar)
#define COMPINFO_TLTABLE(n) ((n)->tltable)
#define COMPINFO_EULERTOUR(n) ((n)->eulertour)
#define COMPINFO_PREARR(n) ((n)->prearr)
#define COMPINFO_CROSSCLOS(n) ((n)->crossclos)
#define COMPINFO_TLC(n) ((n)->tlc)
#define COMPINFO_LUB(n) ((n)->lub)
#define COMPINFO_DIST(n) ((n)->dist)
#define COMPINFO_TOPOLIST(n) ((n)->topolist)

#define LUBINFO_NUMINTRA(n) ((n)->numintra)
#define LUBINFO_BLOCKSIZE(n) ((n)->blocksize)
#define LUBINFO_BLOCKMIN(n) ((n)->blockmin)
#define LUBINFO_INTERMAT(n) ((n)->intermat)
#define LUBINFO_INTRAMATS(n) ((n)->intramats)
#define LUBINFO_INTRAMATS_POS(n, i) ((n)->intramats[i])
#define LUBINFO_PCPTMAT(n) ((n)->pcptmat)
#define LUBINFO_PCPCMAT(n) ((n)->pcpcmat)

extern void freeCompInfo( compinfo *ci);
extern void freeLubInfo( lubinfo *linfo);

dag* makedag ();
vertex* makevertex ();
vertices* makevertices ();
void freevertices (vertices *vs);
edges* makeedges ();
compinfo* makecompinfo ();
lubinfo* makelubinfo ();
edges *freeCurrentEdge ( edges *e);

#endif
