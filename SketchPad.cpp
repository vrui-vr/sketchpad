/***********************************************************************
SketchPad - A simple sketching application, intended for but not limited
to multitouch screens with styluses.
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

#include "SketchPad.h"

#include <Misc/SizedTypes.h>
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Geometry/HVector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/LinearUnit.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLColorOperations.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/GenericAbstractToolFactory.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>

#include "Capsule.h"
#include "SketchObject.h"
#include "Curve.h"
#include "Image.h"
#include "PaintBucket.h"

/*****************************************
Methods of class SketchPad::SketchPadTool:
*****************************************/

SketchPad::SketchPadTool::SketchPadTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 firstLingerPos(true),lingering(false)
	{
	}

void SketchPad::SketchPadTool::buttonDown(const Point& pos)
	{
	/* Initialize linger detection: */
	lingerPos=pos;
	firstLingerPos=true;
	lingerEndTime=Vrui::getApplicationTime()+application->settings.getLingerTime();
	lastPos=pos;
	lastLingering=false;
	}

bool SketchPad::SketchPadTool::motion(const Point& pos)
	{
	/* Remember last frame's lingering state: */
	lastLingering=lingering;
	
	/* Check if the tool is lingering: */
	SketchSettings& settings=application->settings;
	if(Geometry::sqrDist(pos,lingerPos)>Math::sqr(settings.getLingerSize()))
		{
		/* Re-initialize linger detection: */
		lingerPos=pos;
		firstLingerPos=false;
		lingerEndTime=Vrui::getApplicationTime()+settings.getLingerTime();
		lingering=false;
		}
	else if(!lingering&&Vrui::getApplicationTime()>=lingerEndTime)
		{
		/* Start lingering: */
		lingering=true;
		
		/* Reset the linger detection neighborhood to the current tool position: */
		lingerPos=pos;
		}
	
	if(!lingering)
		{
		/* Schedule another Vrui frame for when the linger timeout expires: */
		Vrui::scheduleUpdate(lingerEndTime);
		}
	
	/* Check if the tool has moved: */
	bool result=pos!=lastPos;
	lastPos=pos;
	return result;
	}

void SketchPad::SketchPadTool::buttonUp(const Point& pos)
	{
	/* Stop lingering: */
	firstLingerPos=true;
	lingering=false;
	}

/**********************************************
Static elements of class SketchPad::SketchTool:
**********************************************/

SketchPad::SketchToolFactory* SketchPad::SketchTool::factory=0;

/**************************************
Methods of class SketchPad::SketchTool:
**************************************/

void SketchPad::SketchTool::initClass(Vrui::ToolFactory* baseClass)
	{
	/* Create a factory object for the custom tool class: */
	factory=new SketchToolFactory("SketchTool","Draw",baseClass,*Vrui::getToolManager());
	
	/* Set the sketching tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Draw");
	
	/* Register the sketching tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SketchPad::SketchTool::SketchTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:SketchPadTool(factory,inputAssignment),
	 sketchFactory(0),sketchFactoryVersion(0U),imageFactory(0),
	 active(false)
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
		
		/* Activate the tool: */
		active=true;
		}
	else
		{
		/* Stop dragging: */
		buttonUp(pos);
		
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
		active=false;
		}
	}

