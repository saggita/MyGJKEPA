/*
  FreeSOLID - Software Library for Interference Detection
  Copyright (C) 1997-1998  Gino van den Bergen

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Please send remarks, questions and bug reports to gino@win.tue.nl,
  or write to:
                  Gino van den Bergen
		  Department of Mathematics and Computing Science
		  Eindhoven University of Technology
		  P.O. Box 513, 5600 MB Eindhoven, The Netherlands
*/

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <SOLID/solid.h>
#include <SOLID/broad.h>

#include "Transform.h"
#include "BBox.h"
#include "Shape.h"

class Object 
{
 public:
  Object(DtObjectRef obj, ShapePtr shape, BP_SceneHandle broadphase); 
  ~Object();
  
  void do_broadphase();
  void proceed();
  
  void translate(const Vector& v) 
  {
    curr.translate(v); 
    m_dirty = true; 
  }
  void rotate(const Quaternion& q) 
  {
    curr.rotate(q); 
    m_dirty = true;
  }
  void scale(Scalar x, Scalar y, Scalar z)
  { 
    curr.scale(x, y, z); 
    m_dirty = true;
  }
  
  void setIdentity()
  {
    curr.setIdentity(); 
    m_dirty = true;
  }
	
  void setMatrix(const float v[16]) 
  { 
    curr.setValue(v); 
    m_dirty = true;
  }
  // Saved for later
  /*	void setMatrix(const double v[16]) { 
    curr.setValue(v); 
    m_dirty = true;
    }
  */	
  void multMatrix(const float v[16])
  {
    curr *= Transform(v);
    m_dirty = true;
  }
// Saved for later
/*	
  void multMatrix(const double v[16]) 
  {
  curr *= Transform(v);
  m_dirty = true;
  }
*/	
  Transform curr;
  // prev seems to NEVER get set or updated, yet it does get used.
  Transform prev;
  DtObjectRef ref;
  ShapePtr shapePtr;
  
  //For broadphase
  mutable BBox bbox;
  mutable bool m_dirty;
  BP_SceneHandle m_broadphase;
  BP_ProxyHandle m_proxy;
};

bool intersect(const Object&, const Object&, Vector& v);
bool common_point(const Object&, const Object&, Vector&, Point&, Point&);
bool prev_closest_points(const Object&, const Object&,Vector&, Point&, Point&);
#endif





