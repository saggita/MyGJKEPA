#pragma once

class CCollisionObject;

class ICollidable
{
public:
	ICollidable(void);
	virtual ~ICollidable(void);
	
protected:
	float m_Margin;

public:
	float GetMargin() const { return m_Margin; }
	void SetMargin(float margin) { m_Margin = margin; }

	virtual CCollisionObject* GetCollisionObject() = 0;
	virtual const CCollisionObject* GetCollisionObject() const = 0;
};

