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

#include "SketchPadTool.h"

#include <Vrui/Vrui.h>

/*****************************************
Methods of class SketchPad::SketchPadTool:
*****************************************/

SketchPad::SketchPadTool::SketchPadTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 active(false),firstLingerPos(true),lingering(false)
	{
	}

void SketchPad::SketchPadTool::buttonDown(const Point& pos)
	{
	/* Activate the tool: */
	active=true;
	firstPos=pos;
	
	/* Initialize linger detection: */
	lingerPos=pos;
	firstLingerPos=true;
	lingerEndTime=Vrui::getApplicationTime()+application->settings.getLingerTime();
	lingering=false;
	lastPos=pos;
	lastLingering=false;
	}

bool SketchPad::SketchPadTool::motion(const Point& pos)
	{
	/* Remember last frame's lingering state: */
	lastLingering=lingering;
	
	/* Check if the tool is lingering: */
	SketchSettings& settings=application->settings;
	if(Geometry::sqrDist(pos,lingerPos)>Math::sqr(settings.getLingerSize()))
		{
		/* Re-initialize linger detection: */
		lingerPos=pos;
		firstLingerPos=false;
		lingerEndTime=Vrui::getApplicationTime()+settings.getLingerTime();
		lingering=false;
		}
	else if(!lingering&&Vrui::getApplicationTime()>=lingerEndTime)
		{
		/* Start lingering: */
		lingering=true;
		
		/* Reset the linger detection neighborhood to the current tool position: */
		lingerPos=pos;
		}
	
	if(!lingering)
		{
		/* Schedule another Vrui frame for when the linger timeout expires: */
		Vrui::scheduleUpdate(lingerEndTime);
		}
	
	/* Check if the tool has moved: */
	bool result=pos!=lastPos;
	lastPos=pos;
	return result;
	}

void SketchPad::SketchPadTool::buttonUp(const Point& pos)
	{
	/* Check if the tool is lingering: */
	SketchSettings& settings=application->settings;
	if(Geometry::sqrDist(pos,lingerPos)>Math::sqr(settings.getLingerSize()))
		{
		/* Stop lingering: */
		lingering=false;
		}
	else
		{
		/* Start lingering if enough time has passed: */
		lingering=Vrui::getApplicationTime()>=lingerEndTime;
		}
	
	/* Deactivate the tool: */
	active=false;
	}