void SketchPad::SketchTool::frame(void)
	{
	if(active)
		{
		/* Transform the tool position to navigational coordinates: */
		Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		
		if(imageFactory!=0)
			{
			/* Map the environment's "up" direction into the sketching plane: */
			Vector up=Vrui::getInverseNavigationTransformation().transform(Vrui::getUpDirection());
			
			/* Calculate a rotation that aligns the "up" vector with the y axis: */
			imageFactory->setOrientation(Transformation::Rotation::rotateFromTo(Vector(0,1,0),up));
			}
		
		/* Continue dragging if the tool is moving and not currently lingering: */
		if((motion(pos)&&!isLingering())||(isLingering()&&!wasLingering()))
			{
			/* Deliver a motion event to the sketch object factory: */
			sketchFactory->motion(pos,isLingering(),!hasMoved());
			}
		
		/* Request another frame while active: */
		Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
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

/*********************************************
Static elements of class SketchPad::EraseTool:
*********************************************/

SketchPad::EraseToolFactory* SketchPad::EraseTool::factory=0;

/*************************************
Methods of class SketchPad::EraseTool:
*************************************/

void SketchPad::EraseTool::initClass(Vrui::ToolFactory* baseClass)
	{
	/* Create a factory object for the custom tool class: */
	factory=new EraseToolFactory("EraseTool","Erase",baseClass,*Vrui::getToolManager());
	
	/* Set the sketching tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Erase");
	
	/* Register the sketching tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SketchPad::EraseTool::EraseTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:SketchPadTool(factory,inputAssignment),
	 active(false)
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
		
		/* Activate the tool: */
		active=true;
		}
	else if(active)
		{
		/* Check if the tool hasn't moved since it was activated: */
		if(!hasMoved())
			{
			/* Erase the top-most object picked by the tool: */
			SketchObject* picked=application->settings.pickTop(lastPos);
			if(picked!=0)
				application->settings.remove(picked);
			}
		
		/* Stop dragging: */
		buttonUp(lastPos);
		
		/* Deactivate the tool: */
		active=false;
		}
	}

void SketchPad::EraseTool::frame(void)
	{
	if(active)
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
		#if 0
		else if(isLingering())
			{
			/* Erase the top-most object picked by the tool: */
			SketchObject* picked=application->settings.pickTop(pos);
			if(picked!=0)
				application->settings.remove(picked);
			
			/* Deactivate the tool: */
			buttonUp(pos);
			active=false;
			}
		#endif
		}
	}

void SketchPad::EraseTool::glRenderAction(GLContextData& contextData) const
	{
	if(active)
		{
		/* Draw the eraser capsule's outline: */
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

/**********************************************
Static elements of class SketchPad::SelectTool:
**********************************************/

SketchPad::SelectToolFactory* SketchPad::SelectTool::factory=0;

/**************************************
Methods of class SketchPad::SelectTool:
**************************************/

void SketchPad::SelectTool::initClass(Vrui::ToolFactory* baseClass)
	{
	/* Create a factory object for the custom tool class: */
	factory=new SelectToolFactory("SelectTool","Select",baseClass,*Vrui::getToolManager());
	
	/* Set the sketching tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Select");
	
	/* Register the sketching tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SketchPad::SelectTool::SelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:SketchPadTool(factory,inputAssignment),
	 box(Box::empty),
	 active(false)
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
		/* Check if the tool is picking a currently selected object: */
		dragging=application->settings.isSelectedPicked(pos);
		if(dragging)
			{
			/* Remember the initial dragging position: */
			initialDragPos=pos;
			}
		else
			{
			/* Activate the tool: */
			buttonDown(pos);
			corner0=pos;
			active=true;
			}
		}
	else
		{
		if(dragging)
			{
			/* Move the selected objects to their new position: */
			application->settings.transformSelectedObjects(Transformation::translate(pos-initialDragPos));
			
			/* Stop dragging: */
			dragging=false;
			}
		else
			{
			/* Check the selection mode: */
			application->settings.selectNone();
			if(hasMoved())
				{
				/* Update the selection box: */
				box=Box::empty;
				box.addPoint(corner0);
				box.addPoint(pos);
				
				/* Select all objects inside the current box: */
				application->settings.select(box);
				}
			else
				{
				/* Select all objects at the original position: */
				application->settings.select(corner0);
				}
			
			/* Deactivate the tool: */
			buttonUp(pos);
			active=false;
			}
		}
	}

void SketchPad::SelectTool::frame(void)
	{
	if(active)
		{
		Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		motion(pos);
		
		/* Update the selection box: */
		box=Box::empty;
		box.addPoint(corner0);
		box.addPoint(pos);
		}
	}

void SketchPad::SelectTool::glRenderAction(GLContextData& contextData) const
	{
	if(dragging)
		{
		/* Draw all selected objects at their tentative new positions: */
		Point pos=Point(Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0)));
		application->settings.drawSelectedObjects(Transformation::translate(pos-initialDragPos),contextData);
		}
	else if(active)
		{
		/* Draw the selection box: */
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

/**************************
Methods of class SketchPad:
**************************/

void SketchPad::loadSketchFile(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Open the selected file: */
		IO::FilePtr file=cbData->selectedDirectory->openFile(cbData->selectedFileName);
		file->setEndianness(Misc::LittleEndian);
		
		/* Read the number of sketch objects in the file: */
		size_t numSketchObjects=file->read<Misc::UInt32>();
		
		/* Read all sketch objects contained in the file: */
		SketchObjectList newSketchObjects;
		for(size_t i=0;i<numSketchObjects;++i)
			newSketchObjects.push_back(objectCreator.readObject(*file));
		
		/* Replace the current sketch object list with the newly-loaded one: */
		settings.getSketchObjects().clear();
		newSketchObjects.transfer(settings.getSketchObjects());
		
		/* Clear the selection: */
		settings.selectNone();
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError("Load Sketch File: Could not load file %s due to exception %s",cbData->getSelectedPath().c_str(),err.what());
		}
	}

