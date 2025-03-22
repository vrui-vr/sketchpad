/***********************************************************************
SketchObjectCreator - Class keeping track of sketch object classes.
Copyright (c) 2016 Oliver Kreylos
***********************************************************************/

#ifndef SKETCHOBJECTCREATOR_INCLUDED
#define SKETCHOBJECTCREATOR_INCLUDED

/* Forward declarations: */
namespace IO {
class File;
}
class SketchObject;

class SketchObjectCreator
	{
	/* Constructors and destructors: */
	public:
	SketchObjectCreator(void); // Assigns unique type codes to all sketch object classes
	~SketchObjectCreator(void); // Destroys all sketch object classes
	
	/* Methods: */
	SketchObject* createObject(unsigned int typeCode); // Returns a new object of a class matching the given type code
	SketchObject* readObject(IO::File& file); // Reads a sketch object from the given file
	void writeObject(const SketchObject* object,IO::File& file) const; // Writes the given sketch object to the given file
	};

#endif
