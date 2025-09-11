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
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBCopyBuffer.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBGeometryShader4.h>
#include <GL/Extensions/GLARBFragmentShader.h>
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
		};
	
	struct CacheItem // Structure representing a cached polyline
		{
		/* Elements: */
		public:
		MemoryBlock* memoryBlock; // Pointer to memory block containing the polyline's vertex data
		size_t offset,size; // Offset and size of memory chunk allocated to the polyline, in units of vertices
		unsigned int version; // Version number of polyline in the cache
		
		/* Constructors and destructors: */
		CacheItem(MemoryBlock* sMemoryBlock,size_t sOffset,size_t sSize)
			:memoryBlock(sMemoryBlock),offset(sOffset),size(sSize),version(0)
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
	CacheItem* uploadItem; // Pointer to a cache item whose vertices are currently being uploaded into the item's memory chunk
	DataItem::Vertex* uploadPtr; // Position in the upload memory chunk where the next vertex will be uploaded
	DataItem::Vertex* uploadEnd; // Pointer after the end of the allocated memory chunk
	size_t uploadNumVertices; // Number of polyline vertices already uploaded
	Point uploadP0; // The previously uploaded polyline vertex
	
	/* Constructors and destructors: */
	DataItem(GLContextData& contextData);
	virtual ~DataItem(void);
	
	CacheItem allocate(size_t size); // Allocates a memory chunk of the given size
	CacheItem allocateLargest(size_t minSize); // Allocates the largest memory chunk of the given minimum size
	void release(const CacheItem& cacheItem); // Releases an allocated memory chunk
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

/********************************************
Methods of struct PolylineRenderer::DataItem:
********************************************/

PolylineRenderer::DataItem::DataItem(GLContextData& contextData)
	:cacheMap(17),
	 currentBufferId(0U),
	 haveCoreGeometryShaders(contextData.getContext().isVersionLargerEqual(3,2)),
	 lineShader(0),
	 currentColor(0,0,0),currentLineWidth(0),
	 uploadItem(0),uploadPtr(0),uploadEnd(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBVertexBufferObject::initExtension();
	GLARBCopyBuffer::initExtension();
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
	/* Find a free memory chunk that either fits the request exactly or has the smallest overhead: */
	std::vector<MemoryBlock*>::iterator bestMbIt;
	std::vector<MemoryBlock::FreeChunk>::iterator bestFcIt;
	size_t bestSize=~size_t(0);
	for(std::vector<MemoryBlock*>::iterator mbIt=memoryBlocks.begin();mbIt!=memoryBlocks.end();++mbIt)
		{
		for(std::vector<MemoryBlock::FreeChunk>::iterator fcIt=(*mbIt)->freeChunks.begin();fcIt!=(*mbIt)->freeChunks.end();++fcIt)
			{
			if(fcIt->size>=size&&bestSize>fcIt->size)
				{
				/* Tentatively take this chunk: */
				bestMbIt=mbIt;
				bestFcIt=fcIt;
				bestSize=fcIt->size;
				
				/* Stop looking if the chunk fits the request exactly: */
				if(bestSize==size)
					goto chunkFound;
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

PolylineRenderer::DataItem::CacheItem PolylineRenderer::DataItem::allocateLargest(size_t minSize)
	{
	/* Find the largest free memory chunk: */
	std::vector<MemoryBlock*>::iterator bestMbIt;
	std::vector<MemoryBlock::FreeChunk>::iterator bestFcIt;
	size_t bestSize=0U;
	for(std::vector<MemoryBlock*>::iterator mbIt=memoryBlocks.begin();mbIt!=memoryBlocks.end();++mbIt)
		{
		for(std::vector<MemoryBlock::FreeChunk>::iterator fcIt=(*mbIt)->freeChunks.begin();fcIt!=(*mbIt)->freeChunks.end();++fcIt)
			{
			if(bestSize<fcIt->size)
				{
				/* Tentatively take this chunk: */
				bestMbIt=mbIt;
				bestFcIt=fcIt;
				bestSize=fcIt->size;
				}
			}
		}
	
	/* Check if no appropriate chunk was found: */
	if(bestSize<minSize)
		{
		/* Create a new memory block: */
		memoryBlocks.push_back(new MemoryBlock(1U<<20)); // 1M vertices per block
		
		/* Take the new block's free chunk: */
		bestMbIt=memoryBlocks.end()-1;
		bestFcIt=(*bestMbIt)->freeChunks.begin();
		bestSize=bestFcIt->size;
		}
	
	/* Allocate the entire found chunk: */
	CacheItem result(*bestMbIt,bestFcIt->offset,bestSize);
	
	/* Remove the found chunk from the free chunk list: */
	(*bestMbIt)->freeChunks.erase(bestFcIt);
	
	return result;
	}

void PolylineRenderer::DataItem::release(const PolylineRenderer::DataItem::CacheItem& cacheItem)
	{
	/* Find the appropriate slot in the memory block's free chunk list: */
	std::vector<MemoryBlock::FreeChunk>& freeChunks=cacheItem.memoryBlock->freeChunks;
	std::vector<MemoryBlock::FreeChunk>::iterator fcIt;
	for(fcIt=freeChunks.begin();fcIt!=freeChunks.end()&&fcIt->offset<cacheItem.offset;++fcIt)
		;
	
	/* Check if the new free chunk can be merged with the free chunks to the left and/or right of it: */
	bool mergeLeft=fcIt!=freeChunks.begin()&&fcIt[-1].offset+fcIt[-1].size==cacheItem.offset;
	bool mergeRight=fcIt!=freeChunks.end()&&fcIt->offset==cacheItem.offset+cacheItem.size;
	if(mergeLeft&&mergeRight)
		{
		/* Increase the size of the free chunk to the left: */
		fcIt[-1].size+=cacheItem.size+fcIt->size;
		
		/* Remove the free chunk to the right: */
		freeChunks.erase(fcIt);
		}
	else if(mergeLeft)
		{
		/* Increase the size of the free chunk to the left: */
		fcIt[-1].size+=cacheItem.size;
		}
	else if(mergeRight)
		{
		/* Increase the size of the free chunk to the right: */
		fcIt->offset-=cacheItem.size;
		fcIt->size+=cacheItem.size;
		}
	else
		{
		/* Insert a new free chunk into the list: */
		freeChunks.insert(fcIt,MemoryBlock::FreeChunk(cacheItem.offset,cacheItem.size));
		}
	}

/*****************************************
Static elements of class PolylineRenderer:
*****************************************/

PolylineRenderer* PolylineRenderer::theRenderer=0;
Threads::Atomic<unsigned int> PolylineRenderer::refCount(0);

/*********************************
Methods of class PolylineRenderer:
*********************************/

void PolylineRenderer::cleanCache(Vrui::PreRenderingCallbackData* cbData)
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=cbData->contextData.retrieveDataItem<DataItem>(this);
	
	/* Process the drop list: */
	for(std::vector<const void*>::const_iterator dlIt=dropList.begin();dlIt!=dropList.end();++dlIt)
		{
		DataItem::CacheMap::Iterator cmIt=dataItem->cacheMap.findEntry(*dlIt);
		if(!cmIt.isFinished())
			{
			/* Release the item's memory chunk: */
			dataItem->release(cmIt->getDest());
			
			/* Remove the item from the cache: */
			dataItem->cacheMap.removeEntry(cmIt);
			}
		}
	}

void PolylineRenderer::clearDropList(Misc::CallbackData*)
	{
	/* Clear the drop list: */
	dropList.clear();
	}

PolylineRenderer::PolylineRenderer(void)
	:scaleFactor(1)
	{
	/* Install callbacks with the Vrui kernel: */
	Vrui::getPreRenderingCallbacks().add(this,&PolylineRenderer::cleanCache);
	Vrui::getPostRenderingCallbacks().add(this,&PolylineRenderer::clearDropList);
	}

PolylineRenderer::~PolylineRenderer(void)
	{
	/* Remove callbacks from the Vrui kernel: */
	Vrui::getPreRenderingCallbacks().remove(this,&PolylineRenderer::cleanCache);
	Vrui::getPostRenderingCallbacks().remove(this,&PolylineRenderer::clearDropList);
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
	glEnableClientState(GL_VERTEX_ARRAY);
	
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
	glDisableClientState(GL_VERTEX_ARRAY);
	}

PolylineRenderer* PolylineRenderer::acquire(void)
	{
	/* Increase the reference count and create a new singleton object if the reference count was zero: */
	if(refCount.postAdd(1)==0)
		theRenderer=new PolylineRenderer;
	
	return theRenderer;
	}

void PolylineRenderer::release(void)
	{
	/* Decrease the reference count and delete the singleton object if the reference count reaches zero: */
	if(refCount.preSub(1)==0)
		{
		delete theRenderer;
		theRenderer=0;
		}
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

void PolylineRenderer::draw(const void* cacheId,unsigned int version,const PolylineRenderer::Polyline& polyline,const Color& color,Scalar lineWidth,GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* Find the polyline in the cache: */
	bool uploadPolyline=false;
	DataItem::CacheMap::Iterator cmIt=myDataItem->cacheMap.findEntry(cacheId);
	if(cmIt.isFinished())
		{
		/* Find a memory chunk to hold the polyline's vertices: */
		DataItem::CacheItem cacheItem=myDataItem->allocate(Misc::max(polyline.size(),size_t(2))+2U); // Polylines use at least two vertices and two extra vertices
		
		/* Add the polyline to the cache: */
		cacheItem.version=version;
		cmIt=myDataItem->cacheMap.setAndFindEntry(DataItem::CacheMap::Entry(cacheId,cacheItem));
		
		/* Upload the polyline's vertices later: */
		uploadPolyline=true;
		}
	else if(cmIt->getDest().version!=version)
		{
		/* Assign a different memory chunk to hold the polyline's vertices: */
		myDataItem->release(cmIt->getDest());
		cmIt->getDest()=myDataItem->allocate(Misc::max(polyline.size(),size_t(2))+2U); // Polylines use at least two vertices and two extra vertices
		
		/* Update the cache item's version number: */
		cmIt->getDest().version=version;
		
		/* Upload the polyline's vertices later: */
		uploadPolyline=true;
		}
	
	/* Check if the polyline's vertices are in a different memory block: */
	if(myDataItem->currentBufferId!=cmIt->getDest().memoryBlock->bufferId)
		{
		/* Bind the polyline's memory block: */
		myDataItem->currentBufferId=cmIt->getDest().memoryBlock->bufferId;
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,myDataItem->currentBufferId);
		
		/* Reset the vertex pointers into the new memory block: */
		glVertexPointer(3,GL_FLOAT,sizeof(DataItem::Vertex),static_cast<const GLfloat*>(0)+0);
		}
	
	/* Upload the polyline's vertices to the assigned memory chunk if necessary: */
	if(uploadPolyline)
		{
		/* Map the allocated memory chunk: */
		DataItem::Vertex* vPtr=static_cast<DataItem::Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		vPtr+=cmIt->getDest().offset;
		
		/* Upload the first polyline vertex: */
		vPtr->position=polyline.front();
		++vPtr;
		
		if(polyline.size()>1)
			{
			/* Upload all polyline vertices: */
			for(Polyline::const_iterator pIt=polyline.begin();pIt!=polyline.end();++pIt)
				{
				vPtr->position=*pIt;
				++vPtr;
				}
			}
		else
			{
			/* Upload the only polyline vertex twice: */
			vPtr->position=polyline.front();
			++vPtr;
			vPtr->position=polyline.back();
			++vPtr;
			}
		
		/* Upload the last polyline vertex: */
		vPtr->position=polyline.back();
		++vPtr;
		
		/* Unmap the memory chunk: */
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		}
	
	/* Update the line width if necessary: */
	if(myDataItem->currentLineWidth!=lineWidth)
		{
		myDataItem->currentLineWidth=lineWidth;
		glUniform1fARB(myDataItem->uniforms[0],myDataItem->currentLineWidth*scaleFactor);
		}
	
	/* Set the color: */
	glColor(color);
	
	/* Draw the polyline as a line strip with adjacency: */
	glDrawArrays(GL_LINE_STRIP_ADJACENCY,cmIt->getDest().offset,cmIt->getDest().size);
	}

bool PolylineRenderer::draw(const void* cacheId,unsigned int version,const Color& color,Scalar lineWidth,GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* Find the polyline in the cache: */
	DataItem::CacheMap::Iterator cmIt=myDataItem->cacheMap.findEntry(cacheId);
	bool uploadPolyline=false;
	if(cmIt.isFinished())
		{
		/* Find the largest available memory chunk to hold the polyline's vertices: */
		DataItem::CacheItem cacheItem=myDataItem->allocateLargest(4U); // Every polyline has at least two vertices and two extra vertices
		
		/* Add the polyline to the cache: */
		cacheItem.version=version;
		cmIt=myDataItem->cacheMap.setAndFindEntry(DataItem::CacheMap::Entry(cacheId,cacheItem));
		
		/* Upload the polyline's vertices later: */
		uploadPolyline=true;
		}
	else if(cmIt->getDest().version!=version)
		{
		/* Release the polyline's current memory chunk: */
		myDataItem->release(cmIt->getDest());
		
		/* Find the largest available memory chunk to hold the polyline's vertices: */
		cmIt->getDest()=myDataItem->allocateLargest(2U); // Every polyline has at least two vertices
		
		/* Update the cache item's version number: */
		cmIt->getDest().version=version;
		
		/* Upload the polyline's vertices later: */
		uploadPolyline=true;
		}
	
	/* Check if the polyline's vertices are in a different memory block: */
	if(myDataItem->currentBufferId!=cmIt->getDest().memoryBlock->bufferId)
		{
		/* Bind the polyline's memory block: */
		myDataItem->currentBufferId=cmIt->getDest().memoryBlock->bufferId;
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,myDataItem->currentBufferId);
		
		/* Reset the vertex pointers into the new memory block: */
		glVertexPointer(3,GL_FLOAT,sizeof(DataItem::Vertex),static_cast<const GLfloat*>(0)+3);
		}
	
	/* Update the line width if necessary: */
	if(myDataItem->currentLineWidth!=lineWidth)
		{
		myDataItem->currentLineWidth=lineWidth;
		glUniform1fARB(myDataItem->uniforms[0],myDataItem->currentLineWidth*scaleFactor);
		}
	
	/* Set the color: */
	glColor(color);
	
	if(uploadPolyline)
		{
		/* Mark the cache item for upload: */
		myDataItem->uploadItem=&cmIt->getDest();
		
		/* Prepare the allocated memory chunk for polyline vertex upload: */
		myDataItem->uploadPtr=static_cast<DataItem::Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		myDataItem->uploadPtr+=myDataItem->uploadItem->offset;
		myDataItem->uploadEnd=myDataItem->uploadPtr+myDataItem->uploadItem->size;
		
		myDataItem->uploadNumVertices=0U;
		}
	else
		{
		/* Draw the polyline as a line strip with adjacency: */
		glDrawArrays(GL_LINE_STRIP_ADJACENCY,cmIt->getDest().offset,cmIt->getDest().size);
		}
	
	return uploadPolyline;
	}

void PolylineRenderer::addVertex(const Point& vertex,GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	if(myDataItem->uploadNumVertices==0U)
		{
		/* Upload the initial vertex: */
		myDataItem->uploadPtr->position=vertex;
		++myDataItem->uploadPtr;
		}
	
	/* Upload the current vertex: */
	myDataItem->uploadPtr->position=vertex;
	++myDataItem->uploadPtr;
	
	/* Remember this vertex in case it's the last: */
	myDataItem->uploadP0=vertex;
	++myDataItem->uploadNumVertices;
	
	/* Check if there is no more room in the allocated memory chunk: */
	if(myDataItem->uploadPtr==myDataItem->uploadEnd)
		{
		/* Allocate another memory chunk: */
		DataItem::CacheItem newItem=myDataItem->allocateLargest((myDataItem->uploadNumVertices*4U)/3U+1U); // Geometric growth to achieve O(N) upload time
		newItem.version=myDataItem->uploadItem->version;
		
		/* Copy already-uploaded vertices from the current into the new memory chunk: */
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		glBindBufferARB(GL_COPY_WRITE_BUFFER,newItem.memoryBlock->bufferId);
		glCopyBufferSubData(GL_ARRAY_BUFFER_ARB,GL_COPY_WRITE_BUFFER,myDataItem->uploadItem->offset*sizeof(DataItem::Vertex),newItem.offset*sizeof(DataItem::Vertex),myDataItem->uploadItem->size*sizeof(DataItem::Vertex));
		glBindBufferARB(GL_COPY_WRITE_BUFFER,0);
		if(myDataItem->currentBufferId!=newItem.memoryBlock->bufferId)
			{
			/* Bind the polyline's new memory block: */
			myDataItem->currentBufferId=newItem.memoryBlock->bufferId;
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,myDataItem->currentBufferId);
			
			/* Reset the vertex pointers into the new memory block: */
			glNormalPointer(GL_FLOAT,sizeof(DataItem::Vertex),static_cast<const GLfloat*>(0)+0);
			glVertexPointer(3,GL_FLOAT,sizeof(DataItem::Vertex),static_cast<const GLfloat*>(0)+3);
			}
		
		/* Release the current memory chunk and install the new one: */
		myDataItem->release(*myDataItem->uploadItem);
		*myDataItem->uploadItem=newItem;
		myDataItem->uploadPtr=static_cast<DataItem::Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		myDataItem->uploadPtr+=myDataItem->uploadItem->offset;
		myDataItem->uploadEnd=myDataItem->uploadPtr+myDataItem->uploadItem->size;
		}
	}

void PolylineRenderer::finish(GLObject::DataItem* dataItem) const
	{
	/* Retrieve the data item: */
	DataItem* myDataItem=static_cast<DataItem*>(dataItem);
	
	/* If only one vertex was specified, use it twice: */
	if(myDataItem->uploadNumVertices==1U)
		{
		myDataItem->uploadPtr->position=myDataItem->uploadP0;
		++myDataItem->uploadPtr;
		}
	
	/* Upload the final vertex: */
	myDataItem->uploadPtr->position=myDataItem->uploadP0;
	++myDataItem->uploadPtr;
	
	/* Finalize the allocated memory chunk: */
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Check if there is leftover space in the allocated memory chunk: */
	size_t leftoverSpace=size_t(myDataItem->uploadEnd-myDataItem->uploadPtr);
	if(leftoverSpace!=0U)
		{
		/* Release the unused part of the allocated memory chunk: */
		DataItem::CacheItem unusedItem=*myDataItem->uploadItem;
		myDataItem->uploadItem->size-=leftoverSpace;
		unusedItem.offset+=myDataItem->uploadItem->size;
		unusedItem.size=leftoverSpace;
		myDataItem->release(unusedItem);
		}
	
	/* Draw the polyline as a line strip with adjacency: */
	glDrawArrays(GL_LINE_STRIP_ADJACENCY,myDataItem->uploadItem->offset,myDataItem->uploadItem->size);
	
	/* Reset upload state: */
	myDataItem->uploadItem=0;
	myDataItem->uploadEnd=myDataItem->uploadPtr=0;
	}

void PolylineRenderer::drop(const void* cacheId)
	{
	/* Add the item to the drop list: */
	dropList.push_back(cacheId);
	}
