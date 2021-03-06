#pragma once

#include <vector>
#include "Vector3D.h"

typedef unsigned int Bits;

class CGJKSimplex
{
public:
	CGJKSimplex(void);
	~CGJKSimplex(void);

private:
    CVector3D m_Points[4];			  
    float m_LengthSqr[4];			// m_LengthSqr[i] = m_Points[i].LengthSqr()
    float m_MaxLengthSqr;			// Maximum value among m_LengthSqr[i]
    CVector3D m_SuppPointsA[4];		  
    CVector3D m_SuppPointsB[4];		  
    CVector3D m_DiffLength[4][4];	// m_DiffLength[i][j] = m_Points[i] - m_Points[j]
    float m_DiffLengthSqr[4][4];
	float m_Det[16][4];			// Determinant 
 
	Bits m_CurBits; 
	unsigned int m_LastFound; // indicates a bit location found last time. It is between 0 to 3. 
	Bits m_LastBit; // m_LastBit = 1 << m_LastFound
	Bits m_AllBits; // m_AllBits = m_CurBits | m_LastBit

    bool IsSubset(Bits a, Bits b) const;                        
    bool IsValidSubset(Bits subset) const;                      
    void UpdateDiffLengths();                                   
    void CalcDeterminants();                                 
    CVector3D computeClosestPointForSubset(Bits subset);        

public:
	bool IsFull() const;                                                                          
    bool isEmpty() const;                                                                         
    int GetPoints(std::vector<CVector3D>& suppPointsA, std::vector<CVector3D>& suppPointsB, std::vector<CVector3D>& points) const;  
    float MaxLengthSqr() const;                                                      
    void AddPoint(const CVector3D& point, const CVector3D& suppPointA, const CVector3D& suppPointB);  
    bool IsDegenerate(const CVector3D& point) const;                                             
    bool IsAffinelyIndependent() const;                                                              
    void ClosestPointAandB(CVector3D& pA, CVector3D& pB) const;                             
    bool RunJohnsonAlgorithm(CVector3D& v);                                                 
};


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

// Return true if the simplex is empty
inline bool CGJKSimplex::isEmpty() const 
{
    return (m_CurBits == 0x0);
}

// Return the maximum squared length of a point
inline float CGJKSimplex::MaxLengthSqr() const 
{
    return m_MaxLengthSqr;
}