/***********************************************************************
EraseTool - Class for erasing tools.
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

#include "EraseTool.h"

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include "RenderState.h"

/*********************************************
Static elements of class SketchPad::EraseTool:
*********************************************/

SketchPad::EraseTool::Factory* SketchPad::EraseTool::factory=0;

/*************************************
Methods of class SketchPad::EraseTool:
*************************************/

void SketchPad::EraseTool::initClass(Vrui::ToolFactory* baseClass)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("EraseTool","Erase",baseClass,*Vrui::getToolManager());
	
	/* Set the sketching tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Erase");
	
	/* Register the sketching tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SketchPad::EraseTool::EraseTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:SketchPadTool(factory,inputAssignment)
	{
	}

SketchPad::EraseTool::~EraseTool(void)
	{
	}

const Vrui::ToolFactory* SketchPad::EraseTool::getFactory(void) const
	{
	return factory;
	}

void SketchPad::EraseTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Start dragging: */
		lastPos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		buttonDown(lastPos);
		
		/* Initialize the eraser capsule: */
		eraser=Capsule(lastPos,lastPos,Scalar(Vrui::getPointPickDistance())*Scalar(2));
		}
	else
		{
		/* Check if the tool hasn't moved since it was activated: */
		if(!hasMoved())
			{
			/* Erase the object picked by the tool: */
			SketchObject::PickResult pickResult=application->settings.pick(lastPos);
			if(pickResult.isValid())
				application->settings.remove(pickResult.pickedObject);
			}
		
		/* Stop dragging: */
		buttonUp(lastPos);
		}
	}

void SketchPad::EraseTool::frame(void)
	{
	if(isActive())
		{
		/* Transform the tool position to navigational coordinates: */
		Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		
		/* Continue dragging: */
		motion(pos);
		
		/* Check if the tool has moved in this dragging sequence, or if it lingered at the initial position: */
		if(hasMoved())
			{
			/* Update the eraser capsule: */
			eraser=Capsule(lastPos,pos,Scalar(Vrui::getPointPickDistance())*Scalar(2));
			
			/* Rub out all sketch objects parts inside the eraser capsule: */
			SketchObjectList::iterator soIt=application->settings.getSketchObjects().begin();
			while(soIt!=application->settings.getSketchObjects().end())
				{
				/* Get an iterator to the next object: */
				SketchObjectList::iterator nextIt=soIt;
				++nextIt;
				
				/* Erase from the object if its bounding box intersects the capsule: */
				if(eraser.doesIntersect(soIt->getBoundingBox()))
					soIt->rubout(eraser,application->settings);
				
				/* Go to the next object, even if the current one was deleted or split: */
				soIt=nextIt;
				}
			
			lastPos=pos;
			}
		}
	}

void SketchPad::EraseTool::glRenderAction(RenderState& renderState) const
	{
	if(isActive())
		{
		/* Draw the eraser capsule's outline: */
		renderState.setRenderer(0);
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		
		if(eraser.getAxisLen2()!=Scalar(0))
			{
			/* Draw the eraser as two semicircles connected by two straight lines: */
			const Point& c0=eraser.getC0();
			const Point& c1=eraser.getC1();
			Scalar r=eraser.getRadius();
			Scalar semiAngle=Math::atan2(c1[1]-c0[1],c1[0]-c0[0])+Math::Constants<Scalar>::pi*Scalar(0.5);
			glBegin(GL_LINE_LOOP);
			glColor(Vrui::getForegroundColor());
			for(int i=0;i<=16;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(32)+semiAngle;
				glVertex(c0[0]+Math::cos(angle)*r,c0[1]+Math::sin(angle)*r,Scalar(0));
				}
			for(int i=16;i<=32;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(32)+semiAngle;
				glVertex(c1[0]+Math::cos(angle)*r,c1[1]+Math::sin(angle)*r,Scalar(0));
				}
			glEnd();
			}
		else
			{
			/* Draw the eraser as a circle: */
			const Point& c0=eraser.getC0();
			Scalar r=eraser.getRadius();
			glBegin(GL_LINE_LOOP);
			glColor(Vrui::getForegroundColor());
			for(int i=0;i<32;++i)
				{
				Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
				glVertex(c0[0]+Math::cos(angle)*r,c0[1]+Math::sin(angle)*r,Vrui::Scalar(0));
				}
			glEnd();
			}
		
		glPopAttrib();
		}
	}
