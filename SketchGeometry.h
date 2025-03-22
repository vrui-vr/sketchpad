/***********************************************************************
SketchGeometry - Definitions of geometry types used in the SketchPad
application.
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

#ifndef SKETCHGEOMETRY_INCLUDED
#define SKETCHGEOMETRY_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>

typedef float Scalar; // Scalar type for affine space
typedef Geometry::Point<Scalar,3> Point; // Point type
typedef Geometry::Vector<Scalar,3> Vector; // Vector type
typedef Geometry::Box<Scalar,3> Box; // Axis-aligned box type
typedef Geometry::OrthogonalTransformation<Scalar,3> Transformation; // Transformation type

typedef GLColor<GLubyte,4> Color; // Type for object colors

#endif
