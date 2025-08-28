/***********************************************************************
Group - Class for groups of sketching objects.
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

#ifndef GROUP_INCLUDED
#define GROUP_INCLUDED

#include <vector>

#include "SketchObject.h"
#include "SketchObjectContainer.h"

class Group:public SketchObject,public SketchObjectContainer
	{
	friend class SketchObjectCreator;
	
	/* Elements: */
	private:
	static unsigned int typeCode; // The group class's type code
	
	/* Methods from class SketchObject: */
	public:
	virtual unsigned int getTypeCode(void) const;
	virtual bool pick(PickResult& result);
	virtual SketchObject* clone(void) const;
	virtual void applySettings(const SketchSettings& settings);
	virtual void transform(const Transformation& transform);
	virtual void snapToGrid(Scalar gridSize);
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container);
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const;
	virtual void read(IO::File& file,SketchObjectCreator& creator);
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual void glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const;
	
	/* Methods from class SketchObjectContainer: */
	virtual void append(SketchObject* newObject);
	virtual void insertAfter(SketchObject* pred,SketchObject* newObject);
	
	/* New methods: */
	void transferMembers(SketchObjectList& receiver); // Appends all group members to the given list and clears the group
	};

#endif
