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

#ifndef _ALGOTABLE_H_
#define _ALGOTABLE_H_

#include "Shape.h"

/*! The number of different convex shapes supported */
const int NUM_TYPES = 8;

/*! Function Look up table class for one type of convex shape against another */
template <class Function>
class AlgoTable {
public:
  void addEntry(ShapeType type1, ShapeType type2, Function function) { 
    table[type2][type1] = function;
    table[type1][type2] = function;
  }

  Function lookup(ShapeType type1, ShapeType type2) const {
    return table[type1][type2];
  }

private:
  Function table[NUM_TYPES][NUM_TYPES];
};

#endif
