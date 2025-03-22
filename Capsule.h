/***********************************************************************
Capsule - Class for cylinders with two hemispherical end caps.
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

#ifndef CAPSULE_INCLUDED
#define CAPSULE_INCLUDED

#include <utility>
#include <Math/Math.h>
#include <Math/Interval.h>

#include "SketchGeometry.h"

class Capsule
	{
	/* Embedded classes: */
	public:
	typedef Math::Interval<Scalar> Interval; // Type for closed intervals
	
	/* Elements: */
	private:
	Point c0; // First endpoint of cylinder axis
	Point c1; // Second endpoint of cylinder axis
	Scalar radius; // Cylinder and endcap radius
	
	/* Cached derived state: */
	Point center; // Center point of axis
	Vector axis; // Half vector from c0 to c1
	Scalar axisLen2; // Squared length of axis
	Scalar radius2; // Squared radius
	
	/* Constructors and destructors: */
	public:
	Capsule(void)
		{
		}
	Capsule(const Point& sC0,const Point& sC1,Scalar sRadius)
		:c0(sC0),c1(sC1),radius(sRadius),
		 center(Geometry::mid(c0,c1)),axis((c1-c0)*Scalar(0.5)),axisLen2(axis.sqr()),radius2(Math::sqr(radius))
		{
		}
	
	/* Methods: */
	const Point& getC0(void) const // Returns the first axis end point
		{
		return c0;
		}
	const Point& getC1(void) const // Returns the second axis end point
		{
		return c1;
		}
	Scalar getRadius(void) const // Returns the cylinder radius
		{
		return radius;
		}
	Scalar getAxisLen2(void) const // Returns the squared cylinder axis length
		{
		return axisLen2;
		}
	bool isInside(const Point& p) const // Returns true if the given point is inside the capsule
		{
		/* Check the sphere around the first axis endpoint: */
		if(Geometry::sqrDist(p,c0)<=radius2)
			return true;
		
		/* Check the sphere around the second axis endpoint: */
		if(Geometry::sqrDist(p,c1)<=radius2)
			return true;
		
		/* Check the cylinder: */
		Vector pc=p-center;
		Scalar y2=Math::sqr(pc*axis)/axisLen2;
		if(y2<=axisLen2&&Geometry::sqr(pc)-y2<=radius2)
			return true;
		
		/* Point is not inside: */
		return false;
		}
	bool doesIntersect(const Box& box) const // Returns true if there is a very high probability that the capsule intersects the given box
		{
		/* Clip the line segment defined by c0 and c1 against the box, extruded outwards by radius: */
		Interval intersect=Interval::full;
		
		/* Process each primary axis independently: */
		for(int i=0;i<3;++i)
			{
			/* Get the line's delta along this dimension: */
			Scalar d=c1[i]-c0[i];
			if(d>Scalar(0))
				{
				/* Intersect with the box's minimum first, then the maximum: */
				intersect.intersectInterval(Interval((box.min[i]-radius-c0[i])/d,(box.max[i]+radius-c0[i])/d));
				}
			else if(d<Scalar(0))
				{
				/* Intersect with the box's maximum first, then the minimum: */
				intersect.intersectInterval(Interval((box.max[i]+radius-c0[i])/d,(box.min[i]-radius-c0[i])/d));
				}
			else
				{
				/* Bail out if the line segment is completely outside the box: */
				if(c0[i]<box.min[i]-radius||c0[i]>box.max[i]+radius)
					return false;
				}
			}
		
		/* Line intersects if the intersection interval is not empty: */
		return !intersect.isNull();
		}
	Interval intersectLine(const Point& p0,const Point& p1) const // Intersects a line segment with the capsule; returns interval of line parameters inside capsule
		{
		/* Initialize the result interval to empty: */
		Interval result=Interval::empty;
		
		/* Calculate the line segment length: */
		Vector d=p1-p0;
		Scalar d2=d.sqr();
		
		/* Return an empty interval if the line segment has zero length: */
		if(d2==Scalar(0))
			return result;
		
		/* Intersect the line segment with the first end cap: */
		Vector p0c0=p0-c0;
		{
		Scalar bh=d*p0c0;
		Scalar c=p0c0.sqr()-radius2;
		Scalar disc=bh*bh-d2*c;
		if(disc>=Scalar(0))
			{
			disc=Math::sqrt(disc);
			result.addInterval(Interval((-bh-disc)/d2,(-bh+disc)/d2));
			}
		}
		
		/* Intersect the line segment with the second end cap: */
		Vector p0c1=p0-c1;
		{
		Scalar bh=d*p0c1;
		Scalar c=p0c1.sqr()-radius2;
		Scalar disc=bh*bh-d2*c;
		if(disc>=Scalar(0))
			{
			disc=Math::sqrt(disc);
			result.addInterval(Interval((-bh-disc)/d2,(-bh+disc)/d2));
			}
		}
		
		/* Intersect the line segment with the infinite slab containing the cylinder: */
		Interval cylinder;
		Scalar da=d*axis;
		if(da!=Scalar(0))
			{
			Scalar l0=-(p0c0*axis)/da;
			Scalar l1=-(p0c1*axis)/da;
			cylinder=Interval(Math::min(l0,l1),Math::max(l0,l1));
			}
		else if(Math::sqr((p0-center)*axis)/axisLen2<=axisLen2)
			cylinder=Interval::full;
		else
			cylinder=Interval::empty;
		
		/* No need to intersect with the cylinder if the line doesn't hit the slab: */
		if(!cylinder.isNull())
			{
			/* Intersect the line segment with the cylinder: */
			Scalar d2da2=d2-da*da/axisLen2;
			if(d2da2!=Scalar(0))
				{
				Vector p0c=p0-center;
				Scalar p0ca=p0c*axis;
				Scalar bh=p0c*d-p0ca*da/axisLen2;
				Scalar c=p0c.sqr()-p0ca*p0ca/axisLen2-radius2;
				Scalar disc=bh*bh-d2da2*c;
				if(disc>=Scalar(0))
					{
					disc=Math::sqrt(disc);
					cylinder.intersectInterval(Interval((-bh-disc)/d2da2,(-bh+disc)/d2da2));
					
					if(!cylinder.isNull())
						{
						/* Join the cylinder intersection with the cap intersections: */
						result.addInterval(cylinder);
						}
					}
				}
			}
		
		return result;
		}
	};

#endif
