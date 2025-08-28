/***********************************************************************
SketchSettings - Class to represent a set of parameters to create sketch
objects.
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

#include "SketchSettings.h"

#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>

#include "Group.h"

/*******************************
Methods of class SketchSettings:
*******************************/

SketchSettings::SketchSettings(void)
	:color(255U,255U,255U),
	 lineWidth(1.0f),
	 versionNumber(0U),
	 detailSize(0),pickRadius(0),
	 gridEnabled(false),gridColor(0U,0U,0U),gridSize(0),
	 highlightColor(0U,0U,0U),
	 lingerSize(0),lingerTime(0.5),
	 highlightCycleLength(1),highlightCycle(0),
	 selectedObjects(17)
	{
	}

void SketchSettings::insertAfter(SketchObject* pred,SketchObject* newObject)
	{
	/* Call the base class method: */
	SketchObjectContainer::insertAfter(pred,newObject);
	
	/* Check if the predecessor is selected: */
	if(pred!=0&&selectedObjects.isEntry(pred))
		{
		/* Select the new object: */
		selectedObjects.setEntry(newObject);
		}
	}

void SketchSettings::remove(SketchObject* object)
	{
	/* De-select the object: */
	selectedObjects.removeEntry(object);
	
	/* Call the base class method: */
	SketchObjectContainer::remove(object);
	}

bool SketchSettings::setHighlightCycle(double applicationTime)
	{
	/* Calculate the new cycle value in [-1, 1]: */
	highlightCycle=Scalar(Math::sin(2.0*Math::Constants<double>::pi*applicationTime/highlightCycleLength)*0.5);
	
	/* Return true if there are selected objects: */
	return selectedObjects.getNumEntries()>0;
	}

void SketchSettings::setColor(const Color& newColor)
	{
	color=newColor;
	++versionNumber;
	}

void SketchSettings::setLineWidth(float newLineWidth)
	{
	lineWidth=newLineWidth;
	++versionNumber;
	}

void SketchSettings::setDetailSize(Scalar newDetailSize)
	{
	detailSize=newDetailSize;
	}

void SketchSettings::setPickRadius(Scalar newPickRadius)
	{
	pickRadius=newPickRadius;
	}

void SketchSettings::setGridEnabled(bool newGridEnabled)
	{
	gridEnabled=newGridEnabled;
	}

void SketchSettings::setGridColor(const Color& newGridColor)
	{
	gridColor=newGridColor;
	}

void SketchSettings::setGridSize(Scalar newGridSize)
	{
	gridSize=newGridSize;
	}

void SketchSettings::setHighlightColor(const Color& newHighlightColor)
	{
	highlightColor=newHighlightColor;
	}

void SketchSettings::setLingerSize(Scalar newLingerSize)
	{
	lingerSize=newLingerSize;
	}

void SketchSettings::setLingerTime(double newLingerTime)
	{
	lingerTime=newLingerTime;
	}

Point SketchSettings::snap(const Point& pos)
	{
	/* Pick all objects: */
	SketchObject::PickResult pickResult=SketchObjectContainer::pick(pos,pickRadius);
	if(pickResult.isValid())
		{
		/* Return the picked point: */
		return pickResult.pickedPoint;
		}
	else if(gridEnabled)
		{
		/* Snap individually in x and y: */
		Point result=pos;
		for(int i=0;i<2;++i)
			{
			/* Find the nearest grid line: */
			Scalar grid=Math::floor(pos[i]/gridSize+Scalar(0.5))*gridSize;
			
			/* Check if the grid line is close enough: */
			if(Math::abs(pos[i]-grid)<pickRadius)
				result[i]=grid;
			}
		return result;
		}
	else
		{
		/* Return the given position: */
		return pos;
		}
	}

SketchObject::PickResult SketchSettings::pickSelected(const Point& pos)
	{
	/* Create a pick result: */
	SketchObject::PickResult result(pos,pickRadius);
	
	/* Pick the selected objects: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		ssoIt->getSource()->pick(result);
	
	return result;
	}

void SketchSettings::select(const Point& pos)
	{
	/* Pick an object at the given position: */
	SketchObject::PickResult pickResult(pos,pickRadius);
	for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		soIt->pick(pickResult);
	
	/* Select a picked object: */
	if(pickResult.isValid())
		selectedObjects.setEntry(pickResult.pickedObject);
	}

void SketchSettings::select(const Box& box)
	{
	/* Check every object against the box: */
	for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		{
		const Box& obox=soIt->getBoundingBox();
		if(box.min[0]<=obox.min[0]&&box.max[0]>=obox.max[0]&&box.min[1]<=obox.min[1]&&box.max[1]>=obox.max[1])
			selectedObjects.setEntry(&*soIt);
		}
	}

