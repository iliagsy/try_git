/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _TIMING_H_
#define _TIMING_H_
#include "graphtypes.h"
typedef struct test_result {
	int total;
	double cratio;
	double ppt_adaptive;
	double ppt_greedy;
	double ppt_ratio;
	double qt_adaptive;
	double qt_greedy;
	double qt_ratio;
}tr;

void init_result(tr* result);
tr* time_difference(dag *g);
#endif
