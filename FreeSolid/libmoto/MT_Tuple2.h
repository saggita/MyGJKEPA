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

#ifndef MT_Tuple2_H
#define MT_Tuple2_H

#include "GEN_Stream.h"
#include "MT_Scalar.h"

class MT_Tuple2 {
public:
    MT_Tuple2() {}
    explicit MT_Tuple2(const float *v) { setValue(v); }
  // Saved for later
  //    explicit MT_Tuple2(const double *v) { setValue(v); }
    MT_Tuple2(MT_Scalar x, MT_Scalar y) { setValue(x, y); }
    
    MT_Scalar&       operator[](int i)       { return m_co[i]; }
    const MT_Scalar& operator[](int i) const { return m_co[i]; }
    
    MT_Scalar&       x()       { return m_co[0]; } 
    const MT_Scalar& x() const { return m_co[0]; } 

    MT_Scalar&       y()       { return m_co[1]; }
    const MT_Scalar& y() const { return m_co[1]; } 

	MT_Scalar&       u()       { return m_co[0]; } 
    const MT_Scalar& u() const { return m_co[0]; } 

    MT_Scalar&       v()       { return m_co[1]; }
    const MT_Scalar& v() const { return m_co[1]; } 

    operator MT_Scalar       *()       { return m_co; }
    operator const MT_Scalar *() const { return m_co; }

    void getValue(float *v) const { 
        v[0] = m_co[0]; v[1] = m_co[1];
    }
    
    void getValue(double *v) const { 
        v[0] = m_co[0]; v[1] = m_co[1];
    }
    
    void setValue(const float *v) {
        m_co[0] = v[0]; m_co[1] = v[1];
    }
    
  // Saved for later
  /*
    void setValue(const double *v) {
        m_co[0] = v[0]; m_co[1] = v[1];
    }
  */
    void setValue(MT_Scalar x, MT_Scalar y) {
        m_co[0] = x; m_co[1] = y; 
    }
    
protected:
    MT_Scalar m_co[2];                            
};

inline bool operator==(const MT_Tuple2& t1, const MT_Tuple2& t2) {
    return t1[0] == t2[0] && t1[1] == t2[1];
}

inline GEN_OStream& operator<<(GEN_OStream& os, const MT_Tuple2& t) {
    return os << t[0] << ' ' << t[1];
}

#endif
