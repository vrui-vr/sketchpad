/***********************************************************************
Group - Class for groups of sketching objects.
Copyright (c) 2016-2019 Oliver Kreylos
***********************************************************************/

#ifndef GROUP_INCLUDED
#define GROUP_INCLUDED

#include <vector>

#include "SketchObject.h"
#include "SketchObjectContainer.h"

class Group:public SketchObject,public SketchObjectContainer
	{
	friend class SketchObjectCreator;
	
	/* Elements: */
	private:
	static unsigned int typeCode; // The group class's type code
	
	/* Methods from SketchObject: */
	public:
	virtual unsigned int getTypeCode(void) const;
	virtual bool pick(const Point& center,Scalar radius2) const;
	virtual SnapResult snap(const Point& center,Scalar radius2) const;	
	virtual SketchObject* clone(void) const;
	virtual void applySettings(const SketchSettings& settings);
	virtual void transform(const Transformation& transform);
	virtual void snapToGrid(Scalar gridSize);
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container);
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const;
	virtual void read(IO::File& file,SketchObjectCreator& creator);
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual void glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const;
	
	/* Methods from SketchObjectContainer: */
	virtual void append(SketchObject* newObject);
	virtual void insertAfter(SketchObject* pred,SketchObject* newObject);
	
	/* New methods: */
	void transferMembers(SketchObjectList& receiver); // Appends all group members to the given list and clears the group
	};

#endif
