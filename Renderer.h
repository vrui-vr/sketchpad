/***********************************************************************
Renderer - Abstract base class for sketch object renderers.
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

#ifndef RENDERER_INCLUDED
#define RENDERER_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>

/* Forward declarations: */
class RenderState;

class Renderer:public GLObject
	{
	/* New methods: */
	public:
	virtual DataItem* activate(RenderState& renderState) const =0; // Activates the renderer in the OpenGL context associated with the given render state and returns a context state object to be used for subsequent rendering calls
	virtual void deactivate(GLObject::DataItem* dataItem,RenderState& renderState) const =0; // Deactivates the renderer in the OpenGL context in which it was previously activated; callee will dispose of context state object
	};

#endif
