/***********************************************************************
PolylineRenderer - Class to render polylines with geometric line width
and high-quality anti-aliasing.
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

#ifndef POLYLINERENDERER_INCLUDED
#define POLYLINERENDERER_INCLUDED

#include <vector>

#include "SketchGeometry.h"
#include "Renderer.h"

class PolylineRenderer:public Renderer
	{
	/* Embedded classes: */
	public:
	typedef std::vector<Point> Polyline; // Type for polylines defined by lists of points
	
	private:
	struct DataItem; // Forward declaration of per-context data structure
	
	/* Elements: */
	static PolylineRenderer theRenderer; // Singleton polyline rendering object
	Scalar scaleFactor; // Scale factor from line widths to model space units
	
	/* Constructors and destructors: */
	public:
	PolylineRenderer(void);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* Methods from class Renderer: */
	GLObject::DataItem* activate(GLContextData& contextData) const;
	void deactivate(GLObject::DataItem* dataItem) const;
	
	/* New methods: */
	static PolylineRenderer* getTheRenderer(void) // Returns the singleton rendering object
		{
		return &theRenderer;
		}
	void setScaleFactor(Scalar newScaleFactor); // Updates the scale factor from line widths to model space units
	void draw(const Polyline& polyline,const Color& color,Scalar lineWidth,GLObject::DataItem* dataItem) const; // Renders the given polyline with the given color and line width
	void cache(const void* cacheId,const Polyline& polyline,GLObject::DataItem* dataItem) const; // Caches the given polyline for future faster drawing
	void draw(const void* cacheId,const Color& color,Scalar lineWidth,GLObject::DataItem* dataItem) const; // Renders a previously cached polyline with the given color and line width
	};

#endif
