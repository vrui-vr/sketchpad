/***********************************************************************
Curve - Class for free-hand curves represented as polylines.
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

#ifndef CURVE_INCLUDED
#define CURVE_INCLUDED

#include <vector>

#include "SketchGeometry.h"
#include "SketchObject.h"

class Curve:public SketchObject
	{
	friend class SketchObjectCreator;
	friend class CurveFactory;
	
	/* Embedded classes: */
	private:
	class Renderer; // Helper class to manage the curve rendering shaders
	
	/* Elements: */
	static unsigned int typeCode; // The curve class's type code
	static Renderer renderer; // Static object containing the shader code for curve rendering
	
	Color color; // Curve's color
	float lineWidth; // Curve's cosmetic line width
	std::vector<Point> points; // Vector of curve points
	
	/* Private methods: */
	void draw(const Color& drawColor,GLContextData& contextData) const; // Draws the curve using the given color
	
	/* Methods from SketchObject: */
	public:
	virtual unsigned int getTypeCode(void) const;
	virtual bool pick(PickResult& result);
	virtual SketchObject* clone(void) const;
	virtual void applySettings(const SketchSettings& settings);
	virtual void transform(const Transformation& transform);
	virtual void snapToGrid(Scalar gridSize);
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container);
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const;
	virtual void read(IO::File& file,SketchObjectCreator& creator);
	virtual void setGLState(GLContextData& contextData) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual void glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const;
	virtual void resetGLState(GLContextData& contextData) const;
	};

class CurveFactory:public SketchObjectFactory
	{
	/* Elements: */
	private:
	Curve* current; // The currently created curve, or 0 if not creating a curve
	bool lineMode; // Flag whether the factory has changed mode to line drawing
	bool lastLinger; // Flag if the tool was lingering during the previous motion callback
	Point curveLast; // The last fixed curve point
	std::vector<Point> points; // Tentative points at the end of the current curve
	
	/* Constructors and destructors: */
	public:
	CurveFactory(SketchSettings& sSettings);
	virtual ~CurveFactory(void);
	
	/* Methods from SketchObjectFactory: */
	virtual void buttonDown(const Point& pos);
	virtual void motion(const Point& pos,bool lingering,bool firstNeighborhood);
	virtual bool buttonUp(const Point& pos);
	virtual SketchObject* finish(void);
	virtual void glRenderAction(GLContextData& contextData) const;
	};

#endif
