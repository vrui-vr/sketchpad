/***********************************************************************
Image - Class for color images.
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

#ifndef IMAGE_INCLUDED
#define IMAGE_INCLUDED

#include <string>
#include <IO/VariableMemoryFile.h>
#include <Images/TextureSet.h>

#include "SketchGeometry.h"
#include "SketchObject.h"

class Image:public SketchObject
	{
	friend class SketchObjectCreator;
	friend class ImageFactory;
	
	/* Elements: */
	static unsigned int typeCode; // The image class's type code
	static Images::TextureSet* textureSet; // Singleton texture set managing all images in a sketch environment
	std::string imageFileName; // Original name of the image file
	IO::VariableMemoryFile imageFile; // In-memory file containing the (compressed) image data
	Images::TextureSet::Key imageKey; // Key to retrieve image's image data
	Transformation imageTransform; // Transformation from image's pixel space into sketch environment
	
	/* Constructors and destructors: */
	static void initClass(void); // Initializes the Image object class
	static void deinitClass(void); // De-initializes the Image object class
	Image(void); // Creates an empty image
	virtual ~Image(void); // Destroys the image
	
	/* Methods from SketchObject: */
	public:
	virtual unsigned int getTypeCode(void) const;
	virtual bool pick(PickResult& result) const;
	virtual bool pick(const Point& center,Scalar radius2) const;
	virtual SnapResult snap(const Point& center,Scalar radius2) const;	
	virtual SketchObject* clone(void) const;
	virtual void applySettings(const SketchSettings& settings);
	virtual void transform(const Transformation& transform);
	virtual void snapToGrid(Scalar gridSize);
	virtual void rubout(const Capsule& eraser,SketchObjectContainer& container);
	virtual void write(IO::File& file,const SketchObjectCreator& creator) const;
	virtual void read(IO::File& file,SketchObjectCreator& creator);
	virtual void setGLState(GLContextData& contextData) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual void glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const;
	virtual void resetGLState(GLContextData& contextData) const;
	};

class ImageFactory:public SketchObjectFactory
	{
	/* Elements: */
	private:
	Image* next; // Pointer to the next image to be created
	Scalar size[2]; // Image size in pixels
	Image* current; // Pointer to currently created image object, or null
	Point p0,p1; // Diagonally opposite corners of the new image's bounding box
	Transformation::Rotation orientation; // Orientation for the new image
	
	/* Constructors and destructors: */
	public:
	ImageFactory(const SketchSettings& sSettings,const char* imageFileName,IO::File& imageFile);
	virtual ~ImageFactory(void);
	
	/* Methods from SketchObjectFactory: */
	virtual void buttonDown(const Point& pos);
	virtual void motion(const Point& pos,bool lingering,bool firstNeighborhood);
	virtual bool buttonUp(const Point& pos);
	virtual SketchObject* finish(void);
	virtual void glRenderAction(GLContextData& contextData) const;
	
	/* New methods: */
	void setOrientation(const Transformation::Rotation& newOrientation); // Updates the base vectors to align the created image; assumed to be orthonormal
	};

#endif