void SketchPad::saveSketchFile(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Open the selected file: */
		IO::FilePtr file=cbData->selectedDirectory->openFile(cbData->selectedFileName,IO::File::WriteOnly);
		file->setEndianness(Misc::LittleEndian);
		
		/* Write the number of sketch objects: */
		const SketchObjectList& sos=settings.getSketchObjects();
		file->write<Misc::UInt32>(sos.size());
		
		/* Write all sketch objects: */
		for(SketchObjectList::const_iterator soIt=sos.begin();soIt!=sos.end();++soIt)
			objectCreator.writeObject(&*soIt,*file);
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError("Load Sketch File: Could not save file %s due to exception %s",cbData->getSelectedPath().c_str(),err.what());
		}
	}

void SketchPad::loadImage(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Open the selected file: */
		IO::FilePtr file=cbData->selectedDirectory->openFile(cbData->selectedFileName);
		
		/* Create a sketch object factory for the selected image file: */
		delete nextSketchFactory;
		nextSketchFactory=new ImageFactory(settings,cbData->getSelectedPath().c_str(),*file);
		
		/* Invalidate all current sketch object factories: */
		++sketchFactoryVersion;
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError("Load Image: Could not load image %s due to exception %s",cbData->getSelectedPath().c_str(),err.what());
		}
	}

