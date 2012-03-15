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

#ifndef SOLID_SOLID_H
#define SOLID_SOLID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

typedef void *DtObjectRef;
DT_DECLARE_HANDLE(DtShapeRef);

typedef enum DtPolyType {
  DT_SIMPLEX,
  DT_POLYGON,
  DT_POLYHEDRON
} DtPolyType;

typedef enum DtResponseType { 
  DT_NO_RESPONSE,
  DT_SIMPLE_RESPONSE,
  DT_SMART_RESPONSE,
  DT_WITNESSED_RESPONSE,
} DtResponseType;

typedef struct DtCollData {
  DT_Vector3 point1;
  DT_Vector3 point2;
  DT_Vector3 normal;
} DtCollData;

typedef void (*DtResponse)(
  void *client_data,
  DtObjectRef object1,
  DtObjectRef object2,
  const DtCollData *coll_data);



/* Shape definition */

extern FREESOLID_DLL DtShapeRef dtBox(DT_Scalar x, DT_Scalar y, DT_Scalar z);
extern FREESOLID_DLL DtShapeRef dtCone(DT_Scalar radius, DT_Scalar height);
extern FREESOLID_DLL DtShapeRef dtCylinder(DT_Scalar radius, DT_Scalar height);
extern FREESOLID_DLL DtShapeRef dtSphere(DT_Scalar radius);
extern FREESOLID_DLL DtShapeRef dtEllipsoid(DT_Scalar radiusx,DT_Scalar radiusy,DT_Scalar radiusz);

extern FREESOLID_DLL DtShapeRef dtNewComplexShape();
extern FREESOLID_DLL void dtEndComplexShape();

extern FREESOLID_DLL void dtBegin(DtPolyType type);
extern FREESOLID_DLL void dtEnd();

extern FREESOLID_DLL void dtVertex(DT_Scalar x, DT_Scalar y, DT_Scalar z);
extern FREESOLID_DLL void dtVertexBase(const void *base);
extern FREESOLID_DLL void dtVertexIndex(DT_Index index);
extern FREESOLID_DLL void dtVertexIndices(DtPolyType type, DT_Count count, 
			    const DT_Index *indices);
extern FREESOLID_DLL void dtVertexRange(DtPolyType type, DT_Index first, DT_Count count); 

extern FREESOLID_DLL void dtChangeVertexBase(DtShapeRef shape, const void *base);

extern FREESOLID_DLL void dtDeleteShape(DtShapeRef shape);


/* Object  */

extern FREESOLID_DLL void dtCreateObject(DtObjectRef object, DtShapeRef shape); 
extern FREESOLID_DLL void dtDeleteObject(DtObjectRef object);
extern FREESOLID_DLL void dtSelectObject(DtObjectRef object);

extern FREESOLID_DLL void dtLoadIdentity();

extern FREESOLID_DLL void dtLoadMatrixf(const float *m);
extern FREESOLID_DLL void dtLoadMatrixd(const double *m);

extern FREESOLID_DLL void dtMultMatrixf(const float *m);
extern FREESOLID_DLL void dtMultMatrixd(const double *m);

extern FREESOLID_DLL void dtTranslate(DT_Scalar x, DT_Scalar y, DT_Scalar z);
extern FREESOLID_DLL void dtRotate(DT_Scalar x, DT_Scalar y, DT_Scalar z, DT_Scalar w);
extern FREESOLID_DLL void dtScale(DT_Scalar x, DT_Scalar y, DT_Scalar z);


/* Response */

extern FREESOLID_DLL void dtSetDefaultResponse(DtResponse response, DtResponseType type, 
				 void *client_data);

extern FREESOLID_DLL void dtClearDefaultResponse();

extern FREESOLID_DLL void dtSetObjectResponse(DtObjectRef object, DtResponse response, 
				DtResponseType type, void *client_data);
extern FREESOLID_DLL void dtClearObjectResponse(DtObjectRef object);
extern FREESOLID_DLL void dtResetObjectResponse(DtObjectRef object);

extern FREESOLID_DLL void dtSetPairResponse(DtObjectRef object1, DtObjectRef object2, 
			      DtResponse response, DtResponseType type, 
			      void *client_data);
extern FREESOLID_DLL void dtClearPairResponse(DtObjectRef object1, DtObjectRef object2);
extern FREESOLID_DLL void dtResetPairResponse(DtObjectRef object1, DtObjectRef object2);


/* Global */

extern FREESOLID_DLL DT_Count dtTest();
extern FREESOLID_DLL void dtTestObjects(DtObjectRef object1, DtObjectRef object2);
extern FREESOLID_DLL void dtProceed();

extern FREESOLID_DLL void dtEnableCaching();
extern FREESOLID_DLL void dtDisableCaching();

extern FREESOLID_DLL void dtSetTolerance(DT_Scalar tol);

#ifdef __cplusplus
}
#endif

#endif
