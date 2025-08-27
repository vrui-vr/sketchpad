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
class GLContextData;
class Capsule;
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
		Scalar radius2; // Pick sphere's squared radius, or squared distance to currently picked point
		SketchObject* pickedObject; // Currently picked object; ==0 if no object has been picked
		Point pickedPoint; // Picked position on currently picked object
		
		/* Constructors and destructors: */
		PickResult(const Point& sCenter,Scalar sRadius) // Creates a pick query for the given center point and radius
			:center(sCenter),radius2(Math::sqr(sRadius)),
			 pickedObject(0)
			{
			}
		
		/* Methods: */
		bool isValid(void) const // Returns true if an object has been picked
			{
			return pickedObject!=0;
			}
		bool update(Scalar dist2,SketchObject* object,const Point& point) // Potentially updates the pick result from picking the given object; returns true if pick result changed
			{
			bool pickChanged=radius2>dist2;
			if(pickChanged)
				{
				radius2=dist2;
				pickedObject=object;
				pickedPoint=point;
				}
			
			return pickChanged;
			}
		};
	
	struct SnapResult // Structure to return the result of a snap query
		{
		/* Elements: */
		public:
		bool valid; // Flag whether the snap request succeeded
		Scalar dist2; // Squared snap distance
		Point position; // Snap position
		Vector normal; // Normal vector at the snap position
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
	virtual bool pick(PickResult& result) const =0; // Picks this object with the given pick query; updates query object and returns true if object is picked
	virtual bool pick(const Point& center,Scalar radius2) const =0; // Returns true if the sphere of the given center and radius touches the sketch object
	virtual SnapResult snap(const Point& center,Scalar radius2) const =0; // Snaps the given point to the object, moving it by at most the given radius
	virtual SketchObject* clone(void) const =0; // Creates an identical copy of the sketch object
	virtual void applySettings(const SketchSettings& settings) =0; // Applies settings from the given settings object to the sketch object
	virtual void transform(const Transformation& transform) =0; // Transforms the sketch object with the given transformation
	virtual void snapToGrid(Scalar gridSize) =0; // Snaps the sketch object to a grid of the given grid spacing
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container) =0; // Erases the part of the object that lies within the capsule defined by the two center points and the radius
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const =0; // Writes the sketch object to the given binary file
	virtual void read(IO::File& file,SketchObjectCreator& creator) =0; // Reads the sketch object from the given binary file
	virtual void setGLState(GLContextData& contextData) const; // Prepares the given OpenGL context for rendering a series of sketch objects of this object's class
	virtual void glRenderAction(GLContextData& contextData) const =0; // Renders the sketch object into the given OpenGL context
	virtual void glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const =0; // Highlights the sketch object into the given OpenGL context using the given cycle value in [-1, 1]
	virtual void resetGLState(GLContextData& contextData) const; // Resets the given OpenGL context after rendering a series of sketch objects of this object's class
	};

class SketchObjectFactory
	{
	/* Elements: */
	protected:
	const SketchSettings& settings; // Settings to use for new sketch objects
	unsigned int settingsVersion; // Version number of sketch settings of currently created object
	
	/* Constructors and destructors: */
	public:
	SketchObjectFactory(const SketchSettings& sSettings)
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
	virtual void glRenderAction(GLContextData& contextData) const =0; // Renders the sketch object factory's state into the given OpenGL context
	};

#endif
