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

#include "Image.h"

#include <Misc/SizedTypes.h>
#include <Misc/StandardMarshallers.h>
#include <Geometry/GeometryMarshallers.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/Extensions/GLARBTextureRectangle.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Images/ReadImageFile.h>

#include "SketchSettings.h"

/******************************
Static elements of class Image:
******************************/

unsigned int Image::typeCode=0;
Images::TextureSet* Image::textureSet=0;

/**********************
Methods of class Image:
**********************/

void Image::initClass(void)
	{
	/* Create the singleton texture set: */
	textureSet=new Images::TextureSet;
	}

void Image::deinitClass(void)
	{
	/* Destroy the singleton texture set: */
	delete textureSet;
	textureSet=0;
	}

Image::Image(void)
	:imageKey(~0x0U)
	{
	imageFile.ref();
	}

Image::~Image(void)
	{
	/* Destroy the image texture: */
	textureSet->deleteTexture(imageKey);
	}

unsigned int Image::getTypeCode(void) const
	{
	return typeCode;
	}

bool Image::pick(const Point& center,Scalar radius2) const
	{
	/* Check if the transformed sphere touches the image's rectangle: */
	Point imgCenter=imageTransform.inverseTransform(center);
	const Images::BaseImage& image=textureSet->getTexture(imageKey).getImage();
	Scalar dist2=Scalar(0);
	for(int i=0;i<2;++i)
		{
		if(imgCenter[i]<Scalar(0))
			dist2+=Math::sqr(Scalar(0)-imgCenter[i]);
		else if(imgCenter[i]>Scalar(image.getSize(i)))
			dist2+=Math::sqr(imgCenter[i]-Scalar(image.getSize(i)));
		}
	return dist2*Math::sqr(imageTransform.getScaling())<=radius2;
	}

SketchObject::SnapResult Image::snap(const Point& center,Scalar radius2) const
	{
	/* Images don't snap; return an invalid snap result: */
	SnapResult result;
	result.valid=false;
	return result;
	}

SketchObject* Image::clone(void) const
	{
	/* Create a new Image object: */
	Image* result=new Image;
	
	/* Copy the image's source image file: */
	result->imageFileName=imageFileName;
	imageFile.writeToSink(result->imageFile);
	
	/* Copy this image's bounding box and transformation: */
	result->boundingBox=boundingBox;
	result->imageTransform=imageTransform;
	
	/* Clone this image's image data: */
	const Images::TextureSet::Texture& texture=textureSet->getTexture(imageKey);
	Images::TextureSet::Texture& resultTexture=textureSet->addTexture(texture.getImage(),texture.getTarget(),texture.getInternalFormat());
	resultTexture.setWrapModes(texture.getWrapModes()[0],texture.getWrapModes()[1]);
	resultTexture.setFilterModes(texture.getFilterModes()[0],texture.getFilterModes()[1]);
	result->imageKey=resultTexture.getKey();
	
	return result;
	}

void Image::applySettings(const SketchSettings& settings)
	{
	/* Doesn't do anything */
	}

void Image::transform(const Transformation& transform)
	{
	/* Pre-multiply the current image transformation and update the bounding box: */
	imageTransform.leftMultiply(transform);
	imageTransform.renormalize();
	const Images::BaseImage& image=textureSet->getTexture(imageKey).getImage();
	boundingBox=Box::empty;
	boundingBox.addPoint(imageTransform.transform(Point(0,0,0)));
	boundingBox.addPoint(imageTransform.transform(Point(image.getWidth(),0,0)));
	boundingBox.addPoint(imageTransform.transform(Point(image.getWidth(),image.getHeight(),0)));
	boundingBox.addPoint(imageTransform.transform(Point(0,image.getHeight(),0)));
	}

void Image::snapToGrid(Scalar gridSize)
	{
	/* Doesn't do anything */
	}

void Image::rubout(const Capsule& eraser,SketchObjectContainer& container)
	{
	/* Doesn't do anything */
	}

void Image::write(IO::File& file,const SketchObjectCreator& creator) const
	{
	/* Write the image file name: */
	Misc::Marshaller<std::string>::write(imageFileName,file);
	
	/* Write the source image file: */
	size_t imageFileSize=imageFile.getDataSize();
	file.write<Misc::UInt32>(imageFileSize);
	imageFile.writeToSink(file);
	
	/* Write the image transformation: */
	Misc::Marshaller<Transformation>::write(imageTransform,file);
	}

