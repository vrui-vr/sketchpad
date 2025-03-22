/***********************************************************************
SketchObjectCreator - Class keeping track of sketch object classes.
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
