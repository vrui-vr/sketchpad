/***********************************************************************
SketchObjectList - Class for lists of sketching objects in a simple
sketching application.
Copyright (c) 2016-2025 Oliver Kreylos

This file is part of the SketchPad vector drawing package.

The SketchPad vector drawing package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The SketchPad vector drawing package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the SketchPad vector drawing package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
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
