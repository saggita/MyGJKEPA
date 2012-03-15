#include "Transform.h"

CTransform::CTransform(void)
{
}

CTransform::CTransform(const CTransform& other)
{
	m_Translation = other.m_Translation;
	m_Rotation = other.m_Rotation;
}

CTransform::CTransform(const CVector3D& translation, const CQuaternion& rotation)
{
	m_Translation = translation;
	m_Rotation = rotation;
}

CTransform::~CTransform(void)
{
}

void CTransform::Inverse()
{	
	m_Rotation.Inverse();

	/*CMatrix33 mat;
	m_Rotation.GetRotation(&mat);*/

	m_Translation = m_Rotation * (-m_Translation);
}

CTransform CTransform::InverseOther() const
{
	CTransform other(*this);
	other.Inverse();
	return other;
}

CVector3D CTransform::operator*(const CVector3D& vector) const
{
	/*CMatrix33 mat;
	m_Rotation.GetRotation(&mat);*/

	return m_Rotation * vector + m_Translation;
}

CTransform CTransform::operator*(const CTransform& transform) const
{
	/*CMatrix33 mat;
	m_Rotation.GetRotation(&mat);*/

	return CTransform(m_Rotation * transform.GetTranslation() + m_Translation, m_Rotation * transform.GetRotation());
}

CTransform& CTransform::operator=(const CTransform& other)
{
	if ( this == &other )
		return (*this);

	m_Translation = other.m_Translation;
	m_Rotation = other.m_Rotation;

	return (*this);
}

