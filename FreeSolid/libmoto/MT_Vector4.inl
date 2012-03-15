#include "GEN_Optimize.h"

GEN_INLINE MT_Vector4& MT_Vector4::operator+=(const MT_Vector4& v) {
    m_co[0] += v[0]; m_co[1] += v[1]; m_co[2] += v[2]; m_co[3] += v[3];
    return *this;
}

GEN_INLINE MT_Vector4& MT_Vector4::operator-=(const MT_Vector4& v) {
    m_co[0] -= v[0]; m_co[1] -= v[1]; m_co[2] -= v[2]; m_co[3] -= v[3];
    return *this;
}
 
GEN_INLINE MT_Vector4& MT_Vector4::operator*=(MT_Scalar s) {
    m_co[0] *= s; m_co[1] *= s; m_co[2] *= s; m_co[3] *= s;
    return *this;
}

GEN_INLINE MT_Vector4& MT_Vector4::operator/=(MT_Scalar s) {
    assert(s != MT_Scalar(0.0));
    return *this *= MT_Scalar(1.0) / s;
}

GEN_INLINE MT_Vector4 operator+(const MT_Vector4& v1, const MT_Vector4& v2) {
    return MT_Vector4(v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2], v1[3] + v2[3]);
}

GEN_INLINE MT_Vector4 operator-(const MT_Vector4& v1, const MT_Vector4& v2) {
    return MT_Vector4(v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]);
}

GEN_INLINE MT_Vector4 operator-(const MT_Vector4& v) {
    return MT_Vector4(-v[0], -v[1], -v[2], -v[3]);
}

GEN_INLINE MT_Vector4 operator*(const MT_Vector4& v, MT_Scalar s) {
    return MT_Vector4(v[0] * s, v[1] * s, v[2] * s, v[3] * s);
}

GEN_INLINE MT_Vector4 operator*(MT_Scalar s, const MT_Vector4& v) { return v * s; }

GEN_INLINE MT_Vector4 operator/(const MT_Vector4& v, MT_Scalar s) {
    assert(s != MT_Scalar(0.0));
    return v * (MT_Scalar(1.0) / s);
}

GEN_INLINE MT_Scalar MT_Vector4::dot(const MT_Vector4& v) const {
    return m_co[0] * v[0] + m_co[1] * v[1] + m_co[2] * v[2] + m_co[3] * v[3];
}

GEN_INLINE MT_Scalar MT_Vector4::length2() const { return dot(*this); }
GEN_INLINE MT_Scalar MT_Vector4::length() const { return MT_sqrt(length2()); }

GEN_INLINE void MT_Vector4::normalize() { *this /= length(); }
GEN_INLINE MT_Vector4 MT_Vector4::normalized() const { return *this / length(); }

GEN_INLINE MT_Scalar MT_Vector4::angle(const MT_Vector4& v) const {
    MT_Scalar s = MT_sqrt(length2() * v.length2());
    assert(s != MT_Scalar(0.0));
    return acos(dot(v) / s);
}

GEN_INLINE MT_Vector4 MT_Vector4::absolute() const {
    return MT_Vector4(MT_abs(m_co[0]), MT_abs(m_co[1]), MT_abs(m_co[2]), MT_abs(m_co[3]));
}

GEN_INLINE void MT_Vector4::scale(MT_Scalar x, MT_Scalar y, MT_Scalar z, MT_Scalar w) {
    m_co[0] *= x; m_co[1] *= y; m_co[2] *= z; m_co[3] *= w;
}

GEN_INLINE MT_Vector4 MT_Vector4::scaled(MT_Scalar x, MT_Scalar y, MT_Scalar z, MT_Scalar w) const {
    return MT_Vector4(m_co[0] * x, m_co[1] * y, m_co[2] * z, m_co[3] * w);
}

GEN_INLINE MT_Scalar  MT_dot(const MT_Vector4& v1, const MT_Vector4& v2) { return v1.dot(v2); }

GEN_INLINE MT_Scalar  MT_length2(const MT_Vector4& v) { return v.length2(); }
GEN_INLINE MT_Scalar  MT_length(const MT_Vector4& v) { return v.length(); }

GEN_INLINE MT_Scalar  MT_angle(const MT_Vector4& v1, const MT_Vector4& v2) { return v1.angle(v2); }
