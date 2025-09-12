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

#include "Curve.h"

#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <IO/File.h>
#include <Math/Math.h>

#include "Config.h"
#include "Capsule.h"
#include "SketchObjectContainer.h"
#include "SketchSettings.h"
#include "RenderState.h"
#include "PolylineRenderer.h"

/******************************
Static elements of class Curve:
******************************/

unsigned int Curve::typeCode=0;
PolylineRenderer* Curve::renderer=0;

/**********************
Methods of class Curve:
**********************/

void Curve::initClass(unsigned int newTypeCode)
	{
	/* Store the type code: */
	typeCode=newTypeCode;
	
	/* Acquire a reference to the polyline renderer: */
	renderer=PolylineRenderer::acquire();
	}

Curve::Curve(const Color& sColor,Scalar sLineWidth,const Point& firstVertex)
	:color(sColor),lineWidth(sLineWidth),
	 version(0)
	{
	/* Append the first curve vertex: */
	points.push_back(firstVertex);
	boundingBox.addPoint(firstVertex);
	
	++version;
	}

Curve::Curve(const Color& sColor,Scalar sLineWidth,std::vector<Point>& sPoints,const Box& sBoundingBox)
	:color(sColor),lineWidth(sLineWidth),
	 version(0)
	{
	/* Take the given vertex list and bounding box: */
	std::swap(points,sPoints);
	boundingBox=sBoundingBox;
	
	++version;
	}

