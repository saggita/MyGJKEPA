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

#ifndef MT_MATRIX3X3_H
#define MT_MATRIX3X3_H

#include <assert.h>

#include "MT_Vector3.h"
#include "MT_Quaternion.h"

// Row-major 3x3 matrix

class MT_Matrix3x3 {
public:
	static const MT_Matrix3x3 IDENTITY;

    MT_Matrix3x3() {}
    explicit MT_Matrix3x3(const float *m) { setValue(m); }
// Saved for later
//    explicit MT_Matrix3x3(const double *m) { setValue(m); }
    explicit MT_Matrix3x3(const MT_Quaternion& q) { setRotation(q); }
    
	MT_Matrix3x3(const MT_Quaternion& q, const MT_Vector3& s) { 
		setRotation(q); 
		scale(s[0], s[1], s[2]);
	}
	
	explicit MT_Matrix3x3(const MT_Vector3& euler) { setEuler(euler); }
	MT_Matrix3x3(const MT_Vector3& euler, const MT_Vector3& s) { 
		setEuler(euler); 
		scale(s[0], s[1], s[2]);
	}
	
    MT_Matrix3x3(MT_Scalar xx, MT_Scalar xy, MT_Scalar xz,
                 MT_Scalar yx, MT_Scalar yy, MT_Scalar yz,
                 MT_Scalar zx, MT_Scalar zy, MT_Scalar zz) { 
        setValue(xx, xy, xz, 
                 yx, yy, yz, 
                 zx, zy, zz);
    }
    
    MT_Vector3&       operator[](int i)       { return m_el[i]; }
    const MT_Vector3& operator[](int i) const { return m_el[i]; }
    
    void setValue(const float *m) {
        m_el[0][0] = *m++; m_el[1][0] = *m++; m_el[2][0] = *m++; m++;
        m_el[0][1] = *m++; m_el[1][1] = *m++; m_el[2][1] = *m++; m++;
        m_el[0][2] = *m++; m_el[1][2] = *m++; m_el[2][2] = *m;
    }

// Saved for later
/*    void setValue(const double *m) {
        m_el[0][0] = *m++; m_el[1][0] = *m++; m_el[2][0] = *m++; m++;
        m_el[0][1] = *m++; m_el[1][1] = *m++; m_el[2][1] = *m++; m++;
        m_el[0][2] = *m++; m_el[1][2] = *m++; m_el[2][2] = *m;
    }
*/
    void setValue(MT_Scalar xx, MT_Scalar xy, MT_Scalar xz, 
                  MT_Scalar yx, MT_Scalar yy, MT_Scalar yz, 
                  MT_Scalar zx, MT_Scalar zy, MT_Scalar zz) {
        m_el[0][0] = xx; m_el[0][1] = xy; m_el[0][2] = xz;
        m_el[1][0] = yx; m_el[1][1] = yy; m_el[1][2] = yz;
        m_el[2][0] = zx; m_el[2][1] = zy; m_el[2][2] = zz;
    }
  
    void setRotation(const MT_Quaternion& q) {
        MT_Scalar d = q.length2();
        assert(d != MT_Scalar(0.0));
        MT_Scalar s = MT_Scalar(2.0) / d;
        MT_Scalar xs = q[0] * s,   ys = q[1] * s,   zs = q[2] * s;
        MT_Scalar wx = q[3] * xs,  wy = q[3] * ys,  wz = q[3] * zs;
        MT_Scalar xx = q[0] * xs,  xy = q[0] * ys,  xz = q[0] * zs;
        MT_Scalar yy = q[1] * ys,  yz = q[1] * zs,  zz = q[2] * zs;
        setValue(MT_Scalar(1.0) - (yy + zz), xy - wz        ,         xz + wy,
                 xy + wz        , MT_Scalar(1.0) - (xx + zz),         yz - wx,
                 xz - wy        , yz + wx,         MT_Scalar(1.0) - (xx + yy));
    }
    
	void setEuler(const MT_Vector3& euler) {
		MT_Scalar ci = cos(euler[0]); 
		MT_Scalar cj = cos(euler[1]); 
		MT_Scalar ch = cos(euler[2]);
		MT_Scalar si = sin(euler[0]); 
		MT_Scalar sj = sin(euler[1]); 
		MT_Scalar sh = sin(euler[2]);
		MT_Scalar cc = ci * ch; 
		MT_Scalar cs = ci * sh; 
		MT_Scalar sc = si * ch; 
		MT_Scalar ss = si * sh;
		
		setValue(cj * ch, sj * sc - cs, sj * cc + ss,
				 cj * sh, sj * ss + cc, sj * cs - sc, 
	       			 -sj,      cj * si,      cj * ci);
	}