void Image::read(IO::File& file,SketchObjectCreator& creator)
	{
	/* Read the image file name: */
	imageFileName=Misc::Marshaller<std::string>::read(file);
	
	/* Read the source image file: */
	size_t imageFileSize=file.read<Misc::UInt32>();
	while(imageFileSize>0)
		{
		void* readBuffer;
		size_t readSize=file.readInBuffer(readBuffer,imageFileSize);
		imageFile.writeRaw(readBuffer,readSize);
		imageFileSize-=readSize;
		}
	imageFile.flush();
	
	/* Load the image from the memory buffer: */
	Images::BaseImage image=Images::readGenericImageFile(imageFile,Images::getImageFileFormat(imageFileName.c_str()));
	
	/* Add the image to the texture set: */
	imageKey=Image::textureSet->addTexture(image,GL_TEXTURE_RECTANGLE_ARB,image.getInternalFormat()).getKey();
	Image::textureSet->getTexture(imageKey).setFilterModes(GL_LINEAR,GL_LINEAR);
	
	/* Read the image transformation: */
	imageTransform=Misc::Marshaller<Transformation>::read(file);
	
	/* Calculate the image bounding box: */
	boundingBox=Box::empty;
	boundingBox.addPoint(imageTransform.transform(Point(0,0,0)));
	boundingBox.addPoint(imageTransform.transform(Point(image.getWidth(),0,0)));
	boundingBox.addPoint(imageTransform.transform(Point(image.getWidth(),image.getHeight(),0)));
	boundingBox.addPoint(imageTransform.transform(Point(0,image.getHeight(),0)));
	}

void Image::setGLState(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	}

