/***********************************************************************
SketchObjectCreator - Class keeping track of sketch object classes.
Copyright (c) 2016 Oliver Kreylos
***********************************************************************/

#include "SketchObjectCreator.h"

#include <Misc/SizedTypes.h>
#include <IO/File.h>

#include "Curve.h"
#include "Group.h"
#include "Image.h"

/************************************
Methods of class SketchObjectCreator:
************************************/

SketchObjectCreator::SketchObjectCreator(void)
	{
	/* Assign type codes to all sketch object classes: */
	Curve::typeCode=0;
	Group::typeCode=1;
	Image::typeCode=2;
	
	/* Initialize sketch object classes that need initialization: */
	Image::initClass();
	}

SketchObjectCreator::~SketchObjectCreator(void)
	{
	/* Deinitialize sketch object classes that need deinitialization: */
	Image::deinitClass();
	}

SketchObject* SketchObjectCreator::createObject(unsigned int typeCode)
	{
	switch(typeCode)
		{
		case 0:
			return new Curve;
			break;
		
		case 1:
			return new Group;
			break;
		
		case 2:
			return new Image;
			break;
		
		default:
			return 0;
		}
	}

SketchObject* SketchObjectCreator::readObject(IO::File& file)
	{
	SketchObject* result=0;
	
	/* Read the object's type code: */
	unsigned int typeCode=file.read<Misc::UInt16>();
	
	/* Create the object and read its state: */
	switch(typeCode)
		{
		case 0:
			result=new Curve;
			result->read(file,*this);
			break;
		
		case 1:
			result=new Group;
			result->read(file,*this);
			break;
		
		case 2:
			result=new Image;
			result->read(file,*this);
			break;
		}
	
	return result;
	}

void SketchObjectCreator::writeObject(const SketchObject* object,IO::File& file) const
	{
	/* Write the object's type code: */
	file.write<Misc::UInt16>(object->getTypeCode());
	
	/* Write the object's state: */
	object->write(file,*this);
	}
