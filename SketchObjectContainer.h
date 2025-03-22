/***********************************************************************
SketchObjectContainer - Base class for containers of sketch objects.
Copyright (c) 2016-2019 Oliver Kreylos
***********************************************************************/

#ifndef SKETCHOBJECTCONTAINER_INCLUDED
#define SKETCHOBJECTCONTAINER_INCLUDED

#include "SketchGeometry.h"
#include "SketchObject.h"
#include "SketchObjectList.h"

/* Forward declarations: */
class GLContextData;

class SketchObjectContainer
	{
	/* Elements: */
	protected:
	SketchObjectList sketchObjects; // List of sketch objects in the container
	
	/* Protected methods: */
	void drawObjects(GLContextData& contextData) const; // Draws all sketch objects in the container in order
	void drawObjectsHighlight(Scalar cycle,GLContextData& contextData) const; // Highlights all sketch objects in the container in order using the given cycle in [-1, 1]
	
	/* Constructors and destructors: */
	public:
	virtual ~SketchObjectContainer(void); // Destroys all sketch objects in the container
	
	/* Methods: */
	const SketchObjectList& getSketchObjects(void) const // Returns the list of sketch objects in the container
		{
		return sketchObjects;
		}
	SketchObjectList& getSketchObjects(void) // Ditto
		{
		return sketchObjects;
		}
	virtual void append(SketchObject* newObject); // Appens the given object to the container's list
	virtual void insertAfter(SketchObject* pred,SketchObject* newObject); // Inserts the given object after the given predecessor
	virtual void remove(SketchObject* object); // Removes the given object from the container
	virtual SketchObject* pickTop(const Point& pos,Scalar radius2); // Picks the top-most object touched by the given sphere
	virtual SketchObject::SnapResult snap(const Point& center,Scalar radius2) const; // Snaps the given position against all sketch objects
	};

#endif
