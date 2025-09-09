/***********************************************************************
SelectTool - Class for selecting tools.
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

#ifndef SELECTTOOL_INCLUDED
#define SELECTTOOL_INCLUDED

#include <Vrui/GenericToolFactory.h>

#include "SketchPadTool.h"

class SketchPad::SelectTool:public SketchPad::SketchPadTool
	{
	/* Embedded classes: */
	private:
	typedef Vrui::GenericToolFactory<SelectTool> Factory;
	friend Factory;
	
	/* Elements: */
	private:
	static Factory* factory; // Pointer to the class' factory object
	SketchObject* draggedObject; // Pointer to an object being dragged, or 0
	Vector pickOffset; // Offset from initial tool position to picked point on dragged object
	Vector dragTrans; // The current dragging translation
	Box box; // The current selection box
	
	/* Constructors and destructors: */
	public:
	static void initClass(Vrui::ToolFactory* baseClass); // Initializes the tool class
	SelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	virtual ~SelectTool(void);
	
	/* Methods from class Vrui::Tool: */
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	
	/* Methods from class SketchPadTool: */
	virtual void glRenderAction(RenderState& renderState) const;
	};

#endif
