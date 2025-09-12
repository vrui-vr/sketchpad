/***********************************************************************
Spline - Class for free-hand curves represented Catmull-Rom splines.
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

#ifndef SPLINE_INCLUDED
#define SPLINE_INCLUDED

#include <vector>

#include "SketchGeometry.h"
#include "SketchObject.h"

/* Forward declarations: */
class PolylineRenderer;

class Spline:public SketchObject
	{
	friend class SketchObjectCreator;
	friend class SplineFactory;
	
	/* Elements: */
	private:
	static unsigned int typeCode; // The spline class's type code
	static PolylineRenderer* renderer; // A renderer to render spline
	
	Color color; // Spline's color
	float lineWidth; // Spline's cosmetic line width
	std::vector<Point> points; // Vector of 1+n*3 spline control points
	unsigned int version; // Version number of control point list
	
	/* Constructors and destructors: */
	public:
	static void initClass(unsigned int newTypeCode); // Initializes the spline object class and assigns a unique type code
	Spline(void); // Creates an empty spline with undefined parameters
	virtual ~Spline(void);
	static void deinitClass(void); // De-initializes the spline object class
	
	/* Methods from SketchObject: */
	virtual unsigned int getTypeCode(void) const;
	virtual bool pick(PickResult& result);
	virtual SketchObject* clone(void) const;
	virtual void applySettings(const SketchSettings& settings);
	virtual void transform(const Transformation& transform);
	virtual void snapToGrid(Scalar gridSize);
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container);
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const;
	virtual void read(IO::File& file,SketchObjectCreator& creator);
	virtual void glRenderAction(RenderState& renderState) const;
	virtual void glRenderActionHighlight(Scalar cycle,RenderState& renderState) const;
	};

class SplineFactory:public SketchObjectFactory
	{
	/* Elements: */
	private:
	Spline* current; // The currently created spline, or 0 if not creating a spline
	bool lineMode; // Flag whether the factory has changed mode to line drawing
	bool lastLinger; // Flag if the tool was lingering during the previous motion callback
	
	/* Constructors and destructors: */
	public:
	SplineFactory(SketchSettings& sSettings);
	virtual ~SplineFactory(void);
	
	/* Methods from SketchObjectFactory: */
	virtual void buttonDown(const Point& pos);
	virtual void motion(const Point& pos,bool lingering,bool firstNeighborhood);
	virtual bool buttonUp(const Point& pos);
	virtual SketchObject* finish(void);
	virtual void glRenderAction(RenderState& renderState) const;
	};

#endif
