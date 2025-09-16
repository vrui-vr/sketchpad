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
	Scalar lineWidth; // Spline's cosmetic line width
	std::vector<Point> points; // Vector of 1+n*3 spline control points
	unsigned int version; // Version number of control point list
	
	/* Private methods: */
	static void subdivide(const Point cps[4],RenderState& renderState); // Renders the spline as a polyline using recursive subdivision
	
	/* Constructors and destructors: */
	public:
	static void initClass(unsigned int newTypeCode); // Initializes the spline object class and assigns a unique type code
	Spline(const Color& sColor,Scalar sLineWidth,const Point cps[4]); // Creates a single-segment spline with the given color, line width, and initial control point array
	Spline(IO::File& file); // Creates a spline by reading from the given file
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
	virtual void glRenderAction(RenderState& renderState) const;
	virtual void glRenderActionHighlight(Scalar cycle,RenderState& renderState) const;
	};

class SplineFactory:public SketchObjectFactory
	{
	/* Embedded classes: */
	private:
	struct InputPoint
		{
		/* Elements: */
		public:
		Point pos; // Input point's position
		Scalar tolerance2; // Input point's squared tolerance
		Scalar dist; // Distance to the previous input point
		Scalar param; // Input point's current spline parameter
		
		/* Constructors and destructors: */
		InputPoint(const Point& sPos,Scalar sTolerance,Scalar sDist,Scalar sParam)
			:pos(sPos),tolerance2(Math::sqr(sTolerance)),dist(sDist),param(sParam)
			{
			}
		};
	
	typedef std::vector<InputPoint> InputCurve; // Type for input curves
	
	/* Elements: */
	Spline* current; // The currently created spline, or 0 if not creating a spline
	Box fixedBox; // The bounding box of the fixed part of the currently created spline
	bool lastLinger; // Flag if the tool was lingering during the previous motion callback
	InputCurve inputCurve; // List of input points associated with the active "unfixed" spline segment
	double inputCurveLength; // Length of the current input curve
	Point controlPoints[4]; // The control point array of the active "unfixed" spline segment
	bool g1; // Flag if the active spline segment should link to the previous one with G1 continuity
	Vector t0; // Normalized start tangent vector of the active spline segment if it should link with G1 continuity
	
	/* Private methods: */
	bool isGoodFit(const Point c[4]) const;
	void fitLinear(Point c[4],const Point& c0,const Point& c3) const;
	void fitQuadratic(Point c[4],const Point& c0,const Point& c3) const;
	void fitCubic(Point c[4],const Point& c0,const Point& c3) const;
	void fitQuadraticG1(Point c[4],const Point& c0,const Vector& t0,const Point& c3) const;
	void fitCubicG1(Point c[4],const Point& c0,const Vector& t0,const Point& c3) const;
	void fitQuadraticC1(Point c[4],const Point& c0,const Vector& t0,const Point& c3) const;
	void fitCubicC1(Point c[4],const Point& c0,const Vector& t0,const Point& c3) const;
	
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
