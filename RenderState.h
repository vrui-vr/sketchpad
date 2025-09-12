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

#ifndef RENDERSTATE_INCLUDED
#define RENDERSTATE_INCLUDED

#include "SketchGeometry.h"
#include "Renderer.h"

/* Forward declarations: */
class GLContextData;

class RenderState
	{
	/* Elements: */
	public:
	GLContextData& contextData; // OpenGL context in which this render state operates
	private:
	Scalar pixelSize; // Size of a pixel in the current window in model coordinate units
	const Renderer* activeRenderer; // The currently active sketch object renderer
	GLObject::DataItem* activeDataItem; // The per-context state of the currently active renderer
	
	/* Constructors and destructors: */
	public:
	RenderState(GLContextData& sContextData);
	~RenderState(void);
	
	/* Methods: */
	Scalar getPixelSize(void) const // Returns the current pixel size in model coordinate units
		{
		return pixelSize;
		}
	bool setRenderer(const Renderer* newRenderer); // Sets the given renderer; returns true if the renderer changed
	const Renderer* getRenderer(void) // Returns the active renderer
		{
		return activeRenderer;
		}
	GLObject::DataItem* getDataItem(void) // Returns the active renderer's context state
		{
		return activeDataItem;
		}
	};

#endif
