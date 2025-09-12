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
#include <Math/Matrix.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>

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
			split=(cps[1]-cps[0])*d<-tolerance*dLen||(cps[2]-cps[3])*d>tolerance*dLen;
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
	/* Calculate the current highlight color: */
	Color highlight=cycle>=Scalar(0)?Color(255U,255U,255U):Color(0U,0U,0U);
	cycle=Math::abs(cycle);
	for(int i=0;i<4;++i)
		highlight[i]=GLubyte(Math::floor(Scalar(color[i])*(Scalar(1)-cycle)+Scalar(highlight[i])*cycle+Scalar(0.5)));
	
	/* Draw the spline using a polyline renderer: */
	renderState.setRenderer(renderer);
	if(renderer->draw(this,version,highlight,lineWidth,renderState.getDataItem()))
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

/******************************
Methods of class SplineFactory:
******************************/

namespace {

/****************
Helper functions:
****************/

inline void calcBernsteinPolynomials3(double param,double b[4]) // Calculates cubic Bernstein polynomials for the given parameter value
	{
	/* Calculate the Bernstein polynomials directly: */
	double t01=param;
	double t11=1.0-param;
	double t02=t01*t01;
	double t12=t11*t11;
	b[0]=t12*t11;
	b[1]=3.0*t12*t01;
	b[2]=3.0*t11*t02;
	b[3]=t02*t01;
	}

inline void calcBernsteinPolynomials2(double param,double b[3]) // Calculates quadratic Bernstein polynomials for the given parameter value
	{
	/* Calculate the Bernstein polynomials directly: */
	double t01=param;
	double t11=1.0-param;
	b[0]=t11*t11;
	b[1]=2.0*t01*t11;
	b[2]=t01*t01;
	}

}

void SplineFactory::fitLinear(Point c[4],const Point& c0,const Point& c3) const
	{
	/* Create a constant-derivative linear Bezier curve: */
	c[0]=c0;
	c[1]=Geometry::affineCombination(c0,c3,Scalar(1.0/3.0));
	c[2]=Geometry::affineCombination(c0,c3,Scalar(2.0/3.0));
	c[3]=c3;
	}

void SplineFactory::fitQuadratic(Point c[4],const Point& c0,const Point& c3) const
	{
	/* Try fitting a quadratic Bezier curve to the set of active input points: */
	double ata[3],atb[3];
	for(int dim=0;dim<3;++dim)
		atb[dim]=ata[dim]=0.0;
	
	for(InputCurve::const_iterator icIt=inputCurve.begin();icIt!=inputCurve.end();++icIt)
		{
		/* Calculate the input point's quadratic Bernstein polynomials: */
		double b[3];
		calcBernsteinPolynomials2(icIt->param/inputCurveLength,b);
		
		/* Add the input point to the least-squares system: */
		for(int dim=0;dim<3;++dim)
			{
			ata[dim]+=Math::sqr(b[1]);
			atb[dim]+=b[1]*(icIt->pos[dim]-b[0]*c0[dim]-b[2]*c3[dim]);
			}
		}
	
	/* Check if all three linear systems have solutions: */
	if(ata[0]!=0.0&&ata[1]!=0.0&&ata[2]!=0.0)
		{
		/* Solve the linear system: */
		Point c12;
		for(int dim=0;dim<3;++dim)
			c12[dim]=atb[dim]/ata[dim];
		
		/* Rank-elevate the resulting quadratic Bezier curve: */
		c[0]=c0;
		c[1]=Geometry::affineCombination(c0,c12,Scalar(2.0/3.0));
		c[2]=Geometry::affineCombination(c12,c3,Scalar(1.0/3.0));
		c[3]=c3;
		}
	else
		{
		/* Just go ahead and "fit" a linear Bezier curve and call it a day: */
		fitLinear(c,c0,c3);
		}
	}

void SplineFactory::fitCubic(Point c[4],const Point& c0,const Point& c3) const
	{
	/* Try fitting a cubic Bezier curve to the set of active input points: */
	try
		{
		for(int dim=0;dim<3;++dim)
			{
			/* Create the least-squares system: */
			Math::Matrix ata(2,2,0.0);
			Math::Matrix atb(2,1,0.0);
			for(InputCurve::const_iterator icIt=inputCurve.begin();icIt!=inputCurve.end();++icIt)
				{
				/* Calculate the input point's Bernstein polynomials: */
				double b[4];
				calcBernsteinPolynomials3(icIt->param/inputCurveLength,b);
				
				/* Add the input point to the least-squares system: */
				for(int i=0;i<2;++i)
					{
					for(int j=0;j<2;++j)
						ata(i,j)+=b[i+1]*b[j+1];
					atb(i)+=b[i+1]*(icIt->pos[dim]-b[0]*c0[dim]-b[3]*c3[dim]);
					}
				}
			
			/* Solve the least-squares system: */
			Math::Matrix x=atb.divideFullPivot(ata);
			c[0]=c0;
			for(int i=0;i<2;++i)
				c[i+1][dim]=x(i);
			c[3]=c3;
			}
		}
	catch(const Math::Matrix::RankDeficientError&)
		{
		/* Fall back to quadratic fitting: */
		fitQuadratic(c,c0,c3);
		}
	}