GLMotif::PopupMenu* SketchPad::createFileMenu(void)
	{
	/* Create the submenu's top-level shell: */
	GLMotif::PopupMenu* fileMenuPopup=new GLMotif::PopupMenu("FileMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Button* loadSketchFileButton=new GLMotif::Button("LoadSketchFileButton",fileMenuPopup,"Load Sketch File...");
	sketchFileHelper.addLoadCallback(loadSketchFileButton,Misc::createFunctionCall(this,&SketchPad::loadSketchFile));
	
	GLMotif::Button* saveSketchFileButton=new GLMotif::Button("SaveSketchFileButton",fileMenuPopup,"Save Sketch File...");
	sketchFileHelper.addSaveCallback(saveSketchFileButton,Misc::createFunctionCall(this,&SketchPad::saveSketchFile));
	
	fileMenuPopup->addSeparator();
	
	GLMotif::Button* loadImageButton=new GLMotif::Button("LoadImageButton",fileMenuPopup,"Load Image...");
	imageHelper.addLoadCallback(loadImageButton,Misc::createFunctionCall(this,&SketchPad::loadImage));
	
	fileMenuPopup->manageMenu();
	return fileMenuPopup;
	}

void SketchPad::selectNoneSelected(Misc::CallbackData*)
	{
	/* Deselect all sketching objects: */
	settings.selectNone();
	}

void SketchPad::selectAllSelected(Misc::CallbackData*)
	{
	/* Select all sketching objects: */
	settings.selectAll();
	}

GLMotif::PopupMenu* SketchPad::createSelectMenu(void)
	{
	/* Create the submenu's top-level shell: */
	GLMotif::PopupMenu* selectMenuPopup=new GLMotif::PopupMenu("SelectMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Button* selectNoneButton=new GLMotif::Button("SelectNoneButton",selectMenuPopup,"Select None");
	selectNoneButton->getSelectCallbacks().add(this,&SketchPad::selectNoneSelected);
	
	GLMotif::Button* selectAllButton=new GLMotif::Button("SelectAllButton",selectMenuPopup,"Select All");
	selectAllButton->getSelectCallbacks().add(this,&SketchPad::selectAllSelected);
	
	selectMenuPopup->manageMenu();
	return selectMenuPopup;
	}

void SketchPad::cloneSelectionSelected(Misc::CallbackData*)
	{
	/* Clone all selected objects and make them the new selection: */
	settings.cloneSelection();
	}

void SketchPad::applySettingsSelected(Misc::CallbackData*)
	{
	/* Apply the current settings to all selected objects: */
	settings.applySettingsToSelection();
	}

void SketchPad::snapSelectionToGridSelected(Misc::CallbackData*)
	{
	/* Snap all selected objects to the drawing grid: */
	settings.snapSelectedObjectsToGrid();
	}

void SketchPad::groupSelectionSelected(Misc::CallbackData*)
	{
	/* Make a group out of all selected objects: */
	settings.groupSelection();
	}

void SketchPad::ungroupSelectionSelected(Misc::CallbackData*)
	{
	/* Break open all selected group objects: */
	settings.ungroupSelection();
	}

void SketchPad::selectionToBackSelected(Misc::CallbackData*)
	{
	settings.selectionToBack();
	}

void SketchPad::selectionToFrontSelected(Misc::CallbackData*)
	{
	settings.selectionToFront();
	}

void SketchPad::deleteSelectionSelected(Misc::CallbackData*)
	{
	/* Delete all selected objects: */
	settings.deleteSelection();
	}

GLMotif::PopupMenu* SketchPad::createEditMenu(void)
	{
	/* Create the submenu's top-level shell: */
	GLMotif::PopupMenu* editMenuPopup=new GLMotif::PopupMenu("EditMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Button* cloneSelectionButton=new GLMotif::Button("CloneSelectionButton",editMenuPopup,"Clone");
	cloneSelectionButton->getSelectCallbacks().add(this,&SketchPad::cloneSelectionSelected);
	
	GLMotif::Button* applySettingsButton=new GLMotif::Button("ApplySettingsButton",editMenuPopup,"Apply Settings");
	applySettingsButton->getSelectCallbacks().add(this,&SketchPad::applySettingsSelected);
	
	GLMotif::Button* snapSelectionToGridButton=new GLMotif::Button("SnapSelectionToGridButton",editMenuPopup,"Snap To Grid");
	snapSelectionToGridButton->getSelectCallbacks().add(this,&SketchPad::snapSelectionToGridSelected);
	
	editMenuPopup->addSeparator();
	
	GLMotif::Button* groupSelectionButton=new GLMotif::Button("GroupSelectionButton",editMenuPopup,"Group");
	groupSelectionButton->getSelectCallbacks().add(this,&SketchPad::groupSelectionSelected);
	
	GLMotif::Button* ungroupSelectionButton=new GLMotif::Button("UngroupSelectionButton",editMenuPopup,"Ungroup");
	ungroupSelectionButton->getSelectCallbacks().add(this,&SketchPad::ungroupSelectionSelected);
	
	editMenuPopup->addSeparator();
	
	GLMotif::Button* selectionToBackButton=new GLMotif::Button("SelectionToBackButton",editMenuPopup,"Send To Back");
	selectionToBackButton->getSelectCallbacks().add(this,&SketchPad::selectionToBackSelected);
	
	GLMotif::Button* selectionToFrontButton=new GLMotif::Button("SelectionToFrontButton",editMenuPopup,"Send To Front");
	selectionToFrontButton->getSelectCallbacks().add(this,&SketchPad::selectionToFrontSelected);
	
	editMenuPopup->addSeparator();
	
	GLMotif::Button* deleteSelectionButton=new GLMotif::Button("DeleteSelectionButton",editMenuPopup,"Delete");
	deleteSelectionButton->getSelectCallbacks().add(this,&SketchPad::deleteSelectionSelected);
	
	editMenuPopup->manageMenu();
	return editMenuPopup;
	}

void SketchPad::gridToggleValueChanged(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	settings.setGridEnabled(cbData->set);
	}

GLMotif::PopupMenu* SketchPad::createMainMenu(void)
	{
	/* Create the main menu shell: */
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("SketchPad");
	
	/* Create a cascade button to show the "File" submenu: */
	GLMotif::CascadeButton* fileCascade=new GLMotif::CascadeButton("FileCascade",mainMenuPopup,"File");
	fileCascade->setPopup(createFileMenu());
	
	/* Create a cascade button to show the "Select" submenu: */
	GLMotif::CascadeButton* selectCascade=new GLMotif::CascadeButton("SelectCascade",mainMenuPopup,"Select");
	selectCascade->setPopup(createSelectMenu());
	
	/* Create a cascade button to show the "Edit" submenu: */
	GLMotif::CascadeButton* editCascade=new GLMotif::CascadeButton("EditCascade",mainMenuPopup,"Edit");
	editCascade->setPopup(createEditMenu());
	
	/* Create a toggle button for the drawing grid: */
	GLMotif::ToggleButton* gridToggle=new GLMotif::ToggleButton("GridToggle",mainMenuPopup,"Show Grid");
	gridToggle->setToggle(settings.getGridEnabled());
	gridToggle->getValueChangedCallbacks().add(this,&SketchPad::gridToggleValueChanged);
	
	mainMenuPopup->manageMenu();
	return mainMenuPopup;
	}

void SketchPad::paletteColorSelectorValueChanged(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData)
	{
	/* Convert the new color and the current opacity value to a sketch object color: */
	GLMotif::Color newColor=cbData->newColor;
	newColor[3]=GLfloat(opacitySlider->getValue());
	Color newSketchColor(newColor);
	
	/* Set the color of the currently selected paint bucket and update the sketch settings: */
	selectedPaintBucket->setColor(newSketchColor);
	settings.setColor(newSketchColor);
	
	/* Apply the new sketch settings to all selected objects: */
	settings.applySettingsToSelection();
	}

void SketchPad::opacitySliderValueChanged(GLMotif::Slider::ValueChangedCallbackData* cbData)
	{
	/* Convert the current color and the new opacity value to a sketch object color: */
	GLMotif::Color newColor=paletteColorSelector->getCurrentColor();
	newColor[3]=GLfloat(cbData->value);
	Color newSketchColor(newColor);
	
	/* Set the color of the currently selected paint bucket and update the sketch settings: */
	selectedPaintBucket->setColor(newSketchColor);
	settings.setColor(newSketchColor);
	
	/* Apply the new sketch settings to all selected objects: */
	settings.applySettingsToSelection();
	}

void SketchPad::paintBucketSelected(GLMotif::PaintBucket::SelectCallbackData* cbData)
	{
	/* Pop out the previously selected paint bucket: */
	selectedPaintBucket->setBorderType(GLMotif::Widget::RAISED);
	
	/* Select the new paint bucket: */
	selectedPaintBucket=cbData->paintBucket;
	
	/* Pop in the newly selected paint bucket: */
	selectedPaintBucket->setBorderType(GLMotif::Widget::LOWERED);
	
	/* Update the color selector's current color and the sketch settings: */
	GLMotif::Color col(selectedPaintBucket->getColor());
	paletteColorSelector->setCurrentColor(col);
	opacitySlider->setValue(col[3]);
	settings.setColor(selectedPaintBucket->getColor());
	
	/* Apply the new sketch settings to all selected objects: */
	settings.applySettingsToSelection();
	}

void SketchPad::lineWidthSliderValueChanged(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Update the sketch settings' line width: */
	lineWidth=Scalar(cbData->value);
	
	/* Apply the new sketch settings to all selected objects: */
	settings.applySettingsToSelection();
	}

GLMotif::PopupWindow* SketchPad::createPaletteDialog(void)
	{
	GLMotif::PopupWindow* paletteDialogWindow=new GLMotif::PopupWindow("PaletteDialogWindow",Vrui::getWidgetManager(),"Color Palette");
	paletteDialogWindow->setHideButton(true);
	paletteDialogWindow->setResizableFlags(false,false);
	
	GLMotif::RowColumn* paletteDialog=new GLMotif::RowColumn("PaletteDialog",paletteDialogWindow,false);
	paletteDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	paletteDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	paletteDialog->setNumMinorWidgets(1);
	
	GLMotif::RowColumn* colorBox=new GLMotif::RowColumn("ColorBox",paletteDialog,false);
	colorBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	colorBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	colorBox->setNumMinorWidgets(1);
	
	/* Add a color selector: */
	paletteColorSelector=new GLMotif::HSVColorSelector("PaletteColorSelector",colorBox);
	paletteColorSelector->setPreferredSize(Vrui::getUiSize()*Vrui::Scalar(16));
	paletteColorSelector->setIndicatorSize(Vrui::getUiSize()*Vrui::Scalar(0.75));
	paletteColorSelector->getValueChangedCallbacks().add(this,&SketchPad::paletteColorSelectorValueChanged);
	
	/* Add an opacity slider: */
	opacitySlider=new GLMotif::Slider("OpacitySlider",colorBox,GLMotif::Slider::VERTICAL,Vrui::getUiStyleSheet()->fontHeight*5.0f);
	opacitySlider->setValueRange(0.0f,1.0f,0.0f);
	opacitySlider->setValue(1.0f);
	opacitySlider->getValueChangedCallbacks().add(this,&SketchPad::opacitySliderValueChanged);
	
	/* Add a set of color buckets: */
	GLMotif::RowColumn* paintBuckets=new GLMotif::RowColumn("paintBuckets",colorBox,false);
	paintBuckets->setOrientation(GLMotif::RowColumn::VERTICAL);
	paintBuckets->setPacking(GLMotif::RowColumn::PACK_GRID);
	paintBuckets->setNumMinorWidgets(4);
	
	static const Color defaultColors[]=
		{
		Color(0U,0U,0U),Color(255U,0U,0U),Color(128U,128U,0U),Color(0U,128U,0U),
		Color(0U,128U,128U),Color(0U,0U,255U),Color(255U,0U,255U),Color(255U,255U,255U),
		Color(51U,51U,51U),Color(102U,102U,102U),Color(153U,153U,153U),Color(204U,204U,204U),
		Color(255U,128U,0U),Color(64U,0U,0U),Color(128U,0U,255U),Color(128U,64U,64U)
		};
	GLMotif::PaintBucket* pbs[16];
	for(int i=0;i<16;++i)
		{
		char paintBucketName[14]="PaintBucket00";
		paintBucketName[11]='0'+i/10;
		paintBucketName[12]='0'+i%10;
		pbs[i]=new GLMotif::PaintBucket(paintBucketName,paintBuckets,Vrui::getUiSize()*2.0,defaultColors[i]);
		pbs[i]->getSelectCallbacks().add(this,&SketchPad::paintBucketSelected);
		}
	
	paintBuckets->manageChild();
	
	/* Select one of the paint buckets: */
	selectedPaintBucket=pbs[10];
	paletteColorSelector->setCurrentColor(GLMotif::Color(defaultColors[10]));
	settings.setColor(defaultColors[10]);
	
	colorBox->manageChild();
	
	/* Add a slider to change the line width: */
	GLMotif::RowColumn* lineWidthBox=new GLMotif::RowColumn("LineWidthBox",paletteDialog,false);
	lineWidthBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	lineWidthBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	lineWidthBox->setNumMinorWidgets(1);
	
	new GLMotif::Label("LineWidthLabel",lineWidthBox,"Line Width");
	
	GLMotif::TextFieldSlider* lineWidthSlider=new GLMotif::TextFieldSlider("LineWidthSlider",lineWidthBox,6,Vrui::getUiStyleSheet()->fontHeight*10.0f);
	lineWidthSlider->getTextField()->setPrecision(2);
	lineWidthSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	lineWidthSlider->getTextField()->setEditable(true);
	lineWidthSlider->getSlider()->addNotch(0.0f);
	lineWidthSlider->setSliderMapping(GLMotif::TextFieldSlider::EXP10);
	lineWidthSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	lineWidthSlider->setValueRange(0.25,25.0,0.0);
	lineWidthSlider->setValue(lineWidth);
	lineWidthSlider->getValueChangedCallbacks().add(this,&SketchPad::lineWidthSliderValueChanged);
	
	lineWidthBox->manageChild();
	
	paletteDialog->manageChild();
	
	return paletteDialogWindow;
	}

SketchPad::SketchPad(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 lineWidth(3.0f),
	 lingerRadius(Vrui::getUiSize()*Vrui::Scalar(0.5)),
	 sketchFactoryType(0),sketchFactoryVersion(0U),nextSketchFactory(0),
	 mainMenu(0),paletteDialog(0),
	 imageHelper(Vrui::getWidgetManager(),"Image.png",".ppm;.png;.jpg;.jpeg;.tif;.tiff"),
	 sketchFileHelper(Vrui::getWidgetManager(),"SketchFile.sketch",".sketch")
	{
	/* Parse the command line: */
	const char* sketchFileName=0;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			}
		else if(sketchFileName==0)
			sketchFileName=argv[i];
		}
	
	if(sketchFileName!=0)
		{
		/* Load the given sketch file: */
		try
			{
			/* Open the selected file: */
			IO::FilePtr file=IO::openFile(sketchFileName);
			file->setEndianness(Misc::LittleEndian);
			
			/* Read the number of sketch objects in the file: */
			size_t numSketchObjects=file->read<Misc::UInt32>();
			
			/* Read all sketch objects contained in the file: */
			for(size_t i=0;i<numSketchObjects;++i)
				settings.getSketchObjects().push_back(objectCreator.readObject(*file));
			}
		catch(const std::runtime_error& err)
			{
			Misc::formattedUserError("SketchPad: Unable to load sketch file %s due to exeption %s",sketchFileName,err.what());
			}
		}
	
	/* Initialize sketch settings: */
	settings.setGridEnabled(true);
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Create the palette dialog: */
	paletteDialog=createPaletteDialog();
	Vrui::popupPrimaryWidget(paletteDialog);
	
	/* Create an abstract base class for sketching-related tools: */
	typedef Vrui::GenericAbstractToolFactory<SketchPadTool> BaseToolFactory;
	BaseToolFactory* baseToolFactory=new BaseToolFactory("SketchPadTool","SketchPad",0,*Vrui::getToolManager());
	Vrui::getToolManager()->addAbstractClass(baseToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Create the sketching tool classes: */
	SketchTool::initClass(baseToolFactory);
	EraseTool::initClass(baseToolFactory);
	SelectTool::initClass(baseToolFactory);
	
	/* Set the navigation-space unit to inches: */
	Vrui::getCoordinateManager()->setUnit(Geometry::LinearUnit(Geometry::LinearUnit::INCH,1));
	}

SketchPad::~SketchPad(void)
	{
	delete mainMenu;
	delete paletteDialog;
	}

void SketchPad::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Call the base class method: */
	Vrui::Application::toolCreationCallback(cbData);
	
	/* Check if the new tool is a sketching tool: */
	SketchPadTool* sketchPadTool=dynamic_cast<SketchPadTool*>(cbData->tool);
	if(sketchPadTool!=0)
		{
		/* Add the new tool to the tool list: */
		sketchPadTools.push_back(sketchPadTool);
		}
	}

void SketchPad::toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
	{
	/* Check if the destroyed tool is a sketching tool: */
	SketchPadTool* sketchPadTool=dynamic_cast<SketchPadTool*>(cbData->tool);
	if(sketchPadTool!=0)
		{
		/* Remove the destroyed tool from the tool list: */
		for(std::vector<SketchPadTool*>::iterator sptIt=sketchPadTools.begin();sptIt!=sketchPadTools.end();++sptIt)
			if(*sptIt==sketchPadTool)
				{
				sketchPadTools.erase(sptIt);
				break;
				}
		}
	
	/* Call the base class method: */
	Vrui::Application::toolDestructionCallback(cbData);
	}

void SketchPad::frame(void)
	{
	/* Retrieve the current navigation transformation's scaling factor: */
	Scalar navScaling(Vrui::getNavigationTransformation().getScaling());
	
	/* Set the line width to lineWidth points in physical space: */
	settings.setLineWidth(lineWidth/(navScaling*Scalar(72)));
	
	/* Adjust the sketching detail size based on the current navigation transformation scale factor: */
	settings.setDetailSize(Scalar(0.1)*lineWidth/(navScaling*Scalar(72)));
	
	/* Adjust the pick distance: */
	settings.setPickRadius(Vrui::getPointPickDistance());
	
	if(settings.getGridEnabled())
		{
		/* Calculate an appropriate size for a drawing support grid, starting at 1/4" at 1:1 scale: */
		Vrui::Scalar targetPhysGridSize=Vrui::getInchFactor()*Vrui::Scalar(0.25);
		Vrui::Scalar gridSpacing(0.25);
		
		Vrui::Scalar physGridSizeMin=targetPhysGridSize/Math::sqrt(Vrui::Scalar(2));
		while(gridSpacing*navScaling<physGridSizeMin)
			gridSpacing*=Vrui::Scalar(2);
		Vrui::Scalar physGridSizeMax=targetPhysGridSize*Math::sqrt(Vrui::Scalar(2));
		while(gridSpacing*navScaling>physGridSizeMax)
			gridSpacing/=Vrui::Scalar(2);
		settings.setGridSize(gridSpacing);
		
		/* Set the grid color: */
		Color gridColor(Vrui::getBackgroundColor());
		for(int i=0;i<3;++i)
			gridColor[i]=255U-gridColor[i];
		gridColor[3]=48U;
		settings.setGridColor(gridColor);
		settings.setHighlightColor(Vrui::getForegroundColor());
		}
	
	/* Adjust the linger detection size based on the current navigation transformation scale factor: */
	settings.setLingerSize(lingerRadius/navScaling);
	
	/* Set the current highlight cycle value: */
	if(settings.setHighlightCycle(Vrui::getApplicationTime()))
		Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
	}

void SketchPad::display(GLContextData& contextData) const
	{
	/*********************************************************************
	Calculate the intersection of the view frustum with the drawing plane:
	*********************************************************************/
	
	/* Calculate the corner points of the frustum in navigational coordinates: */
	const Vrui::DisplayState& ds=Vrui::getDisplayState(contextData);
	Vrui::Point fvNavs[8];
	for(int i=0;i<8;++i)
		{
		/* Convert the frustum vertex from clip coordinates to navigational coordinates: */
		Vrui::PTransform::HVector fvClip(i&0x1?1:-1,i&0x2?1:-1,i&0x4?1:-1,1);
		fvNavs[i]=ds.modelviewNavigational.inverseTransform(ds.projection.inverseTransform(fvClip).toPoint());
		}
	
	/* Intersect the frustum's edges with the (x, y) plane: */
	static const int edges[12*2]=
		{
		0,1,2,3,4,5,6,7, // Edges along x
		0,2,1,3,4,6,5,7, // Edges along y
		0,4,1,5,2,6,3,7 // Edges along z
		};
	Box viewBox=Box::empty;
	for(int edge=0;edge<12;++edge)
		{
		/* Get the edge's vertices: */
		Vrui::Point& v0=fvNavs[edges[edge*2+0]];
		Vrui::Point& v1=fvNavs[edges[edge*2+1]];
		
		/* Check if the edge intersects the (x, y) plane: */
		if((v0[2]<=Vrui::Scalar(0)&&v1[2]>Vrui::Scalar(0))||(v0[2]>Vrui::Scalar(0)&&v1[2]<=Vrui::Scalar(0)))
			{
			/* Intersect the edge with the (x, y) plane and add the intersection point to the view box: */
			Vrui::Scalar w1=(Vrui::Scalar(0)-v0[2])/(v1[2]-v0[2]);
			Vrui::Scalar w0=Vrui::Scalar(1)-w1;
			viewBox.addPoint(Point(v0[0]*w0+v1[0]*w1,v0[1]*w0+v1[1]*w1,0));
			}
		}
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	
	/* Enable alpha blending: */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	
	/* Draw the sketching environment: */
	settings.glRenderAction(viewBox,contextData);
	
	/* Draw the current state of all sketch tools: */
	for(std::vector<SketchPadTool*>::const_iterator sptIt=sketchPadTools.begin();sptIt!=sketchPadTools.end();++sptIt)
		(*sptIt)->glRenderAction(contextData);
	
	/* Draw the rendering grid: */
	settings.renderGrid(viewBox,contextData);
	
	/* Return to standard OpenGL state: */
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	
	#if 0
	
	/* Render all bounding boxes (debugging): */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	glColor(Vrui::getForegroundColor());
	for(SketchObjectList::const_iterator soIt=settings.getSketchObjects().begin();soIt!=settings.getSketchObjects().end();++soIt)
		{
		const Box& box=soIt->getBoundingBox();
		
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
	
	#endif
	
	#if 0
	
	/* Draw the bounding boxes of all selected sketch objects: */
	settings.highlightSelectedObjects(Transformation::identity,contextData);
	
	#endif
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void SketchPad::resetNavigation(void)
	{
	/* Center on the origin, scale to 1:1 scale, align (x, y) plane with environment: */
	Vrui::NavTransform nav=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	
	Vrui::Vector y=Vrui::getUpDirection();
	Vrui::Vector x=Vrui::getForwardDirection()^y;
	nav*=Vrui::NavTransform::rotate(Vrui::Rotation::fromBaseVectors(x,y));
	
	nav*=Vrui::NavTransform::scale(Vrui::getInchFactor());
	
	Vrui::setNavigationTransformation(nav);
	}

SketchObjectFactory* SketchPad::getSketchFactory(void)
	{
	if(nextSketchFactory!=0)
		{
		/* Return the pre-set sketch factory: */
		SketchObjectFactory* result=nextSketchFactory;
		nextSketchFactory=0;
		return result;
		}
	else
		{
		/* Return a new curve factory: */
		return new CurveFactory(settings);
		}
	}

VRUI_APPLICATION_RUN(SketchPad)
