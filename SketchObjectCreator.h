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
	SketchObject* readObject(IO::File& file) const; // Reads a sketch object from the given file
	void writeObject(const SketchObject* object,IO::File& file) const; // Writes the given sketch object to the given file
	};

#endif
