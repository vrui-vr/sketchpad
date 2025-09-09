/***********************************************************************
PolylineRenderer - Class to render polylines with geometric line width
and high-quality anti-aliasing.
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

#include "PolylineRenderer.h"

#include <stddef.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContext.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBGeometryShader4.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRScreen.h>
#include <Vrui/VRWindow.h>
#include <Vrui/DisplayState.h>

#include "Config.h"

/************************************************
Declaration of struct PolylineRenderer::DataItem:
************************************************/

struct PolylineRenderer::DataItem:public GLObject::DataItem
	{
	/* Embedded classes: */
	public:
	struct Vertex // Structure representing a polyline vertex in GPU memory
		{
		/* Elements: */
		public:
		Vector normal; // Halfway vector between adjacent polyline segments, uploaded to shader as normal vector
		Point position; // Vertex position
		};
	
	struct MemoryBlock // Structure representing a memory block in GPU memory
		{
		/* Embedded classes: */
		public:
		struct FreeChunk // Structure representing an unused chunk of memory inside a memory block
			{
			/* Elements: */
			public:
			size_t offset,size; // Offset and size of free chunk, in units of vertices
			
			/* Constructors and destructors: */
			FreeChunk(size_t sOffset,size_t sSize)
				:offset(sOffset),size(sSize)
				{
				}
			};
		
		/* Elements: */
		public:
		GLuint bufferId; // ID of the OpenGL buffer backing this memory block
		std::vector<FreeChunk> freeChunks; // List of unused chunks of memory, sorted by increasing offset
		
		/* Constructors and destructors: */
		MemoryBlock(size_t numVertices); // Creates a memory block holding the given number of polyline vertices
		private:
		MemoryBlock(const MemoryBlock& source);
		MemoryBlock& operator=(const MemoryBlock& source);
		public:
		~MemoryBlock(void);
		
		/* Methods: */
		void release(size_t offset,size_t size); // Releases the allocated memory chunk
		};
	
	struct CacheItem // Structure representing a cached polyline
		{
		/* Elements: */
		public:
		MemoryBlock* memoryBlock; // Pointer to memory block containing the polyline's vertex data
		size_t offset,size; // Offset and size of memory chunk allocated to the polyline, in units of vertices
		
		/* Constructors and destructors: */
		CacheItem(MemoryBlock* sMemoryBlock,size_t sOffset,size_t sSize)
			:memoryBlock(sMemoryBlock),offset(sOffset),size(sSize)
			{
			}
		};
	
	typedef Misc::HashTable<const void*,CacheItem> CacheMap; // Type for hash tables mapping cache IDs to cached polylines
	
	/* Elements: */
	public:
	std::vector<MemoryBlock*> memoryBlocks; // List of memory blocks managed by this renderer
	CacheMap cacheMap; // Map of cached polylines
	GLuint currentBufferId; // ID of currently bound buffer object
	bool haveCoreGeometryShaders; // Flag whether the OpenGL context supports core feature geometry shaders
	GLhandleARB lineShader; // GLSL shader to render anti-aliased lines
	GLint uniforms[2]; // Locations of the line rendering shader's uniform variables
	Color currentColor; // Color currently uploaded into the line rendering shader
	Scalar currentLineWidth; // Line width currently uploaded into the line rendering shader
	
	/* Constructors and destructors: */
	DataItem(GLContextData& contextData);
	virtual ~DataItem(void);
	
	CacheItem allocate(size_t size); // Allocates a memory chunk of the given size
	};

/*********************************************************
Methods of struct PolylineRenderer::DataItem::MemoryBlock:
*********************************************************/

PolylineRenderer::DataItem::MemoryBlock::MemoryBlock(size_t numVertices)
	:bufferId(0)
	{
	/* Create the memory block buffer: */
	glGenBuffersARB(1,&bufferId);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,bufferId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,numVertices*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Initialize the list of unused chunks: */
	freeChunks.push_back(FreeChunk(0,numVertices));
	}

PolylineRenderer::DataItem::MemoryBlock::~MemoryBlock(void)
	{
	/* Destroy the memory block buffer: */
	if(bufferId!=0U)
		glDeleteBuffersARB(1,&bufferId);
	}

