#pragma once

#include "CollisionObject.h"

class CRigidBody : public CCollisionObject
{
public:
	CRigidBody(void);
	virtual ~CRigidBody(void);

	//CCollisionObject* m_pColObj;

public:
	void Create();

	/*CCollisionObject* GetCollisionObject() { return m_pColObj; }
	const CCollisionObject* GetCollisionObject() const { return m_pColObj; }*/
};

