/***********************************************************************
Group - Class for groups of sketching objects.
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

#include "Group.h"

#include <Misc/SizedTypes.h>
#include <IO/File.h>

#include "Capsule.h"
#include "SketchObjectCreator.h"

/******************************
Static elements of class Group:
******************************/

unsigned int Group::typeCode=0;

/**********************
Methods of class Group:
**********************/

unsigned int Group::getTypeCode(void) const
	{
	return typeCode;
	}

bool Group::pick(const Point& center,Scalar radius2) const
	{
	/* Pick all members of the group: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		if(soIt->pick(center,radius2))
			return true;
	
	return false;
	}

SketchObject::SnapResult Group::snap(const Point& center,Scalar radius2) const
	{
	/* Initialize the snap result: */
	SnapResult result;
	result.valid=false;
	result.dist2=radius2;
	
	/* Snap against all members of the group: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		SnapResult sr=soIt->snap(center,result.dist2);
		if(sr.valid)
			result=sr;
		}
	
	return result;
	}

SketchObject* Group::clone(void) const
	{
	/* Create a new group object: */
	Group* result=new Group;
	
	/* Clone all members of the group: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		result->append(soIt->clone());
	
	return result;
	}

void Group::applySettings(const SketchSettings& settings)
	{
	/* Apply settings to all members of the group: */
	for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		soIt->applySettings(settings);
	}

void Group::transform(const Transformation& transform)
	{
	/* Transform all members of the group and re-calculate the bounding box: */
	boundingBox=Box::empty;
	for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		soIt->transform(transform);
		boundingBox.addBox(soIt->getBoundingBox());
		}
	}

void Group::snapToGrid(Scalar gridSize)
	{
	/* Snap all members of the group to the grid and re-calculate the bounding box: */
	boundingBox=Box::empty;
	for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		soIt->snapToGrid(gridSize);
		boundingBox.addBox(soIt->getBoundingBox());
		}
	}

void Group::rubout(const Capsule& eraser,SketchObjectContainer& container)
	{
	/* Rub out all members of the group whose bounding boxes touch the eraser capsule: */
	SketchObjectList::iterator soIt=sketchObjects.begin();
	while(soIt!=sketchObjects.end())
		{
		/* Get an iterator to the next object: */
		SketchObjectList::iterator nextIt=soIt;
		++nextIt;
		
		if(eraser.doesIntersect(soIt->getBoundingBox()))
			soIt->rubout(eraser,*this);
		
		/* Go to the next object: */
		soIt=nextIt;
		}
	
	/* Check if the group became empty: */
	if(sketchObjects.empty())
		{
		/* Delete this group: */
		container.remove(this);
		}
	else
		{
		/* Re-calculate the group's bounding box: */
		boundingBox=Box::empty;
		for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
			boundingBox.addBox(soIt->getBoundingBox());
		}
	}

void Group::write(IO::File& file,const SketchObjectCreator& creator) const
	{
	/* Write the number of group members: */
	file.write<Misc::UInt16>(sketchObjects.size());
	
	/* Write all members of the group: */
	for(SketchObjectList::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		/* Write the member: */
		creator.writeObject(&*soIt,file);
		}
	}

void Group::read(IO::File& file,SketchObjectCreator& creator)
	{
	/* Read the number of group members: */
	size_t numMembers=file.read<Misc::UInt16>();
	
	/* Read all members of the group and calculate a new bounding box: */
	SketchObjectList newSketchObjects;
	Box newBoundingBox=Box::empty;
	for(size_t i=0;i<numMembers;++i)
		{
		/* Read the new member: */
		SketchObject* newMember=creator.readObject(file);
		
		/* Add the new member to the list and update the bounding box: */
		newSketchObjects.push_back(newMember);
		newBoundingBox.addBox(newMember->getBoundingBox());
		}
	
	/* Install the new bounding box and member list: */
	boundingBox=newBoundingBox;
	sketchObjects.clear();
	newSketchObjects.transfer(sketchObjects);
	}

void Group::glRenderAction(GLContextData& contextData) const
	{
	/* Draw all members of the group: */
	drawObjects(contextData);
	}

void Group::glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const
	{
	/* Highlight all members of the group: */
	drawObjectsHighlight(cycle,contextData);
	}

void Group::append(SketchObject* newObject)
	{
	/* Call base class method: */
	SketchObjectContainer::append(newObject);
	
	/* Add the new object to the bounding box: */
	boundingBox.addBox(newObject->getBoundingBox());
	}

void Group::insertAfter(SketchObject* pred,SketchObject* newObject)
	{
	/* Call base class method: */
	SketchObjectContainer::insertAfter(pred,newObject);
	
	/* Add the new object to the bounding box: */
	boundingBox.addBox(newObject->getBoundingBox());
	}

void Group::transferMembers(SketchObjectList& receiver)
	{
	/* Transfer the list of sketch objects and reset the bounding box to empty: */
	sketchObjects.transfer(receiver);
	boundingBox=Box::empty;
	}
