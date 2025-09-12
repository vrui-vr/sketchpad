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

#include "ImageRenderer.h"

#include <GL/gl.h>

/**************************************
Static elements of class ImageRenderer:
**************************************/

ImageRenderer* ImageRenderer::theRenderer=0;
Threads::Atomic<unsigned int> ImageRenderer::refCount(0);

/******************************
Methods of class ImageRenderer:
******************************/

void ImageRenderer::initContext(GLContextData& contextData) const
	{
	}

GLObject::DataItem* ImageRenderer::activate(RenderState&) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	
	return 0;
	}

void ImageRenderer::deactivate(GLObject::DataItem*,RenderState&) const
	{
	/* Reset OpenGL state: */
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
	glPopAttrib();
	}

ImageRenderer* ImageRenderer::acquire(void)
	{
	/* Increase the reference count and create a new singleton object if the reference count was zero: */
	if(refCount.postAdd(1)==0)
		theRenderer=new ImageRenderer;
	
	return theRenderer;
	}

void ImageRenderer::release(void)
	{
	/* Decrease the reference count and delete the singleton object if the reference count reaches zero: */
	if(refCount.preSub(1)==0)
		{
		delete theRenderer;
		theRenderer=0;
		}
	}
