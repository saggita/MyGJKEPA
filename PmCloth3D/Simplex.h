#pragma once

#include "Vector3D.h"

typedef unsigned int Bits;

class CGJKSimplex
{
public:
	CGJKSimplex(void);
	~CGJKSimplex(void);

private:
    CVector3D m_Points[4];			  
    double m_LengthSqr[4];			// m_LengthSqr[i] = m_Points[i].LengthSqr()
    double m_MaxLengthSqr;			// Maximum value among m_LengthSqr[i]
    CVector3D m_SuppPointsA[4];		  
    CVector3D m_SuppPointsB[4];		  
    CVector3D m_DiffLength[4][4];	// diffLength[i][j] = m_Points[i] - m_Points[j]
    double m_DiffLengthSqr[4][4];
	double m_Det[16][4];			// determinant 
 
	Bits m_CurBits; // Identifies current simplex
	unsigned int m_LastFound; // Identifies last found slot for support point. It should be between 0 and 3.
	Bits m_LastBit; // m_LastBit = 1 << m_LastFound
	Bits m_AllBits; // m_AllBits = m_CurBits | m_LastBit

    bool overlap(Bits a, Bits b) const;                         
    bool IsSubset(Bits a, Bits b) const;                        
    bool IsValidSubset(Bits subset) const;                      
    void UpdateDiffLengths();                                   
    void CalcDeterminants();                                 
    CVector3D computeClosestPointForSubset(Bits subset);        

public:
	bool IsFull() const;                                                                            // Return true if the simplex contains 4 points
    bool isEmpty() const;                                                                           // Return true if the simple is empty
    unsigned int getSimplex(CVector3D* suppPointsA, CVector3D* suppPointsB, CVector3D* points) const;  // Return the points of the simplex
    double MaxLengthSqr() const;                                                      // Return the maximum squared length of a point
    void AddPoint(const CVector3D& point, const CVector3D& suppPointA, const CVector3D& suppPointB);   // Addd a point to the simplex
    bool IsDegenerate(const CVector3D& point) const;                                             
    bool IsAffinelyIndependent() const;                                                               // Return true if the set is affinely dependent
    void ClosestPointAandB(CVector3D& pA, CVector3D& pB) const;                             // Compute the closest points of object A and B
    bool RunJohnsonAlgorithm(CVector3D& v);                                                          // Compute the closest point to the origin of the current simplex
};

// Return true if some bits of "a" overlap with bits of "b"
inline bool CGJKSimplex::overlap(Bits a, Bits b) const 
{
    return ((a & b) != 0x0);
}

// Return true if the bits of "b" is a subset of the bits of "a"
inline bool CGJKSimplex::IsSubset(Bits containerSet, Bits subSet) const 
{
    return ((containerSet & subSet) == subSet);
}

// Return true if the simplex contains 4 points
inline bool CGJKSimplex::IsFull() const 
{
    return (m_CurBits == 0xf);
}

// Return true if the simple is empty
inline bool CGJKSimplex::isEmpty() const 
{
    return (m_CurBits == 0x0);
}

// Return the maximum squared length of a point
inline double CGJKSimplex::MaxLengthSqr() const 
{
    return m_MaxLengthSqr;
}