    void scale(MT_Scalar x, MT_Scalar y, MT_Scalar z) {
        m_el[0][0] *= x; m_el[0][1] *= y; m_el[0][2] *= z;
        m_el[1][0] *= x; m_el[1][1] *= y; m_el[1][2] *= z;
        m_el[2][0] *= x; m_el[2][1] *= y; m_el[2][2] *= z;
    }

    MT_Matrix3x3 scaled(MT_Scalar x, MT_Scalar y, MT_Scalar z) const {
        return MT_Matrix3x3(m_el[0][0] * x, m_el[0][1] * y, m_el[0][2] * z,
                            m_el[1][0] * x, m_el[1][1] * y, m_el[1][2] * z,
                            m_el[2][0] * x, m_el[2][1] * y, m_el[2][2] * z);
    }

	MT_Vector3 getScaling() const {
		return MT_Vector3(m_el[0][0] * m_el[0][0] + m_el[1][0] * m_el[1][0] + m_el[2][0] * m_el[2][0],
						  m_el[0][1] * m_el[0][1] + m_el[1][1] * m_el[1][1] + m_el[2][1] * m_el[2][1],
						  m_el[0][2] * m_el[0][2] + m_el[1][2] * m_el[1][2]	+ m_el[2][2] * m_el[2][2]);
	}
	
    
    void setIdentity() { 
        setValue(MT_Scalar(1.0), MT_Scalar(0.0), MT_Scalar(0.0), 
                 MT_Scalar(0.0), MT_Scalar(1.0), MT_Scalar(0.0), 
                 MT_Scalar(0.0), MT_Scalar(0.0), MT_Scalar(1.0)); 
    }
    
    void getValue(float *m) const {
        *m++ = m_el[0][0]; *m++ = m_el[1][0]; *m++ = m_el[2][0]; *m++ = 0.0;
        *m++ = m_el[0][1]; *m++ = m_el[1][1]; *m++ = m_el[2][1]; *m++ = 0.0;
        *m++ = m_el[0][2]; *m++ = m_el[1][2]; *m++ = m_el[2][2]; *m   = 0.0;
    }

    void getValue(double *m) const {
        *m++ = m_el[0][0]; *m++ = m_el[1][0]; *m++ = m_el[2][0]; *m++ = 0.0;
        *m++ = m_el[0][1]; *m++ = m_el[1][1]; *m++ = m_el[2][1]; *m++ = 0.0;
        *m++ = m_el[0][2]; *m++ = m_el[1][2]; *m++ = m_el[2][2]; *m   = 0.0;
    }

    MT_Quaternion getRotation() const;

    void getEuler(MT_Scalar& yaw, MT_Scalar& pitch, MT_Scalar& roll) const;

	MT_Matrix3x3& operator*=(const MT_Matrix3x3& m); 

    MT_Scalar tdot(int c, const MT_Vector3& v) const {
        return m_el[0][c] * v[0] + m_el[1][c] * v[1] + m_el[2][c] * v[2];
    }
  
    MT_Scalar cofac(int r1, int c1, int r2, int c2) const {
        return m_el[r1][c1] * m_el[r2][c2] - m_el[r1][c2] * m_el[r2][c1];
    }

    MT_Scalar    determinant() const;
	MT_Matrix3x3 adjoint() const;

    MT_Matrix3x3 absolute() const;

    MT_Matrix3x3 transposed() const;
    void         transpose();

    MT_Matrix3x3 inverse() const; 
	void         invert();
  
protected:

    MT_Vector3 m_el[3];
};

MT_Vector3   operator*(const MT_Matrix3x3& m, const MT_Vector3& v);
MT_Vector3   operator*(const MT_Vector3& v, const MT_Matrix3x3& m);
MT_Matrix3x3 operator*(const MT_Matrix3x3& m1, const MT_Matrix3x3& m2);

MT_Matrix3x3 MT_multTransposeLeft(const MT_Matrix3x3& m1, const MT_Matrix3x3& m2);
MT_Matrix3x3 MT_multTransposeRight(const MT_Matrix3x3& m1, const MT_Matrix3x3& m2);

inline GEN_OStream& operator<<(GEN_OStream& os, const MT_Matrix3x3& m) {
    return os << m[0] << GEN_endl << m[1] << GEN_endl << m[2] << GEN_endl;
}

#ifdef GEN_INLINED
#include "MT_Matrix3x3.inl"
#endif

#endif
