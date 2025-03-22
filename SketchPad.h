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

#ifndef SKETCHPAD_INCLUDED
#define SKETCHPAD_INCLUDED

#include <vector>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/Slider.h>
#include <GLMotif/TextFieldSlider.h>
#include <GLMotif/HSVColorSelector.h>
#include <GLMotif/FileSelectionHelper.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/Application.h>

#include "SketchGeometry.h"
#include "Capsule.h"
#include "SketchSettings.h"
#include "SketchObjectList.h"
#include "SketchObjectCreator.h"
#include "PaintBucket.h"

/* Forward declarations: */
namespace GLMotif {
class PopupMenu;
class PopupWindow;
}
class SketchObject;
class SketchObjectFactory;
class ImageFactory;

class SketchPad:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	class SketchPadTool:public Vrui::Tool,public Vrui::Application::Tool<SketchPad> // Base class for tools belonging to the SketchPad application
		{
		/* Elements: */
		private:
		Point lingerPos; // Neighborhood position for linger detection
		bool firstLingerPos; // Flag if the linger position is the initial one when a button down event occurred
		double lingerEndTime; // Application time at which the tool will enter lingering state if it does not leave the current neighborhood
		bool lingering; // Flag whether the tool is currently lingering
		Point lastPos; // Last tool position
		bool lastLingering; // Flag whether the tool was lingering on the previous frame
		
		/* Constructors and destructors: */
		public:
		SketchPadTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* New methods: */
		bool hasMoved(void) const // Returns true if the tool has moved since the button down event
			{
			return !firstLingerPos;
			}
		bool isLingering(void) const // Returns true if the tool is currently lingering
			{
			return lingering;
			}
		bool wasLingering(void) const // Returns true of the tool was lingering on the previous frame
			{
			return lastLingering;
			}
		void buttonDown(const Point& pos); // Processes a button-down event at the given position and the current time
		bool motion(const Point& pos); // Processes a motion event to the given position while the button is pressed at the current time; returns true if the tool actually moved
		void buttonUp(const Point& pos); // Processes a button-up event at the given position and the current time
		virtual void glRenderAction(GLContextData& contextData) const =0; // Renders sketch object-related tool state at the end of the application's display method
		};
	
	class SketchTool;
	typedef Vrui::GenericToolFactory<SketchTool> SketchToolFactory; // Factory class for sketching tools
	
	class SketchTool:public SketchPadTool
		{
		friend class Vrui::GenericToolFactory<SketchTool>;
		
		/* Elements: */
		private:
		static SketchToolFactory* factory; // Pointer to the class' factory object
		SketchObjectFactory* sketchFactory; // Pointer to a factory for sketch objects
		unsigned int sketchFactoryVersion; // Version number of sketch factory object
		ImageFactory* imageFactory; // Pointer to sketch factory if current is an image factory
		bool active; // Flag if the tool's button is currently pressed
		
		/* Constructors and destructors: */
		public:
		static void initClass(Vrui::ToolFactory* baseClass); // Initializes the tool class
		SketchTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		virtual ~SketchTool(void);
		
		/* Methods from Vrui::Tool: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		
		/* Methods from SketchPadTool: */
		virtual void glRenderAction(GLContextData& contextData) const;
		};
	
	class EraseTool;
	typedef Vrui::GenericToolFactory<EraseTool> EraseToolFactory; // Factory class for erasing tools
	
	class EraseTool:public SketchPadTool
		{
		friend class Vrui::GenericToolFactory<EraseTool>;
		
		/* Elements: */
		private:
		static EraseToolFactory* factory; // Pointer to the class' factory object
		bool active; // Flag whether the tool is active
		Point lastPos; // Eraser position on the last frame
		Capsule eraser; // The current eraser capsule
		
		/* Constructors and destructors: */
		public:
		static void initClass(Vrui::ToolFactory* baseClass); // Initializes the tool class
		EraseTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		virtual ~EraseTool(void);
		
		/* Methods from Vrui::Tool: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		
		/* Methods from SketchPadTool: */
		virtual void glRenderAction(GLContextData& contextData) const;
		};
	
	class SelectTool;
	typedef Vrui::GenericToolFactory<SelectTool> SelectToolFactory; // Factory class for selecting tools
	
	class SelectTool:public SketchPadTool
		{
		friend class Vrui::GenericToolFactory<SelectTool>;
		
		/* Elements: */
		private:
		static SelectToolFactory* factory; // Pointer to the class' factory object
		bool dragging; // Flag whether the tool is dragging the current selection
		Point initialDragPos; // Initial dragging position
		Box box; // The current selection box
		Point corner0; // Initial selection box corner
		bool active; // Flag whether the tool is active
		
		/* Constructors and destructors: */
		public:
		static void initClass(Vrui::ToolFactory* baseClass); // Initializes the tool class
		SelectTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		virtual ~SelectTool(void);
		
		/* Methods from Vrui::Tool: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		
		/* Methods from SketchPadTool: */
		virtual void glRenderAction(GLContextData& contextData) const;
		};
	
	/* Elements: */
	private:
	Scalar lineWidth; // Current line width in printer's points in physical space
	Scalar lingerRadius; // Radius of the tool linger detection neighborhood in physical coordinate units
	SketchObjectCreator objectCreator; // Object to manage sketch object classes
	SketchSettings settings; // Settings to create new sketch objects
	int sketchFactoryType; // Code for the current sketch object factory type
	unsigned int sketchFactoryVersion; // Version number of the current sketch object factory
	SketchObjectFactory* nextSketchFactory; // If not null, sketch object factory to return on next call to getSketchFactory
	GLMotif::PopupMenu* mainMenu; // The application's main menu
	GLMotif::PopupWindow* paletteDialog; // Dialog window to select colors from a palette
	GLMotif::HSVColorSelector* paletteColorSelector; // Widget to select colors using the HSV model
	GLMotif::Slider* opacitySlider; // Slider to select a paint bucket's color opacity
	GLMotif::PaintBucket* selectedPaintBucket; // Pointer to the currently selected paint bucket
	GLMotif::FileSelectionHelper imageHelper; // Helper object to load images
	GLMotif::FileSelectionHelper sketchFileHelper; // Helper object to load/save sketch files
	std::vector<SketchPadTool*> sketchPadTools; // List of existing sketching tools
	
	/* Private methods: */
	void loadSketchFile(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback when user selected a sketch file to load
	void saveSketchFile(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback when user selected a sketch file to save
	void loadImage(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback when user selected an image to load
	GLMotif::PopupMenu* createFileMenu(void); // Creates the "File" submenu
	void selectNoneSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Select None" menu entry is selected
	void selectAllSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Select All" menu entry is selected
	GLMotif::PopupMenu* createSelectMenu(void); // Creates the "Select" submenu
	void cloneSelectionSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Clone" menu entry is selected
	void applySettingsSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Apply Settings" menu entry is selected
	void snapSelectionToGridSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Snap To Grid" menu entry is selected
	void groupSelectionSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Group" menu entry is selected
	void ungroupSelectionSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Ungroup" menu entry is selected
	void selectionToBackSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Send to Back" menu entry is selected
	void selectionToFrontSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Send to Front" menu entry is selected
	void deleteSelectionSelected(Misc::CallbackData* cbData); // Callback called when the "Selection"->"Delete" menu entry is selected
	GLMotif::PopupMenu* createEditMenu(void); // Creates the "Edit" submenu
	void gridToggleValueChanged(GLMotif::ToggleButton::ValueChangedCallbackData* cbData); // Callback called when the "Show Grid" toggle button changes value
	GLMotif::PopupMenu* createMainMenu(void); // Creates the application's main menu
	void paletteColorSelectorValueChanged(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData); // Callback called when the color selector's color changes
	void opacitySliderValueChanged(GLMotif::Slider::ValueChangedCallbackData* cbData); // Callback called when the opacity slider's value changes
	void paintBucketSelected(GLMotif::PaintBucket::SelectCallbackData* cbData); // Callback called when a paint bucket is selected
	void lineWidthSliderValueChanged(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the line width slider changes value
	GLMotif::PopupWindow* createPaletteDialog(void); // Creates the palette window
	
	/* Constructors and destructors: */
	public:
	SketchPad(int& argc,char**& argv);
	virtual ~SketchPad(void);
	
	/* Methods from Vrui::Application: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* New methods: */
	SketchObjectFactory* getSketchFactory(void); // Returns a new sketch object factory using current settings
	};

#endif