Curve::Curve(IO::File& file)
	:version(0)
	{
	/* Read the color and line width: */
	for(int i=0;i<4;++i)
		color[i]=file.read<Misc::UInt8>();
	lineWidth=file.read<Misc::Float32>();
	
	/* Read the number of points: */
	size_t numPoints=file.read<Misc::UInt16>();
	if(numPoints==0U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid number of curve points");
	
	/* Read the point list and calculate the bounding box: */
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

Curve::~Curve(void)
	{
	/* Remove this curve from the renderer's cache: */
	renderer->drop(this);
	}

void Curve::deinitClass(void)
	{
	/* Release the reference to the polyline renderer: */
	PolylineRenderer::release();
	renderer=0;
	}

unsigned int Curve::getTypeCode(void) const
	{
	return typeCode;
	}

bool Curve::pick(SketchObject::PickResult& result)
	{
	bool picked=false;
	
	/* Check the beginning vertex against the given sphere: */
	std::vector<Point>::const_iterator p0It=points.begin();
	picked=result.update(this,0,*p0It)||picked;
	
	/* Check every curve segment against the given sphere: */
	std::vector<Point>::const_iterator lastIt=points.end()-1;
	for(std::vector<Point>::const_iterator p1It=p0It+1;p1It!=points.end();p0It=p1It,++p1It)
		{
		/* Check the segment's end vertex against the given sphere: */
		picked=result.update(this,p1It==lastIt?0:1,*p1It)||picked;
		
		/* Check the line segment against the given sphere: */
		picked=result.update(this,*p0It,*p1It)||picked;
		}
	
	return picked;
	}

SketchObject* Curve::clone(void) const
	{
	/* Return a new curve object: */
	return new Curve(*this);
	}

void Curve::applySettings(const SketchSettings& settings)
	{
	/* Copy the settings object's color and line width: */
	color=settings.getColor();
	lineWidth=settings.getLineWidth();
	}

void Curve::transform(const Transformation& transform)
	{
	/* Transform all curve points and re-calculate the bounding box: */
	boundingBox=Box::empty;
	for(std::vector<Point>::iterator pIt=points.begin();pIt!=points.end();++pIt)
		{
		*pIt=transform.transform(*pIt);
		boundingBox.addPoint(*pIt);
		}
	
	++version;
	}

namespace {

/****************
Helper functions:
****************/

inline Point snapPointToGrid(const Point& p,Scalar gridSize)
	{
	Point result;
	for(int i=0;i<Point::dimension;++i)
		result[i]=Math::floor(p[i]/gridSize+Scalar(0.5))*gridSize;
	return result;
	}

}

void Curve::snapToGrid(Scalar gridSize)
	{
	/* Snap all curve points to the grid and re-calculate the bounding box: */
	boundingBox=Box::empty;
	std::vector<Point> newPoints;
	std::vector<Point>::iterator pIt=points.begin();
	newPoints.push_back(snapPointToGrid(*pIt,gridSize));
	boundingBox.addPoint(newPoints.back());
	for(++pIt;pIt!=points.end();++pIt)
		{
		Point p=snapPointToGrid(*pIt,gridSize);
		if(p!=newPoints.back())
			{
			newPoints.push_back(p);
			boundingBox.addPoint(newPoints.back());
			}
		}
	std::swap(points,newPoints);
	
	++version;
	}

void Curve::rubout(const Capsule& eraser,SketchObjectContainer& container)
	{
	/* Create a temporary list of curve points outside the capsule: */
	std::vector<Point> outside;
	Box outsideBox=Box::empty;
	
	/* Check if the curve's beginning vertex lies within the capsule: */
	std::vector<Point>::iterator p0It=points.begin();
	bool inside=eraser.isInside(*p0It);
	bool anyChanges=inside;
	
	/* Store the beginning vertex in the outside list if it is outside: */
	if(!inside)
		{
		outside.push_back(*p0It);
		outsideBox.addPoint(*p0It);
		}
	
	/* Process the rest of the curve: */
	for(std::vector<Point>::iterator p1It=p0It+1;p1It!=points.end();p0It=p1It,++p1It)
		{
		/* Check if the current line segment enters or exits the capsule: */
		if(inside) // Segment start point is inside the capsule
			{
			/* The segment does not change state if the end point is also inside the capsule (convexity): */
			if(!eraser.isInside(*p1It))
				{
				/* The end point is outside; find the exact exit point: */
				Capsule::Interval ii=eraser.intersectLine(*p0It,*p1It);
				Point exit=Geometry::affineCombination(*p0It,*p1It,ii.getMax());
				
				/* Start a new temporary curve with the exit point and the segment end point: */
				outside.push_back(exit);
				outsideBox.addPoint(exit);
				outside.push_back(*p1It);
				outsideBox.addPoint(*p1It);
				inside=false;
				}
			}
		else // Segment start point is outside the capsule
			{
			/* Intersect the curve segment with the capsule: */
			Capsule::Interval ii=eraser.intersectLine(*p0It,*p1It);
			
			/* Check if the segment enters the capsule: */
			if(ii.getMin()>Scalar(0)&&ii.getMin()<=Scalar(1))
				{
				/* End the temporary curve with the exact entry point: */
				Point entry=Geometry::affineCombination(*p0It,*p1It,ii.getMin());
				outside.push_back(entry);
				outsideBox.addPoint(entry);
				
				/* Create a new curve object with the outside curve and insert it into the sketch object list after this one: */
				Curve* newCurve=new Curve(color,lineWidth,outside,outsideBox);
				container.insertAfter(this,newCurve);
				
				/* Reset the temporary outside curve: */
				// outside.clear(); // Redundant, is already clear
				outsideBox=Box::empty;
				
				/* Check if the current segment stays within the capsule: */
				if(ii.getMax()>=Scalar(1))
					{
					/* Next segment start point is inside the capsule: */
					inside=true;
					}
				else
					{
					/* Start a new temporary curve with the exit point and the segment end point: */
					Point exit=Geometry::affineCombination(*p0It,*p1It,ii.getMax());
					outside.push_back(exit);
					outsideBox.addPoint(exit);
					outside.push_back(*p1It);
					outsideBox.addPoint(*p1It);
					}
				
				anyChanges=true;
				}
			else // Entire segment is outside the capsule
				{
				/* Add segment end point to temporary curve: */
				outside.push_back(*p1It);
				outsideBox.addPoint(*p1It);
				}
			}
		}
	
	/* Check if there is a leftover outside curve segment: */
	if(!inside)
		{
		/* Store the leftover curve in this curve object: */
		boundingBox=outsideBox;
		std::swap(points,outside);
		
		/* Invalidate the cache if any changes were made: */
		if(anyChanges)
			++version;
		}
	else
		{
		/* Delete this curve object: */
		container.remove(this);
		}
	}

void Curve::write(IO::File& file,const SketchObjectCreator& creator) const
	{
	/* Write the color and line width: */
	for(int i=0;i<4;++i)
		file.write<Misc::UInt8>(color[i]);
	file.write<Misc::Float32>(lineWidth);
	
	/* Write the number of points: */
	file.write<Misc::UInt16>(points.size());
	
	/* Write all points: */
	for(std::vector<Point>::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
		for(int i=0;i<3;++i)
			file.write<Misc::Float32>((*pIt)[i]);
	}

void Curve::glRenderAction(RenderState& renderState) const
	{
	/* Draw the curve using a polyline renderer: */
	renderState.setRenderer(renderer);
	renderer->draw(this,version,points,color,lineWidth,renderState.getDataItem());
	}

void Curve::glRenderActionHighlight(Scalar cycle,RenderState& renderState) const
	{
	/* Calculate the current highlight color: */
	Color highlight=cycle>=Scalar(0)?Color(255U,255U,255U):Color(0U,0U,0U);
	cycle=Math::abs(cycle);
	for(int i=0;i<4;++i)
		highlight[i]=GLubyte(Math::floor(Scalar(color[i])*(Scalar(1)-cycle)+Scalar(highlight[i])*cycle+Scalar(0.5)));
	
	/* Draw the curve using a polyline renderer: */
	renderState.setRenderer(renderer);
	renderer->draw(this,version,points,highlight,lineWidth,renderState.getDataItem());
	}

/*****************************
Methods of class CurveFactory:
*****************************/

CurveFactory::CurveFactory(SketchSettings& sSettings)
	:SketchObjectFactory(sSettings),
	 current(0),
	 lineMode(false)
	{
	}

CurveFactory::~CurveFactory(void)
	{
	/* Delete the current curve: */
	delete current;
	}

void CurveFactory::buttonDown(const Point& pos)
	{
	/* Delete a pending current curve and start a new one: */
	delete current;
	current=new Curve(settings.getColor(),settings.getLineWidth(),pos);
	
	/* Revert to curve mode: */
	lineMode=false;
	
	/* Reset the curve suffix: */
	curveLast=pos;
	points.clear();
	
	/* Initialize linger detection: */
	lastLinger=false;
	}

void CurveFactory::motion(const Point& pos,bool lingering,bool firstNeighborhood)
	{
	/* Check if the tool just started lingering: */
	bool startLingering=lingering&&!lastLinger;
	
	/* Check if the factory is already in line mode: */
	if(lineMode)
		{
		Point end=pos;
		
		/* Snap the line's end point if the tool is lingering: */
		if(startLingering)
			end=settings.snap(pos);
			
		current->points.back()=end;
		++current->version;
		}
	else if(startLingering)
		{
		/* Check if the current curve should be turned into a line: */
		Vector dir=pos-current->points.front();
		Scalar maxBackspace=settings.getDetailSize()*dir.mag();
		std::vector<Point>::iterator pIt=current->points.begin();
		Scalar offset=*pIt*dir;
		bool straight=true;
		for(++pIt;straight&&pIt!=current->points.end();++pIt)
			{
			Scalar nextOffset=*pIt*dir;
			straight=nextOffset>=offset-maxBackspace;
			offset=nextOffset;
			}
		if(firstNeighborhood||straight)
			{
			/* Create the line: */
			Curve* line=new Curve(current->color,current->lineWidth,current->points.front());
			
			/* Snap the first curve point to nearby sketch objects if the pen hasn't moved yet: */
			if(firstNeighborhood)
				line->points.front()=settings.snap(line->points.front());
			
			/* Add the current position as the line's end point: */
			line->points.push_back(pos);
			++line->version;
			
			/* Go to line mode: */
			delete current;
			current=line;
			lineMode=true;
			}
		}
	else
		{
		/* Update the curve suffix: */
		if(points.empty())
			current->points.push_back(pos);
		else
			current->points.back()=pos;
		++current->version;
		points.push_back(pos);
		
		/* Check if the current curve suffix is well-represented by a line segment: */
		Vector dir=pos-curveLast;
		Vector normal=Geometry::normal(dir);
		normal.normalize();
		Scalar dist0=curveLast*normal;
		bool straight=pos!=curveLast;
		for(std::vector<Point>::iterator pIt=points.begin();straight&&pIt!=points.end();++pIt)
			{
			Scalar dist=Math::abs(*pIt*normal-dist0);
			straight=dist<settings.getDetailSize();
			}
		if(straight&&points.size()>=2)
			{
			/* Check if the curve suffix made a turn: */
			straight=points.back()*dir>=*(points.end()-2)*dir;
			}
		
		/* Finish the curve suffix if it is not straight: */
		if(!straight)
			{
			/* Add the previous end of the curve suffix to the curve: */
			curveLast=*(points.end()-2);
			current->points.back()=curveLast;
			current->boundingBox.addPoint(curveLast);
			current->points.push_back(pos);
			++current->version;
			points.clear();
			points.push_back(pos);
			}
		}
	
	/* Remember if the tool is lingering: */
	lastLinger=lingering;
	}

bool CurveFactory::buttonUp(const Point& pos)
	{
	/* Fix the tentative last curve point: */
	current->boundingBox.addPoint(current->points.back());
	
	/* Tell the caller that the curve is done: */
	return true;
	}

SketchObject* CurveFactory::finish(void)
	{
	if(current!=0)
		{
		/* Fix the tentative last curve point: */
		current->boundingBox.addPoint(current->points.back());
		}
	
	/* Return the current curve object: */
	SketchObject* result=current;
	current=0;
	return result;
	}

void CurveFactory::glRenderAction(RenderState& renderState) const
	{
	/* Draw the current curve if it exists: */
	if(current!=0)
		#if 0
		current->glRenderAction(renderState);
		#else
		{
		renderState.setRenderer(Curve::renderer);
		Curve::renderer->draw(current->points,current->color,current->lineWidth,renderState.getDataItem());
		}
		#endif
	}
