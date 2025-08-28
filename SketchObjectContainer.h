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

#ifndef SKETCHOBJECTCONTAINER_INCLUDED
#define SKETCHOBJECTCONTAINER_INCLUDED

#include "SketchGeometry.h"
#include "SketchObject.h"
#include "SketchObjectList.h"

/* Forward declarations: */
class GLContextData;

class SketchObjectContainer
	{
	/* Elements: */
	protected:
	SketchObjectList sketchObjects; // List of sketch objects in the container
	
	/* Protected methods: */
	void drawObjects(GLContextData& contextData) const; // Draws all sketch objects in the container in order
	void drawObjectsHighlight(Scalar cycle,GLContextData& contextData) const; // Highlights all sketch objects in the container in order using the given cycle in [-1, 1]
	
	/* Constructors and destructors: */
	public:
	virtual ~SketchObjectContainer(void); // Destroys all sketch objects in the container
	
	/* Methods: */
	const SketchObjectList& getSketchObjects(void) const // Returns the list of sketch objects in the container
		{
		return sketchObjects;
		}
	SketchObjectList& getSketchObjects(void) // Ditto
		{
		return sketchObjects;
		}
	virtual void append(SketchObject* newObject); // Appens the given object to the container's list
	virtual void insertAfter(SketchObject* pred,SketchObject* newObject); // Inserts the given object after the given predecessor
	virtual void remove(SketchObject* object); // Removes the given object from the container
	virtual SketchObject::PickResult pick(const Point& pos,Scalar radius); // Returns a pick result for this container
	};

#endif
