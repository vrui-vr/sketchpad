/***********************************************************************
PaintBucket - Custom GLMotif widget class to select and edit painting
colors.
Copyright (c) 2016 Oliver Kreylos
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
	 size(sSize),color(sColor)
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
