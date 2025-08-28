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
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBGeometryShader4.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLEXTGeometryShader4.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/DisplayState.h>
#include <Vrui/VRScreen.h>
#include <Vrui/VRWindow.h>

#include "Config.h"
#include "Capsule.h"
#include "SketchObjectContainer.h"
#include "SketchSettings.h"

/************************************
Declaration of class Curve::Renderer:
************************************/

class Curve::Renderer:public GLObject
	{
	friend class Curve;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLhandleARB lineShader; // GLSL shader to render anti-aliased lines
		GLint uniforms[2]; // Locations of the line rendering shader's uniform variables
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Methods from class GLObject: */
	public:
	virtual void initContext(GLContextData& contextData) const;
	};

/******************************************
Methods of class Curve::Renderer::DataItem:
******************************************/

Curve::Renderer::DataItem::DataItem(void)
	:lineShader(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBShaderObjects::initExtension();
	GLARBVertexShader::initExtension();
	GLARBFragmentShader::initExtension();
	
	/* Create the line rendering shader: */
	lineShader=glCreateProgramObjectARB();
	}

Curve::Renderer::DataItem::~DataItem(void)
	{
	/* Destroy the line rendering shader: */
	glDeleteObjectARB(lineShader);
	}

/********************************
Methods of class Curve::Renderer:
********************************/

void Curve::Renderer::initContext(GLContextData& contextData) const
	{
	/* Create a data item and associate it with this context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the line rendering shader: */
	GLhandleARB vertexShader=glCompileVertexShaderFromFile(SKETCHPAD_SHADERDIR "/CurveRenderer.vs");
	glAttachObjectARB(dataItem->lineShader,vertexShader);
	glDeleteObjectARB(vertexShader);
	
	/* Figure out what type of geometry shader to use: */
	if(contextData.getContext().isVersionLargerEqual(3,2))
		{
		GLhandleARB geometryShader=glCompileARBGeometryShader4FromFile(SKETCHPAD_SHADERDIR "/CurveRendererCore.gs");
		glAttachObjectARB(dataItem->lineShader,geometryShader);
		glDeleteObjectARB(geometryShader);
		}
	else if(GLARBGeometryShader4::isSupported())
		{
		/* Create a GL_ARB_geometry_shader4 geometry shader: */
		GLARBGeometryShader4::initExtension();
		
		GLhandleARB geometryShader=glCompileARBGeometryShader4FromFile(SKETCHPAD_SHADERDIR "/CurveRendererARB.gs");
		glAttachObjectARB(dataItem->lineShader,geometryShader);
		glDeleteObjectARB(geometryShader);
		
		glProgramParameteriARB(dataItem->lineShader,GL_GEOMETRY_INPUT_TYPE_ARB,GL_LINES);
		glProgramParameteriARB(dataItem->lineShader,GL_GEOMETRY_OUTPUT_TYPE_ARB,GL_TRIANGLE_STRIP);
		glProgramParameteriARB(dataItem->lineShader,GL_GEOMETRY_VERTICES_OUT_ARB,8);
		}
	
	GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(SKETCHPAD_SHADERDIR "/CurveRenderer.fs");
	glAttachObjectARB(dataItem->lineShader,fragmentShader);
	glDeleteObjectARB(fragmentShader);
	
	glLinkAndTestShader(dataItem->lineShader);
	dataItem->uniforms[0]=glGetUniformLocationARB(dataItem->lineShader,"lineWidth");
	dataItem->uniforms[1]=glGetUniformLocationARB(dataItem->lineShader,"pixelSize");
	}

/******************************
Static elements of class Curve:
******************************/

unsigned int Curve::typeCode=0;
Curve::Renderer Curve::renderer;

/**********************
Methods of class Curve:
**********************/

