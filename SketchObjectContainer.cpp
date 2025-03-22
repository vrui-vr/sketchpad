/***********************************************************************
SketchObjectContainer - Base class for containers of sketch objects.
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

#include "SketchObjectContainer.h"

/**************************************
Methods of class SketchObjectContainer:
**************************************/

void SketchObjectContainer::drawObjects(GLContextData& contextData) const
	{
	/* Render all sketch objects: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		/* Render each sketch object individually, the inefficient way: */
		soIt->setGLState(contextData);
		soIt->glRenderAction(contextData);
		soIt->resetGLState(contextData);
		}
	}

void SketchObjectContainer::drawObjectsHighlight(Scalar cycle,GLContextData& contextData) const
	{
	/* Highlight all sketch objects: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		/* Render each sketch object individually, the inefficient way: */
		soIt->setGLState(contextData);
		soIt->glRenderActionHighlight(cycle,contextData);
		soIt->resetGLState(contextData);
		}
	}

SketchObjectContainer::~SketchObjectContainer(void)
	{
	}

void SketchObjectContainer::append(SketchObject* newObject)
	{
	/* Append the object to the list: */
	sketchObjects.push_back(newObject);
	}

void SketchObjectContainer::insertAfter(SketchObject* pred,SketchObject* newObject)
	{
	/* Insert the object into the list: */
	sketchObjects.insert(SketchObjectList::iterator(pred),newObject);
	}

void SketchObjectContainer::remove(SketchObject* object)
	{
	/* Remove the object from the list: */
	sketchObjects.erase(SketchObjectList::iterator(object));
	}

SketchObject* SketchObjectContainer::pickTop(const Point& pos,Scalar radius2)
	{
	/* Search backwards from the topmost to the bottommost object: */
	for(SketchObjectList::reverse_iterator soIt=sketchObjects.rbegin();soIt!=sketchObjects.rend();++soIt)
		if(soIt->pick(pos,radius2))
			return &*soIt;
	
	return 0;
	}

SketchObject::SnapResult SketchObjectContainer::snap(const Point& center,Scalar radius2) const
	{
	/* Check the given sphere against all sketch objects: */
	SketchObject::SnapResult result;
	result.valid=false;
	result.dist2=radius2;
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		SketchObject::SnapResult sr=soIt->snap(center,result.dist2);
		if(sr.valid)
			result=sr;
		}
	
	return result;
	}
