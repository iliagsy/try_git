/**************************************************************************
This code is copyright (C) (2015) by the University of Hertfordshire and 
is made available to third parties for research or private study, criticism 
or review, and for the purpose of reporting on the state of the art, under 
the normal fair use/fair dealing exceptions in Sections 29 and 30 of the 
Copyright, Designs and Patents Act 1988. Use of the code under this
provision is limited to non-commercial use: commercial use requires
an appropriate licence from the University of Hertfordshire. 
**************************************************************************/
#include "graphutils.h"
#include "dynelem.h"
#include "dynarray.h"

void initDynarray(dynarray *arrayd){

	DYNARRAY_ELEMS( arrayd) = NULL;
	DYNARRAY_TOTALELEMS( arrayd) = 0;
	DYNARRAY_ALLOCELEMS( arrayd) = 0;

}

dynarray * makeDynarray (){
	dynarray *d = (dynarray *) (malloc(sizeof(dynarray)));
	initDynarray(d);
	return d;
}

void freeElemArray( elem **e, int count){

	int i;

	if(e != NULL){

		for( i = 0; i < count; i++){

			if( e[i] != NULL) {
				freeElem( e[i]);
				e[i] = NULL;
			}

		}

		free(e);
		e=NULL;

	}

}

void freeDynarray( dynarray *arrayd){

	if( arrayd != NULL){

		int i;

		for( i = 0; i < DYNARRAY_ALLOCELEMS( arrayd); i++){

			if( DYNARRAY_ELEMS( arrayd)[i] != NULL){

				freeElem( DYNARRAY_ELEMS( arrayd)[i]);
				DYNARRAY_ELEMS( arrayd)[i] = NULL;

			}

		}

		free(arrayd);
		arrayd=NULL;

	}

}


void freeDynarrayShallow( dynarray *arrayd){

	if( arrayd != NULL){

		int i;

		for( i = 0; i < DYNARRAY_ALLOCELEMS( arrayd); i++){

			if( DYNARRAY_ELEMS( arrayd)[i] != NULL){

				//freeElem( DYNARRAY_ELEMS( arrayd)[i]);
				DYNARRAY_ELEMS( arrayd)[i] = NULL;

			}

		}

		free(arrayd);
		arrayd=NULL;

	}

}

int addToArray( dynarray *arrayd, elem *item){

	int pos, oldsize;

	if(DYNARRAY_TOTALELEMS( arrayd) == DYNARRAY_ALLOCELEMS( arrayd)){

		oldsize = DYNARRAY_ALLOCELEMS( arrayd);
		DYNARRAY_ALLOCELEMS( arrayd) += 3;

		void *_temp = realloc( DYNARRAY_ELEMS( arrayd),
				(DYNARRAY_ALLOCELEMS( arrayd) * sizeof( elem *))/*,
				oldsize * sizeof( elem *)*/);

		if ( !_temp){
			printf ( "addToArray couldn't realloc memory!\n");
			exit (-1);
		}

		/*free( DYNARRAY_ELEMS( arrayd));*/
		DYNARRAY_ELEMS( arrayd) = ( elem**)_temp;

	}

	pos = DYNARRAY_TOTALELEMS( arrayd);
	DYNARRAY_TOTALELEMS( arrayd)++;
	DYNARRAY_ELEMS_POS( arrayd, pos) = item;

	return DYNARRAY_TOTALELEMS( arrayd);

}

int indexExistsInArray( dynarray *arrayd, int idx){

	int i;

	for( i = 0; i < DYNARRAY_TOTALELEMS( arrayd); i++){

		if( idx == ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd,i))){
			return 1;
		}

	}

	return 0;

}

elem* getElemFromArray( dynarray *arrayd, int idx){

	int i;

	for( i = 0; i < DYNARRAY_TOTALELEMS( arrayd); i++){

		if( idx == ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd,i))){
			return DYNARRAY_ELEMS_POS( arrayd,i);
		}

	}

	return NULL;

}

int getPositionInArray( dynarray *arrayd, int idx){

	int i;

	for( i = 0; i < DYNARRAY_TOTALELEMS( arrayd); i++){

		if( idx == ELEM_IDX( DYNARRAY_ELEMS_POS( arrayd,i))){
			return i;
		}

	}

	return -1;

}

int addToArrayAtPos( dynarray *arrayd, elem *item, int pos){

	int i, oldsize = DYNARRAY_TOTALELEMS( arrayd);

	if( pos >= DYNARRAY_ALLOCELEMS( arrayd)){

		//int oldsize = DYNARRAY_ALLOCELEMS( arrayd);
		DYNARRAY_ALLOCELEMS( arrayd) = pos + 1;

		void *_temp = realloc( DYNARRAY_ELEMS( arrayd),
				( DYNARRAY_ALLOCELEMS( arrayd) * sizeof( elem *))/*,
				oldsize * sizeof( elem *)*/);

		if ( !_temp){
			printf ( "addToArrayAtPos couldn't realloc memory!\n");
			exit(-1);
		}

		/*free( DYNARRAY_ELEMS( arrayd));*/
		DYNARRAY_ELEMS( arrayd) = ( elem**)_temp;

	}

	for(i=oldsize; i<DYNARRAY_ALLOCELEMS( arrayd); i++){
		DYNARRAY_ELEMS_POS( arrayd, i) = NULL;
	}

	DYNARRAY_TOTALELEMS( arrayd) = DYNARRAY_ALLOCELEMS( arrayd);
	DYNARRAY_ELEMS_POS( arrayd, pos) = item;

	return DYNARRAY_TOTALELEMS( arrayd);

}

void merge( elem **elems, int lower, int upper, int desc){

	elem **left, **right, **result;
	int mid = (lower + upper)/2;
	int ll, lr, i, total = 0;
	int cond;

	ll = mid-lower + 1;
	lr = upper - mid;
	left = elems + lower;
	right = elems + mid + 1;
	result = (elem **) malloc(( ll + lr) * sizeof( elem *));

	while( ll > 0 && lr > 0){

		if( ELEM_IDX( left[0]) <= ELEM_IDX( right[0])){

			if( desc) cond = 0; else cond = 1;

		} else{

			if(desc) cond=1; else cond=0;

		}

		if(cond){

			result[total++]=*left; left++; ll--;

		} else {

			result[total++]=*right; right++; lr--;

		}

	}

	if(ll>0){

		while(ll>0){
			result[total++]=*left; left++; ll--;
		}

	} else{

		while(lr>0){
			result[total++]=*right; right++; lr--;
		}

	}

	ll = mid - lower + 1;
	lr = upper - mid;
	left = elems + lower;
	right = elems + mid + 1;

	for( i = 0; i < ll; i++){
		left[i] = result[i];
	}

	for(i = 0; i < lr; i++){
		right[i] = result[i+ll];
	}

	free( result);

}

void sortArray( elem **elems, int lower, int upper, int desc){

	if( elems == NULL){
		printf ( "Typechecker trying to sort DYNARRAY with null elements");
		exit (-1);
	}

	if( upper - lower > 0){

		int mid = (upper + lower)/2;
		sortArray( elems, lower, mid, desc);
		sortArray( elems, mid + 1, upper, desc);
		merge( elems, lower, upper, desc);

	}

}