void PolylineRenderer::DataItem::MemoryBlock::release(size_t offset,size_t size)
	{
	/* Find the appropriate slot in the free chunk list: */
	std::vector<FreeChunk>::iterator fcIt;
	for(fcIt=freeChunks.begin();fcIt!=freeChunks.end()&&fcIt->offset<offset;++fcIt)
		;
	
	/* Check if the new free chunk can be merged with the free chunks to the left and/or right of it: */
	bool mergeLeft=fcIt!=freeChunks.begin()&&fcIt[-1].offset+fcIt[-1].size==offset;
	bool mergeRight=fcIt!=freeChunks.end()&&fcIt->offset==offset+size;
	if(mergeLeft&&mergeRight)
		{
		/* Increase the size of the free chunk to the left: */
		fcIt[-1].size+=size+fcIt->size;
		
		/* Remove the free chunk to the right: */
		freeChunks.erase(fcIt);
		}
	else if(mergeLeft)
		{
		/* Increase the size of the free chunk to the left: */
		fcIt[-1].size+=size;
		}
	else if(mergeRight)
		{
		/* Increase the size of the free chunk to the right: */
		fcIt->offset-=size;
		fcIt->size+=size;
		}
	else
		{
		/* Insert a new free chunk into the list: */
		freeChunks.insert(fcIt,FreeChunk(offset,size));
		}
	}

/********************************************
Methods of struct PolylineRenderer::DataItem:
********************************************/

PolylineRenderer::DataItem::DataItem(GLContextData& contextData)
	:cacheMap(17),
	 currentBufferId(0U),
	 haveCoreGeometryShaders(contextData.getContext().isVersionLargerEqual(3,2)),
	 lineShader(0),
	 currentColor(0,0,0),currentLineWidth(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBVertexBufferObject::initExtension();
	GLARBShaderObjects::initExtension();
	GLARBVertexShader::initExtension();
	if(!haveCoreGeometryShaders)
		GLARBGeometryShader4::initExtension();
	GLARBFragmentShader::initExtension();
	
	/* Create the first memory block: */
	memoryBlocks.push_back(new MemoryBlock(1U<<20)); // 1M vertices per block
	
	/* Protect the newly-created buffer: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	/* Create the line rendering shader: */
	lineShader=glCreateProgramObjectARB();
	}

PolylineRenderer::DataItem::~DataItem(void)
	{
	/* Delete all allocated memory blocks: */
	for(std::vector<MemoryBlock*>::iterator mbIt=memoryBlocks.begin();mbIt!=memoryBlocks.end();++mbIt)
		delete *mbIt;
	
	/* Destroy the line rendering shader: */
	glDeleteObjectARB(lineShader);
	}

PolylineRenderer::DataItem::CacheItem PolylineRenderer::DataItem::allocate(size_t size)
	{
	std::vector<MemoryBlock*>::iterator bestMbIt;
	std::vector<MemoryBlock::FreeChunk>::iterator bestFcIt;
	size_t bestSize=~size_t(0);
	for(std::vector<MemoryBlock*>::iterator mbIt=memoryBlocks.begin();mbIt!=memoryBlocks.end();++mbIt)
		{
		for(std::vector<MemoryBlock::FreeChunk>::iterator fcIt=(*mbIt)->freeChunks.begin();fcIt!=(*mbIt)->freeChunks.end();++fcIt)
			{
			if(fcIt->size==size)
				{
				/* Take the chunk immediately if it fits exactly: */
				bestMbIt=mbIt;
				bestFcIt=fcIt;
				bestSize=fcIt->size;
				goto chunkFound;
				}
			else if(fcIt->size>size&&fcIt->size<bestSize)
				{
				/* Tentatively take this chunk: */
				bestMbIt=mbIt;
				bestFcIt=fcIt;
				bestSize=fcIt->size;
				}
			}
		}
	
	/* Check if no chunk was found: */
	if(bestSize==~size_t(0))
		{
		/* Create a new memory block: */
		memoryBlocks.push_back(new MemoryBlock(1U<<20)); // 1M vertices per block
		
		/* Take the new block's free chunk: */
		bestMbIt=memoryBlocks.end()-1;
		bestFcIt=(*bestMbIt)->freeChunks.begin();
		bestSize=bestFcIt->size;
		}
	
	chunkFound:
	
	/* Allocate from the found chunk: */
	CacheItem result(*bestMbIt,bestFcIt->offset,size);
	if(bestSize>size)
		{
		/* Reduce the size of the found chunk: */
		bestFcIt->offset+=size;
		bestFcIt->size-=size;
		}
	else
		{
		/* Remove the found chunk from the free chunk list: */
		(*bestMbIt)->freeChunks.erase(bestFcIt);
		}
	
	return result;
	}

/*****************************************
Static elements of class PolylineRenderer:
*****************************************/

PolylineRenderer PolylineRenderer::theRenderer;

/*********************************
Methods of class PolylineRenderer:
*********************************/

