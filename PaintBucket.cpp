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

#include "PaintBucket.h"

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>

namespace GLMotif {

/****************************
Methods of class PaintBucket:
****************************/

void PaintBucket::setArmed(bool newArmed)
	{
	if(newArmed&&!isArmed)
		{
		/* Arm the paint bucket: */
		savedBorderType=getBorderType();
		Widget::setBorderType(savedBorderType!=Widget::LOWERED?Widget::LOWERED:Widget::RAISED); // Need to use base class method here
		isArmed=true;
		}
	else if(!newArmed&&isArmed)
		{
		/* Disarm the paint bucket: */
		Widget::setBorderType(savedBorderType); // Need to use base class method here
		isArmed=false;
		}
	}

void PaintBucket::select(void)
	{
	/* Call the select callbacks: */
	SelectCallbackData cbData(this);
	selectCallbacks.call(&cbData);
	}

PaintBucket::PaintBucket(const char* sName,Container* sParent,GLfloat sSize,const Color& sColor,bool sManageChild)
	:Widget(sName,sParent,false),
	 size(sSize),color(sColor),
	 isArmed(false)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Button defaults to raised border: */
	setBorderType(Widget::RAISED);
	setBorderWidth(ss->buttonBorderWidth);
	
	if(sManageChild)
		manageChild();
	}

PaintBucket::~PaintBucket(void)
	{
	}

Vector PaintBucket::calcNaturalSize(void) const
	{
	Vector result(size,size,0);
	return calcExteriorSize(result);
	}

ZRange PaintBucket::calcZRange(void) const
	{
	/* Calculate the parent class widget's z range: */
	ZRange myZRange=Widget::calcZRange();
	
	/* Adjust for the popping in/out when arming/disarming: */
	myZRange+=ZRange(getExterior().origin[2]-getBorderWidth(),getExterior().origin[2]+getBorderWidth());
	
	return myZRange;
	}

void PaintBucket::setBorderType(Widget::BorderType newBorderType)
	{
	if(isArmed)
		{
		/* Store the new border type to be set when the paint bucket is unarmed: */
		savedBorderType=newBorderType;
		}
	else
		{
		/* Set the new border type using the base class method: */
		Widget::setBorderType(newBorderType);
		}
	}

void PaintBucket::draw(GLContextData& contextData) const
	{
	/* Call the base class method: */
	GLMotif::Widget::draw(contextData);
	
	/* Draw the widget's interior with the current color, without illumination: */
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	
	glBegin(GL_QUADS);
	glColor(color);
	glVertex(getInterior().getCorner(0));
	glVertex(getInterior().getCorner(1));
	glVertex(getInterior().getCorner(3));
	glVertex(getInterior().getCorner(2));
	glEnd();
	
	glPopAttrib();
	}

void PaintBucket::pointerButtonDown(Event&)
	{
	/* Arm the paint bucket: */
	setArmed(true);
	}

void PaintBucket::pointerButtonUp(Event& event)
	{
	/* Select if the event is for us: */
	if(event.getTargetWidget()==this)
		select();
	
	/* Disarm the paint bucket: */
	setArmed(false);
	}

void PaintBucket::pointerMotion(Event& event)
	{
	if(!event.isPressed())
		return;
	
	/* Check if the new pointer position is still inside the paint bucket: */
	if(event.getTargetWidget()==this)
		{
		/* Arm the paint bucket: */
		setArmed(true);
		}
	else
		{
		/* Disarm the paint bucket: */
		setArmed(false);
		}
	}

void PaintBucket::setColor(const Color& newColor)
	{
	color=newColor;
	}

}
