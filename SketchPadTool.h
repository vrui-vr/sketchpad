/***********************************************************************
SketchPadTool - Base class for tools interacting with the SketchPad
application.
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

#ifndef SKETCHPADTOOL_INCLUDED
#define SKETCHPADTOOL_INCLUDED

#include <Vrui/Tool.h>
#include <Vrui/Application.h>

#include "SketchGeometry.h"
#include "SketchPad.h"

/* Forward declarations: */
class RenderState;

class SketchPad::SketchPadTool:public Vrui::Tool,public Vrui::Application::Tool<SketchPad>
	{
	/* Elements: */
	private:
	bool active; // Flag if the tool's button is currently pressed
	Point firstPos; // Position where the tool became active
	Point lingerPos; // Neighborhood position for linger detection
	bool firstLingerPos; // Flag if the linger position is the initial one when a button down event occurred
	double lingerEndTime; // Application time at which the tool will enter lingering state if it does not leave the current neighborhood
	bool lingering; // Flag whether the tool is currently lingering
	Point lastPos; // Last tool position
	bool lastLingering; // Flag whether the tool was lingering on the previous frame
	
	/* Constructors and destructors: */
	public:
	SketchPadTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* New methods: */
	bool isActive(void) const // Returns true if the tool's button is currently pressed
		{
		return active;
		}
	const Point& getFirstPos(void) const // Returns the position where the tool became active
		{
		return firstPos;
		}
	bool hasMoved(void) const // Returns true if the tool has moved since the button down event
		{
		return !firstLingerPos;
		}
	bool isLingering(void) const // Returns true if the tool is currently lingering
		{
		return lingering;
		}
	bool wasLingering(void) const // Returns true of the tool was lingering on the previous frame
		{
		return lastLingering;
		}
	void buttonDown(const Point& pos); // Processes a button-down event at the given position and the current time
	bool motion(const Point& pos); // Processes a motion event to the given position while the button is pressed at the current time; returns true if the tool actually moved
	void buttonUp(const Point& pos); // Processes a button-up event at the given position and the current time
	virtual void glRenderAction(RenderState& renderState) const =0; // Renders sketch object-related tool state at the end of the application's display method
	};

#endif
