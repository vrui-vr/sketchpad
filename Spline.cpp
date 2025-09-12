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

#include "Spline.h"

#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <IO/File.h>
#include <Math/Math.h>

#include "SketchSettings.h"
#include "RenderState.h"
#include "PolylineRenderer.h"

/*******************************
Static elements of class Spline:
*******************************/

unsigned int Spline::typeCode=0;
PolylineRenderer* Spline::renderer=0;

/***********************
Methods of class Spline:
***********************/

void Spline::subdivide(const Point cps[4],RenderState& renderState)
	{
	/* Check if the spline segment is not sufficiently flat: */
	Scalar tolerance=renderState.getPixelSize();
	Vector d=cps[3]-cps[0];
	Scalar d2=Geometry::sqr(d);
	bool split=false;
	if(d2>Scalar(0))
		{
		/* Check the midpoints against a cylinder around the endpoints: */
		Scalar tolerance2D2=Math::sqr(tolerance)*d2;
		split=Geometry::sqr((d^(cps[1]-cps[0])))>tolerance2D2||Geometry::sqr((d^(cps[2]-cps[3])))>tolerance2D2;
		if(!split)
			{
			Scalar dLen=Math::sqrt(d2);
			split=(cps[1]-cps[0])*d<tolerance*dLen||(cps[2]-cps[3])*d>tolerance*dLen;
			}
		}
	else
		{
		/* The endpoints are identical; split if the midpoints point outwards: */
		Scalar tolerance2=Math::sqr(tolerance);
		split=Geometry::sqrDist(cps[1],cps[0])>tolerance2||Geometry::sqrDist(cps[2],cps[3])>tolerance2;
		}
	if(split)
		{
		/* Subdivide the spline segment at the midpoint: */
		Point scps[7];
		scps[0]=cps[0];
		scps[2]=cps[1];
		scps[4]=cps[2];
		scps[6]=cps[3];
		scps[1]=Geometry::mid(scps[0],scps[2]);
		scps[3]=Geometry::mid(scps[2],scps[4]);
		scps[5]=Geometry::mid(scps[4],scps[6]);
		scps[2]=Geometry::mid(scps[1],scps[3]);
		scps[4]=Geometry::mid(scps[3],scps[5]);
		scps[3]=Geometry::mid(scps[2],scps[4]);
		
		/* Recursively render the two sub-segments: */
		subdivide(scps+0,renderState);
		subdivide(scps+3,renderState);
		}
	else
		{
		/* Emit the spline segment as a straight line segment: */
		renderer->addVertex(cps[3],renderState.getDataItem());
		}
	}

void Spline::initClass(unsigned int newTypeCode)
	{
	/* Store the type code: */
	typeCode=newTypeCode;
	
	/* Acquire a reference to the polyline renderer: */
	renderer=PolylineRenderer::acquire();
	}

Spline::Spline(const Color& sColor,Scalar sLineWidth,const Point cps[4])
	:color(sColor),lineWidth(sLineWidth),
	 version(0)
	{
	/* Copy the initial control point array and initialize the bounding box: */
	for(int i=0;i<4;++i)
		{
		points.push_back(cps[i]);
		boundingBox.addPoint(cps[i]);
		}
	
	++version;
	}

Spline::Spline(IO::File& file)
	:version(0)
	{
	/* Read the color and line width: */
	for(int i=0;i<4;++i)
		color[i]=file.read<Misc::UInt8>();
	lineWidth=file.read<Misc::Float32>();
	
	/* Read the number of control points: */
	size_t numPoints=file.read<Misc::UInt16>();
	if(numPoints<4U||numPoints%3U!=1U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid number of spline control points");
	
	/* Read the control point vector and calculate the bounding box: */
	points.reserve(numPoints);
	for(size_t i=0;i<numPoints;++i)
		{
		Point p;
		for(int j=0;j<3;++j)
			p[j]=file.read<Misc::Float32>();
		points.push_back(p);
		boundingBox.addPoint(p);
		}
	
	++version;
	}

Spline::~Spline(void)
	{
	/* Remove this spline from the renderer's cache: */
	renderer->drop(this);
	}

void Spline::deinitClass(void)
	{
	/* Release the reference to the polyline renderer: */
	PolylineRenderer::release();
	renderer=0;
	}

unsigned int Spline::getTypeCode(void) const
	{
	return typeCode;
	}

bool Spline::pick(PickResult& result)
	{
	return false;
	}

SketchObject* Spline::clone(void) const
	{
	/* Return a new spline object: */
	return new Spline(*this);
	}

void Spline::applySettings(const SketchSettings& settings)
	{
	/* Copy the settings object's color and line width: */
	color=settings.getColor();
	lineWidth=settings.getLineWidth();
	}

void Spline::transform(const Transformation& transform)
	{
	/* Transform all control points and re-calculate the bounding box: */
	boundingBox=Box::empty;
	for(std::vector<Point>::iterator pIt=points.begin();pIt!=points.end();++pIt)
		{
		*pIt=transform.transform(*pIt);
		boundingBox.addPoint(*pIt);
		}
	
	++version;
	}

void Spline::snapToGrid(Scalar gridSize)
	{
	}

void Spline::rubout(const Capsule& eraser,SketchObjectContainer& container)
	{
	}

void Spline::write(IO::File& file,const SketchObjectCreator& creator) const
	{
	/* Write the color and line width: */
	for(int i=0;i<4;++i)
		file.write<Misc::UInt8>(color[i]);
	file.write<Misc::Float32>(lineWidth);
	
	/* Write the number of control points: */
	file.write<Misc::UInt16>(points.size());
	
	/* Write all control points: */
	for(std::vector<Point>::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
		for(int i=0;i<3;++i)
			file.write<Misc::Float32>((*pIt)[i]);
	}

void Spline::glRenderAction(RenderState& renderState) const
	{
	/* Draw the spline using a polyline renderer: */
	renderState.setRenderer(renderer);
	if(renderer->draw(this,version,color,lineWidth,renderState.getDataItem()))
		{
		/* Regenerate the spline's vertices: */
		std::vector<Point>::const_iterator pIt=points.begin();
		renderer->addVertex(*pIt,renderState.getDataItem());
		std::vector<Point>::const_iterator pEnd=points.end()-1;
		for(;pIt!=pEnd;pIt+=3)
			{
			/* Subdivide this spline segment: */
			Point cps[4];
			for(int i=0;i<4;++i)
				cps[i]=pIt[i];
			subdivide(cps,renderState);
			}
		
		/* Finish and draw the spline: */
		renderer->finish(renderState.getDataItem());
		}
	}

void Spline::glRenderActionHighlight(Scalar cycle,RenderState& renderState) const
	{
	}
