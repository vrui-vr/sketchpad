/***********************************************************************
SketchGeometry - Definitions of geometry types used in the SketchPad
application.
Copyright (c) 2016 Oliver Kreylos
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