PolylineRenderer::PolylineRenderer(void)
	:scaleFactor(1)
	{
	}

void PolylineRenderer::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and associate it with this object: */
	DataItem* dataItem=new DataItem(contextData);
	contextData.addDataItem(this,dataItem);
	
	/* Create the polyline rendering shader: */
	GLhandleARB vertexShader=glCompileVertexShaderFromFile(SKETCHPAD_SHADERDIR "/CurveRenderer.vs");
	glAttachObjectARB(dataItem->lineShader,vertexShader);
	glDeleteObjectARB(vertexShader);
	
	GLhandleARB geometryShader=0;
	if(dataItem->haveCoreGeometryShaders)
		{
		/* Create a core OpenGL geometry shader: */
		geometryShader=glCompileARBGeometryShader4FromFile(SKETCHPAD_SHADERDIR "/CurveRendererCore.gs");
		}
	else
		{
		/* Create an ARB geometry shader: */
		geometryShader=glCompileARBGeometryShader4FromFile(SKETCHPAD_SHADERDIR "/CurveRendererARB.gs");
		
		glProgramParameteriARB(dataItem->lineShader,GL_GEOMETRY_INPUT_TYPE_ARB,GL_LINES);
		glProgramParameteriARB(dataItem->lineShader,GL_GEOMETRY_OUTPUT_TYPE_ARB,GL_TRIANGLE_STRIP);
		glProgramParameteriARB(dataItem->lineShader,GL_GEOMETRY_VERTICES_OUT_ARB,8);
		}
	glAttachObjectARB(dataItem->lineShader,geometryShader);
	glDeleteObjectARB(geometryShader);
	
	GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(SKETCHPAD_SHADERDIR "/CurveRenderer.fs");
	glAttachObjectARB(dataItem->lineShader,fragmentShader);
	glDeleteObjectARB(fragmentShader);
	
	glLinkAndTestShader(dataItem->lineShader);
	dataItem->uniforms[0]=glGetUniformLocationARB(dataItem->lineShader,"lineWidth");
	dataItem->uniforms[1]=glGetUniformLocationARB(dataItem->lineShader,"pixelSize");
	}

GLObject::DataItem* PolylineRenderer::activate(GLContextData& contextData) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Enable vertex array rendering: */
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT,sizeof(DataItem::Vertex),static_cast<const GLfloat*>(0)+0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(DataItem::Vertex),static_cast<const GLfloat*>(0)+3);
	
	/* Activate the line rendering shader: */
	glUseProgramObjectARB(dataItem->lineShader);
	
	/* Upload the current line width: */
	glUniform1fARB(dataItem->uniforms[0],dataItem->currentLineWidth*scaleFactor);
	
	/* Calculate this display's pixel size in model coordinate units: */
	const Vrui::DisplayState& ds=Vrui::getDisplayState(contextData);
	const Vrui::Scalar* panRect=ds.window->getPanRect();
	Vrui::Scalar pw=ds.screen->getWidth()*(panRect[1]-panRect[0])/Vrui::Scalar(ds.viewport.size[0]);
	Vrui::Scalar ph=ds.screen->getHeight()*(panRect[3]-panRect[2])/Vrui::Scalar(ds.viewport.size[1]);
	Vrui::Scalar pixelSize=Math::sqrt(pw*ph);
	pixelSize*=Vrui::getInverseNavigationTransformation().getScaling();
	
	/* Upload the pixel size: */
	glUniform1fARB(dataItem->uniforms[1],float(pixelSize));
	
	/* Return the context data item: */
	return dataItem;
	}

void PolylineRenderer::deactivate(GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* Disable the line rendering shader: */
	glUseProgramObjectARB(0);
	
	/* Unbind any currently bound buffers: */
	if(myDataItem->currentBufferId!=0U)
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0U);
	myDataItem->currentBufferId=0U;
	
	/* Disable vertex array rendering: */
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	}

void PolylineRenderer::setScaleFactor(Scalar newScaleFactor)
	{
	scaleFactor=newScaleFactor;
	}