void SketchSettings::selectNone(void)
	{
	/* Clear the selection set: */
	selectedObjects.clear();
	}

void SketchSettings::selectAll(void)
	{
	/* Add all sketch objects to the selection set: */
	for(SketchObjectList::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		selectedObjects.setEntry(&*soIt);
	}

void SketchSettings::cloneSelection(void)
	{
	/* Bail out if there are no selected objects: */
	if(selectedObjects.getNumEntries()==0)
		return;
	
	/* Clone the first selected object: */
	SketchObjectSet::Iterator ssoIt=selectedObjects.begin();
	SketchObject* firstClone=ssoIt->getSource()->clone();
	sketchObjects.push_back(firstClone);
	++ssoIt;
	while(!ssoIt.isFinished())
		{
		/* Clone the object: */
		sketchObjects.push_back(ssoIt->getSource()->clone());
		++ssoIt;
		}
	
	/* Clear the current selection and select all cloned objects: */
	selectedObjects.clear();
	for(SketchObjectList::iterator soIt=firstClone;soIt!=sketchObjects.end();++soIt)
		selectedObjects.setEntry(&*soIt);
	}

void SketchSettings::applySettingsToSelection(void)
	{
	/* Apply the current settings to all selected objects: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		ssoIt->getSource()->applySettings(*this);
	}

void SketchSettings::groupSelection(void)
	{
	/* Create a new group object and add all selected objects to it: */
	Group* newGroup=new Group;
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		{
		SketchObject* obj=sketchObjects.unlink(SketchObjectList::iterator(ssoIt->getSource()));
		newGroup->append(obj);
		}
	
	/* Add the new group to the list of sketch objects: */
	sketchObjects.push_back(newGroup);
	
	/* Select the new group: */
	selectedObjects.clear();
	selectedObjects.setEntry(newGroup);
	}

void SketchSettings::ungroupSelection(void)
	{
	/* Create a new selection list containing all selected non-group objects and the former members of group objects: */
	std::vector<SketchObject*> newSelectedObjects;
	
	/* Ungroup all selected group objects: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		{
		/* Check if the object is a group: */
		Group* group=dynamic_cast<Group*>(ssoIt->getSource());
		if(group!=0)
			{
			/* Remove the group from the object list: */
			sketchObjects.unlink(SketchObjectList::iterator(group));
			
			/* Add the group's former members to the new selection list: */
			for(SketchObjectList::iterator mIt=group->getSketchObjects().begin();mIt!=group->getSketchObjects().end();++mIt)
				newSelectedObjects.push_back(&*mIt);
			
			/* Transfer the group's members to the object list: */
			group->transferMembers(sketchObjects);
			
			/* Destroy the group object: */
			delete group;
			}
		else
			{
			/* Add the object to the new selection list: */
			newSelectedObjects.push_back(ssoIt->getSource());
			}
		}
	
	/* Select all objects from the new selection list: */
	selectedObjects.clear();
	for(std::vector<SketchObject*>::iterator nsoIt=newSelectedObjects.begin();nsoIt!=newSelectedObjects.end();++nsoIt)
		selectedObjects.setEntry(*nsoIt);
	}

void SketchSettings::selectionToBack(void)
	{
	/* Unlink all selected objects from the object list and re-insert them at the beginning of the list: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		{
		sketchObjects.unlink(SketchObjectList::iterator(ssoIt->getSource()));
		sketchObjects.insert(sketchObjects.begin(),ssoIt->getSource());
		}
	}

void SketchSettings::selectionToFront(void)
	{
	/* Unlink all selected objects from the object list and re-insert them at the end of the list: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		{
		sketchObjects.unlink(SketchObjectList::iterator(ssoIt->getSource()));
		sketchObjects.insert(sketchObjects.end(),ssoIt->getSource());
		}
	}

void SketchSettings::deleteSelection(void)
	{
	/* Delete all selected objects: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		sketchObjects.erase(SketchObjectList::iterator(ssoIt->getSource()));
	
	/* Clear the selection: */
	selectedObjects.clear();
	}

void SketchSettings::transformSelectedObjects(const Transformation& transform)
	{
	/* Apply the given transformation to all selected objects: */
	for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		ssoIt->getSource()->transform(transform);
	}

