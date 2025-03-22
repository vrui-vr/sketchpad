/***********************************************************************
SketchObjectList - Class for lists of sketching objects in a simple
sketching application.
Copyright (c) 2016 Oliver Kreylos
***********************************************************************/

#include "SketchObjectList.h"

/*********************************
Methods of class SketchObjectList:
*********************************/

SketchObjectList::~SketchObjectList(void)
	{
	/* Destroy all sketch objects: */
	while(head!=0)
		{
		SketchObject* succ=head->succ;
		delete head;
		head=succ;
		}
	}

size_t SketchObjectList::size(void) const
	{
	size_t result=0;
	for(const SketchObject* soPtr=head;soPtr!=0;soPtr=soPtr->succ)
		++result;
	return result;
	}

void SketchObjectList::clear(void)
	{
	/* Destroy all sketch objects: */
	while(head!=0)
		{
		SketchObject* succ=head->succ;
		delete head;
		head=succ;
		}
	
	/* Clear the list: */
	tail=0;
	}

void SketchObjectList::transfer(SketchObjectList& other)
	{
	/* Bail out if the list is empty: */
	if(head==0)
		return;
	
	/* Append the head of this list to the tail of the other list: */
	if(other.tail!=0)
		other.tail->succ=head;
	else
		other.head=head;
	head->pred=other.tail;
	other.tail=tail;
	
	/* Clear this list: */
	head=0;
	tail=0;
	}
