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

void SketchObjectContainer::drawObjects(RenderState& renderState) const
	{
	/* Render all sketch objects: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		soIt->glRenderAction(renderState);
	}

void SketchObjectContainer::drawObjectsHighlight(Scalar cycle,RenderState& renderState) const
	{
	/* Highlight all sketch objects: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		soIt->glRenderActionHighlight(cycle,renderState);
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

SketchObject::PickResult SketchObjectContainer::pick(const Point& pos,Scalar radius)
	{
	/* Create a pick result: */
	SketchObject::PickResult result(pos,radius);
	
	/* Pick objects from topmost to bottommost: */
	for(SketchObjectList::reverse_iterator soIt=sketchObjects.rbegin();soIt!=sketchObjects.rend();++soIt)
		soIt->pick(result);
	
	return result;
	}