void SketchSettings::snapSelectedObjectsToGrid(void)
	{
	if(gridEnabled)
		{
		/* Snap all selected objects: */
		for(SketchObjectSet::Iterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
			ssoIt->getSource()->snapToGrid(gridSize);
		}
	}

void SketchSettings::drawSelectedObjects(const Transformation& transform,GLContextData& contextData) const
	{
	/* Draw all selected objects at their tentative new positions: */
	glPushMatrix();
	glMultMatrix(transform);
	
	for(SketchObjectSet::ConstIterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		{
		ssoIt->getSource()->setGLState(contextData);
		ssoIt->getSource()->glRenderAction(contextData);
		ssoIt->getSource()->resetGLState(contextData);
		}
	
	glPopMatrix();
	}

void SketchSettings::highlightSelectedObjects(const Transformation& transform,GLContextData& contextData) const
	{
	#if 1
	
	/* Highlight all selected sketch objects: */
	for(SketchObjectSet::ConstIterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		ssoIt->getSource()->glRenderActionHighlight(highlightCycle,contextData);
	
	#else
	
	if(selectedObjects.getNumEntries()>0)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		
		glPushMatrix();
		glMultMatrix(transform);
		
		glColor(highlightColor);
		for(SketchObjectSet::ConstIterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
			{
			const Box& box=ssoIt->getSource()->getBoundingBox();
			
			glBegin(GL_LINE_STRIP);
			glVertex(box.getVertex(0));
			glVertex(box.getVertex(1));
			glVertex(box.getVertex(3));
			glVertex(box.getVertex(2));
			glVertex(box.getVertex(0));
			glVertex(box.getVertex(4));
			glVertex(box.getVertex(5));
			glVertex(box.getVertex(7));
			glVertex(box.getVertex(6));
			glVertex(box.getVertex(4));
			glEnd();
			glBegin(GL_LINES);
			glVertex(box.getVertex(1));
			glVertex(box.getVertex(5));
			glVertex(box.getVertex(3));
			glVertex(box.getVertex(7));
			glVertex(box.getVertex(2));
			glVertex(box.getVertex(6));
			glEnd();
			}
		
		glPopMatrix();
		
		glPopAttrib();
		}
	
	#endif
	}

void SketchSettings::glRenderAction(const Box& viewBox,GLContextData& contextData) const
	{
	/* Render all sketch objects: */
	drawObjects(contextData);
	
	#if 1
	
	/* Highlight all selected sketch objects: */
	for(SketchObjectSet::ConstIterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
		{
		ssoIt->getSource()->setGLState(contextData);
		ssoIt->getSource()->glRenderActionHighlight(highlightCycle,contextData);
		ssoIt->getSource()->resetGLState(contextData);
		}
	
	#else
	
	/* Draw the bounding boxes of all selected sketch objects: */
	if(selectedObjects.getNumEntries()>0)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		
		glColor(highlightColor);
		for(SketchObjectSet::ConstIterator ssoIt=selectedObjects.begin();!ssoIt.isFinished();++ssoIt)
			{
			const Box& box=ssoIt->getSource()->getBoundingBox();
			
			glBegin(GL_LINE_STRIP);
			glVertex(box.getVertex(0));
			glVertex(box.getVertex(1));
			glVertex(box.getVertex(3));
			glVertex(box.getVertex(2));
			glVertex(box.getVertex(0));
			glVertex(box.getVertex(4));
			glVertex(box.getVertex(5));
			glVertex(box.getVertex(7));
			glVertex(box.getVertex(6));
			glVertex(box.getVertex(4));
			glEnd();
			glBegin(GL_LINES);
			glVertex(box.getVertex(1));
			glVertex(box.getVertex(5));
			glVertex(box.getVertex(3));
			glVertex(box.getVertex(7));
			glVertex(box.getVertex(2));
			glVertex(box.getVertex(6));
			glEnd();
			}
		
		glPopAttrib();
		}
	
	#endif
	}

void SketchSettings::renderGrid(const Box& viewBox,GLContextData& contextData) const
	{
	if(gridEnabled)
		{
		/* Render a drawing support grid: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		
		/* Calculate the indices of the first and last grid lines to draw: */
		int min[2],max[2];
		for(int i=0;i<2;++i)
			{
			min[i]=int(Math::ceil(viewBox.min[i]/gridSize));
			max[i]=int(Math::floor(viewBox.max[i]/gridSize));
			}
		
		glBegin(GL_LINES);
		glColor(gridColor);
		for(int x=min[0];x<=max[0];++x)
			{
			glVertex2d(x*gridSize,(min[1]-1)*gridSize);
			glVertex2d(x*gridSize,(max[1]+1)*gridSize);
			}
		for(int y=min[1];y<=max[1];++y)
			{
			glVertex2d((min[0]-1)*gridSize,y*gridSize);
			glVertex2d((max[0]+1)*gridSize,y*gridSize);
			}
		glEnd();
		
		glPopAttrib();
		}
	}
