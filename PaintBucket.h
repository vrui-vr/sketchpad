/***********************************************************************
PaintBucket - Custom GLMotif widget class to select and edit painting
colors.
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

#ifndef PAINTBUCKET_INCLUDED
#define PAINTBUCKET_INCLUDED

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GLMotif/Widget.h>

#include "SketchGeometry.h"

namespace GLMotif {

class PaintBucket:public Widget
	{
	/* Embedded classes: */
	public:
	class CallbackData:public Misc::CallbackData // Base class for paint bucket events
		{
		/* Elements: */
		public:
		PaintBucket* paintBucket; // Pointer to the paint bucket widget that caused the event
		
		/* Constructors and destructors: */
		CallbackData(PaintBucket* sPaintBucket)
			:paintBucket(sPaintBucket)
			{
			}
		};
	
	class SelectCallbackData:public CallbackData // Class for callback data sent when a paint bucket is selected
		{
		/* Constructors and destructors: */
		public:
		SelectCallbackData(PaintBucket* sPaintBucket)
			:CallbackData(sPaintBucket)
			{
			}
		};
	
	/* Elements: */
	private:
	GLfloat size; // Size of the paint bucket's interior
	Color color; // Color stored in the paint bucket
	bool isArmed; // Flag if the button is "armed," i.e., is touched by a selecting pointing device
	BorderType savedBorderType; // The paint bucket's border type right before the paint bucket was armed
	Misc::CallbackList selectCallbacks; // List of callbacks to be called when the paint bucket is selected
	
	/* Protected methods: */
	virtual void setArmed(bool newArmed); // Changes the "armed" state of the paint bucket
	virtual void select(void); // Is called when the paint bucket is selected
	
	/* Constructors and destructors: */
	public:
	PaintBucket(const char* sName,Container* sParent,GLfloat sSize,const Color& sColor,bool sManageChild =true);
	virtual ~PaintBucket(void);
	
	/* Methods from GLMotif::Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual ZRange calcZRange(void) const;
	virtual void setBorderType(Widget::BorderType newBorderType);
	virtual void draw(GLContextData& contextData) const;
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	
	/* New methods: */
	const Color& getColor(void) const // Returns the paint bucket's current color
		{
		return color;
		}
	void setColor(const Color& newColor); // Sets the paint bucket's current color
	Misc::CallbackList& getSelectCallbacks(void) // Returns the list of select callbacks
		{
		return selectCallbacks;
		}
	};

}

#endif
