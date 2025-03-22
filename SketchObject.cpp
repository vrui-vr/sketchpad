/***********************************************************************
SketchObject - Base class for sketching objects for a simple sketching
application.
Copyright (c) 2016 Oliver Kreylos
***********************************************************************/

#include "SketchObject.h"

/*****************************
Methods of class SketchObject:
*****************************/

SketchObject::~SketchObject(void)
	{
	}

void SketchObject::setGLState(GLContextData& contextData) const
	{
	}

void SketchObject::resetGLState(GLContextData& contextData) const
	{
	}

/************************************
Methods of class SketchObjectFactory:
************************************/

SketchObjectFactory::~SketchObjectFactory(void)
	{
	}