void Image::glRenderAction(GLContextData& contextData) const
	{
	/* Retrieve the texture set's OpenGL state: */
	Images::TextureSet::GLState* glState=textureSet->getGLState(contextData);
	
	/* Bind the image texture and enable its texture target: */
	const Images::TextureSet::GLState::Texture& texture=glState->bindTexture(imageKey);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Draw the image: */
	glPushMatrix();
	glMultMatrix(imageTransform);
	
	const Images::BaseImage& image=texture.getImage();
	const GLfloat* texMin=texture.getTexCoordMin();
	const GLfloat* texMax=texture.getTexCoordMax();
	glBegin(GL_QUADS);
	glTexCoord2f(texMin[0],texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(texMax[0],texMin[1]);
	glVertex2i(image.getSize(0),0);
	glTexCoord2f(texMax[0],texMax[1]);
	glVertex2i(image.getSize(0),image.getSize(1));
	glTexCoord2f(texMin[0],texMax[1]);
	glVertex2i(0,image.getSize(1));
	glEnd();
	
	glPopMatrix();
	}

void Image::glRenderActionHighlight(Scalar cycle,GLContextData& contextData) const
	{
	/* Calculate a highlight color: */
	GLfloat highlight[4];
	for(int i=0;i<3;++i)
		highlight[i]=cycle>=Scalar(0)?1.0f:0.0f;
	highlight[3]=float(Math::abs(cycle));
	
	/* Retrieve the texture set's OpenGL state: */
	Images::TextureSet::GLState* glState=textureSet->getGLState(contextData);
	
	/* Bind the image texture and enable its texture target: */
	const Images::TextureSet::GLState::Texture& texture=glState->bindTexture(imageKey);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB,GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB,GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_ALPHA,GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,highlight);
	
	/* Draw the image: */
	glPushMatrix();
	glMultMatrix(imageTransform);
	
	const Images::BaseImage& image=texture.getImage();
	const GLfloat* texMin=texture.getTexCoordMin();
	const GLfloat* texMax=texture.getTexCoordMax();
	glBegin(GL_QUADS);
	glTexCoord2f(texMin[0],texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(texMax[0],texMin[1]);
	glVertex2i(image.getSize(0),0);
	glTexCoord2f(texMax[0],texMax[1]);
	glVertex2i(image.getSize(0),image.getSize(1));
	glTexCoord2f(texMin[0],texMax[1]);
	glVertex2i(0,image.getSize(1));
	glEnd();
	
	glPopMatrix();
	}

void Image::resetGLState(GLContextData& contextData) const
	{
	/* Reset OpenGL state: */
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
	glPopAttrib();
	}

/*****************************
Methods of class ImageFactory:
*****************************/

ImageFactory::ImageFactory(const SketchSettings& sSettings,const char* imageFileName,IO::File& imageFile)
	:SketchObjectFactory(sSettings),
	 next(new Image),
	 current(0),
	 orientation(Transformation::Rotation::identity)
	{
	/* Read the (compressed) image file into the next image's memory buffer: */
	next->imageFileName=imageFileName;
	void* readBuffer;
	size_t readSize;
	while((readSize=imageFile.readInBuffer(readBuffer))!=0)
		next->imageFile.writeRaw(readBuffer,readSize);
	next->imageFile.flush();
	
	/* Load the image from the memory buffer: */
	Images::BaseImage nextImage=Images::readGenericImageFile(next->imageFile,Images::getImageFileFormat(next->imageFileName.c_str()));
	
	/* Add the image to the texture set: */
	next->imageKey=Image::textureSet->addTexture(nextImage,GL_TEXTURE_RECTANGLE_ARB,GL_RGB8).getKey();
	Image::textureSet->getTexture(next->imageKey).setFilterModes(GL_LINEAR,GL_LINEAR);
	
	/* Get the image size: */
	for(int i=0;i<2;++i)
		size[i]=Scalar(nextImage.getSize(i));
	}

ImageFactory::~ImageFactory(void)
	{
	if(next!=0)
		delete next;
	if(current!=0)
		delete current;
	}

void ImageFactory::buttonDown(const Point& pos)
	{
	/* Start drawing a rectangle: */
	current=next;
	next=0;
	current->imageTransform=Transformation(pos-Point::origin,orientation,Scalar(0));
	p0=pos;
	p1=pos;
	}

void ImageFactory::motion(const Point& pos,bool lingering,bool firstNeighborhood)
	{
	/* Update the drawing rectangle: */
	p1=pos;
	
	/* Update the image transformation: */
	Point ip0=orientation.inverseTransform(p0);
	Point ip1=orientation.inverseTransform(p1);
	Scalar min[2],max[2],bSize[2];
	for(int i=0;i<2;++i)
		{
		if(ip0[i]<=ip1[i])
			{
			min[i]=ip0[i];
			max[i]=ip1[i];
			}
		else
			{
			min[i]=ip1[i];
			max[i]=ip0[i];
			}
		bSize[i]=max[i]-min[i];
		}
	
	/* Check the current box's aspect ratio against the image's: */
	Point iOrigin(min[0],min[1],Scalar(0));
	Scalar scale=Scalar(0);
	if(size[0]*bSize[1]<=size[1]*bSize[0])
		{
		/* Box is too wide: */
		scale=bSize[1]/size[1];
		iOrigin[0]+=Math::div2(bSize[0]-size[0]*scale);
		}
	else
		{
		/* Box is too tall: */
		scale=bSize[0]/size[0];
		iOrigin[1]+=Math::div2(bSize[1]-size[1]*scale);
		}
	
	/* Create the image transformation: */
	current->imageTransform=Transformation::translateFromOriginTo(orientation.transform(iOrigin));
	current->imageTransform*=Transformation::rotate(orientation);
	current->imageTransform*=Transformation::scale(scale);
	
	/* Update the image bounding box: */
	current->boundingBox=Box::empty;
	current->boundingBox.addPoint(current->imageTransform.transform(Point(0,0,0)));
	current->boundingBox.addPoint(current->imageTransform.transform(Point(size[0],0,0)));
	current->boundingBox.addPoint(current->imageTransform.transform(Point(size[0],size[1],0)));
	current->boundingBox.addPoint(current->imageTransform.transform(Point(0,size[1],0)));
	}

bool ImageFactory::buttonUp(const Point& pos)
	{
	/* Notify the caller that the object is finished: */
	return true;
	}

SketchObject* ImageFactory::finish(void)
	{
	/* Return the current image object: */
	SketchObject* result=current;
	current=0;
	return result;
	}

void ImageFactory::glRenderAction(GLContextData& contextData) const
	{
	if(current!=0)
		{
		/* Draw the current image at its current size and position: */
		current->setGLState(contextData);
		current->glRenderAction(contextData);
		current->resetGLState(contextData);
		
		glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
		glPointSize(3.0f);
		glBegin(GL_POINTS);
		glColor(settings.getHighlightColor());
		glVertex(p0);
		glVertex(p1);
		glEnd();
		
		glPopAttrib();
		}
	}

void ImageFactory::setOrientation(const Transformation::Rotation& newOrientation)
	{
	/* Update the image orientation: */
	orientation=newOrientation;
	}
