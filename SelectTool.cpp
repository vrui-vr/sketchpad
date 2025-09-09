/***********************************************************************
SelectTool - Class for selecting tools.
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

#include "SelectTool.h"

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include "RenderState.h"

/**********************************************
Static elements of class SketchPad::SelectTool:
**********************************************/

SketchPad::SelectTool::Factory* SketchPad::SelectTool::factory=0;

/**************************************
Methods of class SketchPad::SelectTool:
**************************************/

void SketchPad::SelectTool::initClass(Vrui::ToolFactory* baseClass)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("SelectTool","Select",baseClass,*Vrui::getToolManager());
	
	/* Set the sketching tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Select");
	
	/* Register the sketching tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SketchPad::SelectTool::SelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:SketchPadTool(factory,inputAssignment),
	 draggedObject(0),box(Box::empty)
	{
	}

SketchPad::SelectTool::~SelectTool(void)
	{
	}

const Vrui::ToolFactory* SketchPad::SelectTool::getFactory(void) const
	{
	return factory;
	}

void SketchPad::SelectTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Transform the tool position to navigational coordinates: */
	Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
	
	if(cbData->newButtonState)
		{
		/* Activate the tool: */
		buttonDown(pos);
		
		/* Check if the tool is picking a currently selected object: */
		SketchObject::PickResult pickResult=application->settings.pickSelected(pos);
		if(pickResult.isValid())
			{
			/* Remember the dragged object and the initial dragging offset: */
			draggedObject=pickResult.pickedObject;
			pickOffset=pickResult.pickedPoint-pos;
			dragTrans=Vector::zero;
			}
		}
	else
		{
		if(draggedObject!=0)
			{
			if(hasMoved())
				{
				/* Move the selected objects to their new position: */
				application->settings.transformSelectedObjects(Transformation::translate(dragTrans));
				}
			else
				{
				/* Deselect the originally picked object: */
				application->settings.unselect(draggedObject);
				}
			
			/* Stop dragging: */
			draggedObject=0;
			}
		else
			{
			/* Check the selection mode: */
			application->settings.selectNone();
			if(hasMoved())
				{
				/* Select all objects inside the current selection box: */
				application->settings.select(box);
				}
			else
				{
				/* Select all objects at the original position: */
				application->settings.select(getFirstPos());
				}
			}
		
		/* Deactivate the tool: */
		buttonUp(pos);
		}
	}

void SketchPad::SelectTool::frame(void)
	{
	if(isActive())
		{
		/* Transform the tool position to navigational coordinates: */
		Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		
		/* Continue dragging: */
		motion(pos);
		
		if(draggedObject!=0)
			{
			/* Check if the tool is lingering: */
			if(isLingering())
				{
				/* Snap the initially picked position: */
				Point snappedPos=application->settings.snap(pos+pickOffset);
				dragTrans=(snappedPos-getFirstPos())-pickOffset;
				}
			else
				{
				/* Update the dragging translation: */
				dragTrans=pos-getFirstPos();
				}
			}
		else
			{
			/* Update the selection box: */
			box=Box::empty;
			box.addPoint(getFirstPos());
			box.addPoint(pos);
			}
		}
	}

void SketchPad::SelectTool::glRenderAction(RenderState& renderState) const
	{
	if(isActive())
		{
		if(draggedObject!=0)
			{
			/* Draw all selected objects at their tentative new positions: */
			application->settings.drawSelectedObjects(Transformation::translate(dragTrans),renderState);
			}
		else 
			{
			/* Draw the selection box: */
			renderState.setRenderer(0);
			glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
			glDisable(GL_LIGHTING);
			glLineWidth(1.0f);
			
			glColor(Vrui::getForegroundColor());
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
			
			glPopAttrib();
			}
		}
	}