void Curve::draw(const Color& drawColor,GLContextData& contextData) const
	{
	/* Draw the curve: */
	
	/* Install the line rendering shader: */
	Renderer::DataItem* dataItem=contextData.retrieveDataItem<Renderer::DataItem>(&renderer);
	glUseProgramObjectARB(dataItem->lineShader);
	glUniform1fARB(dataItem->uniforms[0],lineWidth);
	
	/* Calculate the size of a pixel in the current navigation transformation: */
	const Vrui::DisplayState& ds=Vrui::getDisplayState(contextData);
	const Vrui::Scalar* panRect=ds.window->getPanRect();
	double pw=ds.screen->getWidth()*(panRect[1]-panRect[0])/double(ds.viewport.size[0]);
	double ph=ds.screen->getHeight()*(panRect[3]-panRect[2])/double(ds.viewport.size[1]);
	double pixelSize=Math::sqrt(pw*ph);
	pixelSize*=Vrui::getInverseNavigationTransformation().getScaling();
	glUniform1fARB(dataItem->uniforms[1],float(pixelSize));
	
	glColor(drawColor);
	if(points.size()>=2)
		{
		/* Draw the curve as a line strip: */
		glBegin(GL_LINE_STRIP);
		std::vector<Point>::const_iterator p0It=points.begin();
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(*p0It);
		
		std::vector<Point>::const_iterator p1It=p0It+1;
		Vector v0=*p1It-*p0It;
		v0.normalize();
		while(true)
			{
			/* Get the next vertex and bail out if there isn't one: */
			std::vector<Point>::const_iterator p2It=p1It+1;
			if(p2It==points.end())
				break;
			
			/* Calculate the separating normal vector between the two adjacent line segments: */
			Vector v1=*p2It-*p1It;
			v1.normalize();
			glNormal(v0+v1);
			glVertex(*p1It);
			
			/* Go to the next line segment: */
			p0It=p1It;
			p1It=p2It;
			v0=v1;
			}
		
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(*p1It);
		
		glEnd();
		}
	else
		{
		/* Draw a single point as a line with identical end points: */
		glBegin(GL_LINES);
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(points[0]);
		glEnd();
		}
	
	glUseProgramObjectARB(0);
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
	/* Create a new curve object: */
	Curve* result=new Curve;
	
	/* Copy this curve's bounding box, parameters, and point vector: */
	result->boundingBox=boundingBox;
	result->color=color;
	result->lineWidth=lineWidth;
	result->points=points;
	
	return result;
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
	}

void Curve::rubout(const Capsule& eraser,SketchObjectContainer& container)
	{
	/* Create a temporary list of curve points outside the capsule: */
	std::vector<Point> outside;
	Box outsideBox=Box::empty;
	
	/* Check if the curve's beginning vertex lies within the capsule: */
	std::vector<Point>::iterator p0It=points.begin();
	bool inside=eraser.isInside(*p0It);
	
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
				Curve* newCurve=new Curve;
				newCurve->boundingBox=outsideBox;
				newCurve->color=color;
				newCurve->lineWidth=lineWidth;
				std::swap(newCurve->points,outside);
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

void Curve::read(IO::File& file,SketchObjectCreator& creator)
	{
	/* Read the color and line width: */
	for(int i=0;i<4;++i)
		color[i]=file.read<Misc::UInt8>();
	lineWidth=file.read<Misc::Float32>();
	
	/* Read the number of points: */
	size_t numPoints=file.read<Misc::UInt16>();
	
	/* Read a new point vector and calculate a new bounding box: */
	std::vector<Point> newPoints;
	newPoints.reserve(numPoints);
	Box newBoundingBox=Box::empty;
	for(size_t i=0;i<numPoints;++i)
		{
		Point p;
		for(int j=0;j<3;++j)
			p[j]=file.read<Misc::Float32>();
		newPoints.push_back(p);
		newBoundingBox.addPoint(p);
		}
	
	/* Swap the old and new bounding box and point vector: */
	boundingBox=newBoundingBox;
	std::swap(points,newPoints);
	}

void Curve::setGLState(GLContextData& contextData) const
	{
	#if 0 // Not necessary -- it's the default state
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	
	#endif
	}

void Curve::glRenderAction(GLContextData& contextData) const
	{
	/* Draw the curve using its color: */
	draw(color,contextData);
	}

void Curve::glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const
	{
	/* Calculate the current highlight color: */
	Color highlight=cycle>=Scalar(0)?Color(255U,255U,255U):Color(0U,0U,0U);
	cycle=Math::abs(cycle);
	for(int i=0;i<4;++i)
		highlight[i]=GLubyte(Math::floor(Scalar(color[i])*(Scalar(1)-cycle)+Scalar(highlight[i])*cycle+Scalar(0.5)));
	
	/* Draw the curve using the highlight color: */
	draw(highlight,contextData);
	}

void Curve::resetGLState(GLContextData& contextData) const
	{
	#if 0 // Not necessary -- it's the default state
	
	/* Reset OpenGL state: */
	glPopAttrib();
	
	#endif
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
	current=new Curve;
	current->color=settings.getColor();
	current->lineWidth=settings.getLineWidth();
	
	/* Revert to curve mode: */
	lineMode=false;
	
	/* Append the first curve point: */
	current->points.push_back(pos);
	current->boundingBox.addPoint(pos);
	
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
			Curve* line=new Curve;
			line->color=current->color;
			line->lineWidth=current->lineWidth;
			
			/* Find the line's starting point: */
			Point first=current->points.front();
			if(firstNeighborhood)
				{
				/* Snap the first curve point to nearby sketch objects: */
				first=settings.snap(first);
				}
			
			/* Create the line: */
			line->points.push_back(first);
			line->boundingBox.addPoint(first);
			line->points.push_back(pos);
			
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

void CurveFactory::glRenderAction(GLContextData& contextData) const
	{
	if(current!=0)
		{
		/* Draw the currently created curve: */
		current->setGLState(contextData);
		current->glRenderAction(contextData);
		current->resetGLState(contextData);
		}
	}
