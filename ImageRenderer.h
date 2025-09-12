/***********************************************************************
ImageRenderer - Class to render images using 2D textures.
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

#ifndef IMAGERENDERER_INCLUDED
#define IMAGERENDERER_INCLUDED

#include <Threads/Atomic.h>
#include <Images/TextureSet.h>

#include "Renderer.h"

class ImageRenderer:public Renderer
	{
	/* Elements: */
	private:
	static ImageRenderer* theRenderer; // Singleton image rendering object
	static Threads::Atomic<unsigned int> refCount; // Number of references to the singleton image rendering object
	Images::TextureSet textureSet; // Texture set managing all current images
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* Methods from class Renderer: */
	public:
	virtual GLObject::DataItem* activate(RenderState& renderState) const;
	virtual void deactivate(GLObject::DataItem* dataItem,RenderState& renderState) const;
	
	/* New methods: */
	static ImageRenderer* acquire(void); // Acquires a reference to the singleton rendering object
	static void release(void); // Releases a reference to the singleton rendering object
	Images::TextureSet& getTextureSet(void) // Returns the texture set
		{
		return textureSet;
		}
	};

#endif
