/***********************************************************************
SketchTool - Class for sketching tools.
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

#include "SketchTool.h"

#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include "SketchObject.h"
#include "Image.h"

/**********************************************
Static elements of class SketchPad::SketchTool:
**********************************************/

SketchPad::SketchTool::Factory* SketchPad::SketchTool::factory=0;

/**************************************
Methods of class SketchPad::SketchTool:
**************************************/

void SketchPad::SketchTool::initClass(Vrui::ToolFactory* baseClass)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("SketchTool","Draw",baseClass,*Vrui::getToolManager());
	
	/* Set the sketching tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Draw");
	
	/* Register the sketching tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SketchPad::SketchTool::SketchTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:SketchPadTool(factory,inputAssignment),
	 sketchFactory(0),sketchFactoryVersion(0U),imageFactory(0)
	{
	}

SketchPad::SketchTool::~SketchTool(void)
	{
	if(sketchFactory!=0)
		{
		/* Finish any sketch objects still being created: */
		SketchObject* current=sketchFactory->finish();
		if(current!=0)
			application->settings.getSketchObjects().push_back(current);
		
		/* Delete the sketch object factory: */
		delete sketchFactory;
		}
	}

const Vrui::ToolFactory* SketchPad::SketchTool::getFactory(void) const
	{
	return factory;
	}

void SketchPad::SketchTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Transform the tool position to navigational coordinates: */
	Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
	
	if(cbData->newButtonState)
		{
		/* Start dragging: */
		buttonDown(pos);
		
		/* Check if the current sketch object factory is outdated: */
		if(sketchFactory!=0&&sketchFactoryVersion!=application->sketchFactoryVersion)
			{
			/* Finish any sketch objects still being created: */
			SketchObject* current=sketchFactory->finish();
			if(current!=0)
				application->settings.getSketchObjects().push_back(current);
			
			/* Delete the sketch object factory: */
			delete sketchFactory;
			sketchFactory=0;
			}
		
		/* Check if there is no sketch object factory: */
		if(sketchFactory==0)
			{
			/* Get a new sketch object factory: */
			sketchFactory=application->getSketchFactory();
			sketchFactoryVersion=application->sketchFactoryVersion;
			
			/* Check if the new factory is an image factory: */
			imageFactory=dynamic_cast<ImageFactory*>(sketchFactory);
			}
		
		/* Deliver a button down event to the sketch factory: */
		sketchFactory->buttonDown(pos);
		}
	else
		{
		/* Deliver a button up event to the sketch factory and check if the current object is finished: */
		if(sketchFactory->buttonUp(pos))
			{
			/* Finalize the current sketch object and append it to the application's list: */
			application->settings.getSketchObjects().push_back(sketchFactory->finish());
			
			/* Delete the current sketch factory if it is an image factory: */
			if(imageFactory!=0)
				{
				delete sketchFactory;
				sketchFactory=0;
				imageFactory=0;
				}
			}
		
		/* Deactivate the tool: */
		buttonUp(pos);
		}
	}

void SketchPad::SketchTool::frame(void)
	{
	if(isActive())
		{
		/* Transform the tool position to navigational coordinates: */
		Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		
		/* Continue dragging: */
		bool moved=motion(pos);
		
		if(imageFactory!=0)
			{
			/* Map the environment's "up" direction into the sketching plane: */
			Vector up=Vrui::getInverseNavigationTransformation().transform(Vrui::getUpDirection());
			
			/* Calculate a rotation that aligns the "up" vector with the y axis: */
			imageFactory->setOrientation(Transformation::Rotation::rotateFromTo(Vector(0,1,0),up));
			}
		
		/* Continue dragging if the tool is moving and not currently lingering: */
		if((moved&&!isLingering())||(isLingering()&&!wasLingering()))
			{
			/* Deliver a motion event to the sketch object factory: */
			sketchFactory->motion(pos,isLingering(),!hasMoved());
			}
		
		#if 0
		/* Request another frame while active: */
		Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
		#endif
		}
	}

void SketchPad::SketchTool::glRenderAction(GLContextData& contextData) const
	{
	if(sketchFactory!=0)
		{
		/* Draw the sketch object factory's state: */
		sketchFactory->glRenderAction(contextData);
		}
	}
