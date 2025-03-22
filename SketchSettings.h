/***********************************************************************
SketchSettings - Class to represent a set of parameters to create sketch
objects.
Copyright (c) 2016-2023 Oliver Kreylos
***********************************************************************/

#ifndef SKETCHSETTINGS_INCLUDED
#define SKETCHSETTINGS_INCLUDED

#include <Misc/HashTable.h>

#include "SketchGeometry.h"
#include "SketchObject.h"
#include "SketchObjectList.h"
#include "SketchObjectContainer.h"

/* Forward declarations: */
class Group;

class SketchSettings:public SketchObjectContainer
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<SketchObject*,void> SketchObjectSet; // Type for hash tables to represent sets of sketch objects
	
	/* Elements: */
	private:
	
	/* Versioned settings: */
	Color color; // Current color
	float lineWidth; // Current cosmetic or geometric line width
	unsigned int versionNumber; // Version number of current settings
	
	/* Asynchronous settings: */
	Scalar detailSize; // Detail size for sketch objects in navigational coordinates
	Scalar pickRadius; // Radius for pick or snap requests
	bool gridEnabled; // Flag whether the drawing grid is enabled
	Color gridColor; // Color to draw the drawing grid
	Scalar gridSize; // Cell size of the drawing grid
	Color highlightColor; // Color to draw highlights
	Scalar lingerSize; // Size of the linger detection neighborhood
	double lingerTime; // Time threshold to detect lingering tools
	double highlightCycleLength; // Length of a highlight cycle in seconds
	Scalar highlightCycle; // Current cycle value to highlight selected objects
	
	SketchObjectSet selectedObjects; // Set of currently selected sketch objects
	
	/* Constructors and destructors: */
	public:
	SketchSettings(void); // Creates a default set of sketch settings
	
	/* Methods from SketchObjectContainer: */
	virtual void insertAfter(SketchObject* pred,SketchObject* newObject);
	virtual void remove(SketchObject* object);
	virtual SketchObject::SnapResult snap(const Point& center,Scalar radius2) const;
	
	/* Methods: */
	const Color& getColor(void) const // Returns the current color
		{
		return color;
		}
	float getLineWidth(void) const // Returns the current line width
		{
		return lineWidth;
		}
	unsigned int getVersionNumber(void) const // Returns the current settings version number
		{
		return versionNumber;
		}
	Scalar getDetailSize(void) const // Returns the current detail size
		{
		return detailSize;
		}
	Scalar getPickRadius(void) const // Returns the current pick radius
		{
		return pickRadius;
		}
	bool getGridEnabled(void) const // Returns true if the drawing grid is enabled
		{
		return gridEnabled;
		}
	const Color& getGridColor(void) const // Returns the color to draw the drawing grid
		{
		return gridColor;
		}
	Scalar getGridSize(void) const // Returns the cell size of the drawing grid
		{
		return gridSize;
		}
	const Color& getHighlightColor(void) const // Returns the color to draw highlights
		{
		return highlightColor;
		}
	Scalar getLingerSize(void) const // Returns the current linger size
		{
		return lingerSize;
		}
	double getLingerTime(void) const // Returns the current lingering time threshold
		{
		return lingerTime;
		}
	bool setHighlightCycle(double applicationTime); // Sets the highlight cycle value based on the given application time; returns true if there are objects that need highlighting
	void setColor(const Color& newColor); // Sets the current color
	void setLineWidth(float newLineWidth); // Sets the current line width
	void setDetailSize(Scalar newDetailSize); // Sets the current detail size
	void setPickRadius(Scalar newPickRadius); // Sets the current pick radius
	void setGridEnabled(bool newEnableGrid); // Enables or disables the drawing grid
	void setGridColor(const Color& newGridColor); // Sets the drawing grid's drawing color
	void setGridSize(Scalar newGridSize); // Sets the drawing grid's cell size
	void setHighlightColor(const Color& newHighlightColor); // Sets the highlight color
	void setLingerSize(Scalar newLingerSize); // Sets the current linger detection neighborhood size
	void setLingerTime(double newLingerTime); // Sets the lingering detection time threshold
	SketchObject* pickTop(const Point& pos) // Shortcut for pickTop method
		{
		return SketchObjectContainer::pickTop(pos,Math::sqr(pickRadius));
		}
	bool isSelected(SketchObject* object) const // Returns true if the given sketch object is currently selected
		{
		return selectedObjects.isEntry(object);
		}
	void select(SketchObject* object) // Adds the given sketch object to the selection
		{
		selectedObjects.setEntry(object);
		}
	void select(const Point& pos); // Selects all objects picked by a tool at the given position
	void select(const Box& box); // Selects all objects that lie entirely within the given box
	void unselect(SketchObject* object) // Removes the given sketch object from the selection
		{
		selectedObjects.removeEntry(object);
		}
	void selectNone(void); // Clears the selection
	void selectAll(void); // Selects all sketch objects
	void cloneSelection(void); // Clones all currently selected objects and selects them
	void applySettingsToSelection(void); // Applies current settings to selected objects
	void groupSelection(void); // Joins all selected objects into a group
	void ungroupSelection(void); // Breaks apart all groups in the current selection
	void selectionToBack(void); // Sends selected objects to the back of the sketch environment
	void selectionToFront(void); // Sends selected objects to the front of the sketch environment
	void deleteSelection(void); // Deletes all selected objects
	bool isSelectedPicked(const Point& pos) const; // Returns true if a selected object is picked by a tool at the given position
	void transformSelectedObjects(const Transformation& transform); // Transforms all selected objects by the given transformation
	void snapSelectedObjectsToGrid(void); // Snaps all selected objects to the drawing grid
	void drawSelectedObjects(const Transformation& transform,GLContextData& contextData) const; // Draws selected objects with the given transformation
	void highlightSelectedObjects(const Transformation& transform,GLContextData& contextData) const; // Highlights selected objects with the given transformation
	void glRenderAction(const Box& viewBox,GLContextData& contextData) const; // Draws the sketching environment inside the given view bounding box
	void renderGrid(const Box& viewBox,GLContextData& contextData) const; // Renders a drawing grid
	};

#endif