void SplineFactory::fitQuadraticG1(Point c[4],const Point& c0,const Vector& t0,const Point& c3) const
	{
	/* Try fitting a quadratic Bezier curve to the set of active input points: */
	double ata(0.0);
	double atb(0.0);
	for(InputCurve::const_iterator icIt=inputCurve.begin();icIt!=inputCurve.end();++icIt)
		{
		/* Calculate the input point's quadratic Bernstein polynomials: */
		double b[3];
		calcBernsteinPolynomials2(icIt->param/inputCurveLength,b);
		
		/* Add the input point's three equations to the least-squares system: */
		ata+=b[1];
		for(int dim=0;dim<3;++dim)
			atb+=b[1]*t0[dim]*(icIt->pos[dim]-(b[0]+b[1])*c0[dim]-b[2]*c3[dim]);
		}
	
	/* Check if the linear system has a solution: */
	Point c12;
	if(ata!=0.0)
		{
		/* Solve the linear system: */
		c12=Geometry::addScaled(c0,t0,Scalar(atb/ata));
		}
	else
		{
		/* Just make whatever: */
		c12=addScaled(c0,t0,Scalar(Geometry::dist(c0,c3)*0.5));
		}
	
	/* Rank-elevate the resulting quadratic Bezier curve: */
	c[0]=c0;
	c[1]=Geometry::affineCombination(c0,c12,Scalar(2.0/3.0));
	c[2]=Geometry::affineCombination(c12,c3,Scalar(1.0/3.0));
	c[3]=c3;
	}

void SplineFactory::fitCubicG1(Point c[4],const Point& c0,const Vector& t0,const Point& c3) const
	{
	/* Try fitting a cubic Bezier curve to the set of active input points: */
	try
		{
		/* Solve a least-squares linear system to calculate the interior Bezier curve control points: */
		Math::Matrix ata(4,4,0.0);
		Math::Matrix atb(4,1,0.0);
		for(InputCurve::const_iterator icIt=inputCurve.begin();icIt!=inputCurve.end();++icIt)
			{
			/* Calculate the input point's Bernstein polynomials: */
			double b[4];
			calcBernsteinPolynomials3(icIt->param/inputCurveLength,b);
			
			/* Add the input point's three equations to the least-squares system: */
			ata(0,0)+=b[1]*b[1];
			for(int dim=0;dim<3;++dim)
				{
				ata(0,1+dim)+=b[1]*b[2]*t0[dim];
				ata(1+dim,0)+=b[1]*b[2]*t0[dim];
				ata(1+dim,1+dim)+=b[2]*b[2];
				
				double rhs=icIt->pos[dim]-(b[0]+b[1])*c0[dim]-b[3]*c3[dim];
				atb(0)+=b[1]*t0[dim]*rhs;
				atb(1+dim)+=b[2]*rhs;
				}
			}
		
		/* Solve the least-squares system: */
		Math::Matrix x=atb.divideFullPivot(ata);
		c[0]=c0;
		c[1]=Geometry::addScaled(c0,t0,Scalar(x(0)));
		c[2]=Point(x(1),x(2),x(3));
		c[3]=c3;
		}
	catch(const Math::Matrix::RankDeficientError&)
		{
		/* Fall back to quadratic fitting: */
		fitQuadraticG1(c,c0,t0,c3);
		}
	}

SplineFactory::SplineFactory(SketchSettings& sSettings)
	:SketchObjectFactory(sSettings),
	 current(0)
	{
	}

SplineFactory::~SplineFactory(void)
	{
	/* Delete the current spline: */
	delete current;
	}

void SplineFactory::buttonDown(const Point& pos)
	{
	/* Initialize linger detection: */
	lastLinger=false;
	
	/* Calculate the current tolerance: */
	Scalar tolerance(Vrui::getUiSize()*Vrui::getInverseNavigationTransformation().getScaling());
	
	/* Initialize the input curve: */
	inputCurve.clear();
	inputCurve.push_back(InputPoint(pos,tolerance,0,0));
	inputCurveLength=Scalar(0);
	
	/* Initialize the first spline segment: */
	for(int i=0;i<4;++i)
		controlPoints[i]=pos;
	g1=false;
	
	/* Delete a pending current spline and create a new one: */
	delete current;
	current=new Spline(settings.getColor(),settings.getLineWidth(),controlPoints);
	fixedBox=Box(pos,pos);
	}

void SplineFactory::motion(const Point& pos,bool lingering,bool firstNeighborhood)
	{
	/* Calculate the current tolerance: */
	Scalar tolerance(Vrui::getUiSize()*Vrui::getInverseNavigationTransformation().getScaling());
	
	/* Add the input point to the input curve: */
	Scalar dist(Geometry::dist(inputCurve.back().pos,pos));
	inputCurveLength+=dist;
	inputCurve.push_back(InputPoint(pos,tolerance,dist,inputCurveLength));
	
	/* Re-fit the current spline segment: */
	Point newC[4];
	if(g1)
		fitCubicG1(newC,controlPoints[0],t0,pos);
	else
		fitCubic(newC,controlPoints[0],pos);
	for(int i=0;i<4;++i)
		controlPoints[i]=newC[i];
	
	/* Update the current spline: */
	std::vector<Point>::iterator segmentStart=current->points.end()-4;
	current->boundingBox=fixedBox;
	for(int i=0;i<4;++i)
		{
		segmentStart[i]=controlPoints[i];
		current->boundingBox.addPoint(controlPoints[i]);
		}
	++current->version;
	
	/* Remember if the tool is lingering: */
	lastLinger=lingering;
	}

bool SplineFactory::buttonUp(const Point& pos)
	{
	/* Tell the caller that the spline is done: */
	return true;
	}

SketchObject* SplineFactory::finish(void)
	{
	/* Return the current spline object: */
	SketchObject* result=current;
	current=0;
	return result;
	}

void SplineFactory::glRenderAction(RenderState& renderState) const
	{
	/* Draw the current spline if it exists: */
	if(current!=0)
		current->glRenderAction(renderState);
	}
