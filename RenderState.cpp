/***********************************************************************
RenderState - Class holding state while rendering sketch objects.
Copyright (c) 2025 Oliver Kreylos

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

#include "RenderState.h"

/****************************
Methods of class RenderState:
****************************/

RenderState::RenderState(GLContextData& sContextData)
	:contextData(sContextData),
	 activeRenderer(0),activeDataItem(0)
	{
	}

RenderState::~RenderState(void)
	{
	/* Deactivate the current renderer: */
	if(activeRenderer!=0)
		activeRenderer->deactivate(activeDataItem);
	}

bool RenderState::setRenderer(const Renderer* newRenderer)
	{
	bool result=activeRenderer!=newRenderer;
	if(result)
		{
		/* Deactivate the current renderer: */
		if(activeRenderer!=0)
			activeRenderer->deactivate(activeDataItem);
		activeDataItem=0;
		
		/* Set and activate the new renderer: */
		activeRenderer=newRenderer;
		if(activeRenderer!=0)
			activeDataItem=activeRenderer->activate(contextData);
		}
	
	return result;
	}
