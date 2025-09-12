/***********************************************************************
SketchObject - Base class for sketching objects for a simple sketching
application.
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

#ifndef SKETCHOBJECT_INCLUDED
#define SKETCHOBJECT_INCLUDED

#include <Math/Math.h>

#include "SketchGeometry.h"

/* Forward declarations: */
namespace IO {
class File;
}
class Capsule;
class RenderState;
class SketchObjectList;
class SketchObjectContainer;
class SketchSettings;
class SketchObjectCreator;

class SketchObject
	{
	friend class SketchObjectList;
	
	/* Embedded classes: */
	public:
	struct PickResult // Structure to return the result of a pick query
		{
		/* Elements: */
		public:
		Point center; // Pick sphere's center
		Scalar radius2; // Pick sphere's squared radius
		SketchObject* pickedObject; // Currently picked object; ==0 if no object has been picked
		unsigned int pickedPriority; // Priority of the currently picked object; smaller number = higher priority
		Scalar pickedDist2; // Squared distance from pick sphere's center to currently picked object
		Point pickedPoint; // Picked position on currently picked object
		
		/* Constructors and destructors: */
		PickResult(const Point& sCenter,Scalar sRadius) // Creates a pick query for the given center point and radius
			:center(sCenter),radius2(Math::sqr(sRadius)),
			 pickedObject(0),pickedPriority(~0U),pickedDist2(radius2)
			{
			}
		
		/* Methods: */
		bool isValid(void) const // Returns true if an object has been picked
			{
			return pickedObject!=0;
			}
		bool update(SketchObject* object,unsigned int priority,Scalar dist2,const Point& point) // Potentially updates the pick result from picking the given object; returns true if pick result changed
			{
			bool pickChanged=(priority<pickedPriority&&dist2<radius2)||(priority==pickedPriority&&dist2<pickedDist2);
			if(pickChanged)
				{
				/* Update the pick result: */
				pickedObject=object;
				pickedPriority=priority;
				pickedDist2=dist2;
				pickedPoint=point;
				}
			
			return pickChanged;
			}
		bool update(SketchObject* object,unsigned int priority,const Point& point) // Ditto; calculates distance from pick sphere's center to given point
			{
			Scalar dist2=Geometry::sqrDist(center,point);
			bool pickChanged=(priority<pickedPriority&&dist2<radius2)||(priority==pickedPriority&&dist2<pickedDist2);
			if(pickChanged)
				{
				/* Update the pick result: */
				pickedObject=object;
				pickedPriority=priority;
				pickedDist2=dist2;
				pickedPoint=point;
				}
			
			return pickChanged;
			}
		bool update(SketchObject* object,const Point& start,const Point& end) // Potentially updates the pick result from picking a line segment; returns true if pick result changed
			{
			bool pickChanged=false;
			
			/* Bail out if a vertex has already been picked: */
			if(pickedPriority>=1U)
				{
				/* Check the line segment: */
				Vector dir=end-start;
				Scalar dir2=dir.sqr();
				if(dir2>Scalar(0))
					{
					/* Check if the sphere's center is inside the line segment's extents: */
					Point mid=Geometry::mid(start,end);
					Vector mc=center-mid;
					Scalar y=dir*mc;
					if(Scalar(2)*Math::abs(y)<dir2)
						{
						/* Check if this line segment is closer than the currently picked object: */
						Scalar dist2=mc.sqr()-Math::sqr(y)/dir2;
						if((1U<pickedPriority&&dist2<radius2)||(1U==pickedPriority&&dist2<pickedDist2))
							{
							/* Update the pick result: */
							pickedObject=object;
							pickedPriority=1U;
							pickedDist2=dist2;
							pickedPoint=Geometry::addScaled(mid,dir,y/dir2);
							pickChanged=true;
							}
						}
					}
				}
			
			return pickChanged;
			}
		};
	
	/* Elements: */
	private:
	SketchObject* pred; // Pointer to sketch object's predecessor in a linked list
	SketchObject* succ; // Pointer to sketch object's successor in a linked list
	protected:
	Box boundingBox; // Axis-aligned box bounding the sketch object
	
	/* Constructors and destructors: */
	public:
	SketchObject(void)
		:pred(0),succ(0),boundingBox(Box::empty)
		{
		}
	virtual ~SketchObject(void);
	
	/* Methods: */
	const Box& getBoundingBox(void) const // Returns the sketch object's bounding box
		{
		return boundingBox;
		}
	virtual unsigned int getTypeCode(void) const =0; // Returns an integer uniquely identifying a sketching object class
	virtual bool pick(PickResult& result) =0; // Picks this object with the given pick query; updates query object and returns true if object is picked
	virtual SketchObject* clone(void) const =0; // Creates an identical copy of the sketch object
	virtual void applySettings(const SketchSettings& settings) =0; // Applies settings from the given settings object to the sketch object
	virtual void transform(const Transformation& transform) =0; // Transforms the sketch object with the given transformation
	virtual void snapToGrid(Scalar gridSize) =0; // Snaps the sketch object to a grid of the given grid spacing
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container) =0; // Erases the part of the object that lies within the capsule defined by the two center points and the radius
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const =0; // Writes the sketch object to the given binary file
	virtual void glRenderAction(RenderState& renderState) const =0; // Renders the sketch object
	virtual void glRenderActionHighlight(Scalar cycle,RenderState& renderState) const =0; // Highlights the sketch object
	};

class SketchObjectFactory
	{
	/* Elements: */
	protected:
	SketchSettings& settings; // Settings to use for new sketch objects
	unsigned int settingsVersion; // Version number of sketch settings of currently created object
	
	/* Constructors and destructors: */
	public:
	SketchObjectFactory(SketchSettings& sSettings)
		:settings(sSettings),
		 settingsVersion(0U)
		{
		}
	virtual ~SketchObjectFactory(void); // Interrupts creation of a current sketch object and destroys the factory
	
	/* Methods: */
	virtual void buttonDown(const Point& pos) =0; // Registers a button press at the given position
	virtual void motion(const Point& pos,bool lingering,bool firstNeighborhood) =0; // Registers a motion while the button is down to the given position
	virtual bool buttonUp(const Point& pos) =0; // Registers a button release at the given position; returns true if the currently created sketch object is finished
	virtual SketchObject* finish(void) =0; // Finishes and returns the currently created sketch object
	virtual void glRenderAction(RenderState& renderState) const =0; // Renders the sketch object factory's state
	};

#endif
