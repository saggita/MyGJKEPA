#pragma once

class CCollisionObject;

class ICollidable
{
public:
	ICollidable(void);
	virtual ~ICollidable(void);

	virtual CCollisionObject* GetCollisionObject() = 0;
	virtual const CCollisionObject* GetCollisionObject() const = 0;
};