void PolylineRenderer::draw(const PolylineRenderer::Polyline& polyline,const Color& color,Scalar lineWidth,GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* Update the line width if necessary: */
	if(myDataItem->currentLineWidth!=lineWidth)
		{
		myDataItem->currentLineWidth=lineWidth;
		glUniform1fARB(myDataItem->uniforms[0],myDataItem->currentLineWidth*scaleFactor);
		}
	
	/* Draw the polyline: */
	glColor(color);
	if(polyline.size()>2)
		{
		/* Draw the polyline as a line strip: */
		glBegin(GL_LINE_STRIP);
		Polyline::const_iterator p0It=polyline.begin();
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(*p0It);
		
		Polyline::const_iterator p1It=p0It+1;
		Vector v0=*p1It-*p0It;
		v0.normalize();
		for(p0It=p1It,++p1It;p1It!=polyline.end();p0It=p1It,++p1It)
			{
			/* Calculate the separating normal vector between the two adjacent line segments: */
			Vector v1=*p1It-*p0It;
			v1.normalize();
			if(v0*v1>=Scalar(0))
				glNormal(v0+v1);
			else
				glNormal3f(0.0f,0.0f,0.0f);
			glVertex(*p0It);
			
			/* Go to the next line segment: */
			v0=v1;
			}
		
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(*p0It);
		glEnd();
		}
	else if(polyline.size()==2)
		{
		/* Draw a single line segment: */
		glBegin(GL_LINES);
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(polyline[0]);
		glVertex(polyline[1]);
		glEnd();
		}
	else
		{
		/* Draw a single point as a line with identical end points: */
		glBegin(GL_LINES);
		glNormal3f(0.0f,0.0f,0.0f);
		glVertex(polyline[0]);
		glVertex(polyline[0]);
		glEnd();
		}
	}

void PolylineRenderer::cache(const void* cacheId,const PolylineRenderer::Polyline& polyline,GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* Find a memory chunk to hold the polyline's vertices: */
	DataItem::CacheItem cacheItem=myDataItem->allocate(polyline.size());
	
	/* Upload the polyline to the memory chunk: */
	if(myDataItem->currentBufferId!=cacheItem.memoryBlock->bufferId)
		{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,cacheItem.memoryBlock->bufferId);
		myDataItem->currentBufferId=cacheItem.memoryBlock->bufferId;
		}
	DataItem::Vertex* vPtr=static_cast<DataItem::Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
	vPtr+=cacheItem.offset;
	if(polyline.size()>2)
		{
		/* Draw a line strip: */
		Polyline::const_iterator p0It=polyline.begin();
		vPtr->normal=Vector::zero;
		vPtr->position=*p0It;
		++vPtr;
		
		Polyline::const_iterator p1It=p0It+1;
		Vector v0=*p1It-*p0It;
		v0.normalize();
		for(p0It=p1It,++p1It;p1It!=polyline.end();p0It=p1It,++p1It)
			{
			/* Calculate the separating normal vector between the two adjacent line segments: */
			Vector v1=*p1It-*p0It;
			v1.normalize();
			if(v0*v1>=Scalar(0))
				vPtr->normal=(v0+v1);
			else
				vPtr->normal=Vector::zero;
			vPtr->position=*p0It;
			++vPtr;
			
			/* Go to the next line segment: */
			v0=v1;
			}
		
		vPtr->normal=Vector::zero;
		vPtr->position=*p0It;
		++vPtr;
		}
	else if(polyline.size()==2)
		{
		/* Draw a single line segment: */
		vPtr->normal=Vector::zero;
		vPtr->position=polyline[0];
		++vPtr;
		vPtr->normal=Vector::zero;
		vPtr->position=polyline[1];
		++vPtr;
		}
	else
		{
		/* Draw a line segment with identical end points: */
		vPtr->normal=Vector::zero;
		vPtr->position=polyline[0];
		++vPtr;
		vPtr->normal=Vector::zero;
		vPtr->position=polyline[0];
		++vPtr;
		}
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Add the polyline to the cache: */
	myDataItem->cacheMap.setEntry(DataItem::CacheMap::Entry(cacheId,cacheItem));
	}

void PolylineRenderer::draw(const void* cacheId,const Color& color,Scalar lineWidth,GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* Update the line width if necessary: */
	if(myDataItem->currentLineWidth!=lineWidth)
		{
		myDataItem->currentLineWidth=lineWidth;
		glUniform1fARB(myDataItem->uniforms[0],myDataItem->currentLineWidth*scaleFactor);
		}
	
	/* Retrieve the polyline's cache entry: */
	DataItem::CacheItem& cacheItem=myDataItem->cacheMap.getEntry(cacheId).getDest();
	
	/* Bind the polyline's memory block: */
	if(myDataItem->currentBufferId!=cacheItem.memoryBlock->bufferId)
		{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,cacheItem.memoryBlock->bufferId);
		myDataItem->currentBufferId=cacheItem.memoryBlock->bufferId;
		}
	
	/* Draw the polyline as a line strip: */
	glColor(color);
	glDrawArrays(GL_LINE_STRIP,cacheItem.offset,cacheItem.size);
	}
