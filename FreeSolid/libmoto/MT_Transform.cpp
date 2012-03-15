/*
 * Copyright (c) 2001 Dtecta <gino@dtecta.com>
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Dtecta makes no representations about
 * the suitability of this software for any purpose.  It is provided 
 * "as is" without express or implied warranty.
 */

#include "MT_Transform.h"

static const float GL_IDENTITY[] = {
	1.0f, 0.0f, 0.0f, 0.0f, 
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};


const MT_Transform MT_Transform::IDENTITY(GL_IDENTITY);

void MT_Transform::setValue(const float *m) {
    m_basis.setValue(m);
    m_origin.setValue(&m[12]);
    m_type = AFFINE;
}

// Saved for later
/*void MT_Transform::setValue(const double *m) {
    m_basis.setValue(m);
    m_origin.setValue(&m[12]);
    m_type = AFFINE;
}
*/
void MT_Transform::getValue(float *m) const {
    m_basis.getValue(m);
    m_origin.getValue(&m[12]);
    m[15] = 1.0f;
}

void MT_Transform::getValue(double *m) const {
    m_basis.getValue(m);
    m_origin.getValue(&m[12]);
    m[15] = 1.0;
}

MT_Transform& MT_Transform::operator*=(const MT_Transform& t) {
    m_origin += m_basis * t.m_origin;
    m_basis *= t.m_basis;
    m_type |= t.m_type; 
    return *this;
}

void MT_Transform::translate(const MT_Vector3& v) { 
    m_origin += m_basis * v; 
    m_type |= TRANSLATION;
}

void MT_Transform::rotate(const MT_Quaternion& q) { 
    m_basis *= MT_Matrix3x3(q); 
    m_type |= ROTATION; 
}

void MT_Transform::scale(MT_Scalar x, MT_Scalar y, MT_Scalar z) { 
    m_basis.scale(x, y, z);  
    m_type |= SCALING;
}

void MT_Transform::setIdentity() {
    m_basis.setIdentity();
    m_origin.setValue(MT_Scalar(0.0), MT_Scalar(0.0), MT_Scalar(0.0));
    m_type = 0x0;
}

void MT_Transform::invert(const MT_Transform& t) {
    m_basis = t.m_type & SCALING ? 
		t.m_basis.inverse() : 
		t.m_basis.transposed();
    m_origin.setValue(-MT_dot(m_basis[0], t.m_origin), 
                      -MT_dot(m_basis[1], t.m_origin), 
                      -MT_dot(m_basis[2], t.m_origin));  
    m_type = t.m_type;
}

void MT_Transform::mult(const MT_Transform& t1, const MT_Transform& t2) {
    m_basis = t1.m_basis * t2.m_basis;
    m_origin = t1(t2.m_origin);
    m_type = t1.m_type | t2.m_type;
}

void MT_Transform::multInverseLeft(const MT_Transform& t1, const MT_Transform& t2) {
    MT_Vector3 v = t2.m_origin - t1.m_origin;
    if (t1.m_type & SCALING) {
        MT_Matrix3x3 inv = t1.m_basis.inverse();
        m_basis = inv * t2.m_basis;
        m_origin = inv * v;
    }
    else {
        m_basis = MT_multTransposeLeft(t1.m_basis, t2.m_basis);
        m_origin = v * t1.m_basis;
    }
    m_type = t1.m_type | t2.m_type;
}



