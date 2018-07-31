/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#ifndef _REACHHELPER_H_
#define _REACHHELPER_H_


dynarray *buildTransitiveLinkTable(
		dynarray *arrayd, dynarray *csrc,
		dynarray *ctar, dynarray *prearr);

extern void setSrcTarArrays( dynarray *arrayd, 
    		      	     dynarray** arrX, 
			     dynarray** arrY);

extern matrix* computeTLCMatrix( dynarray *arrayd, 
    				 dynarray* arrX, 
				 dynarray* arrY);

#endif
