#include <cfloat>
#include <cassert>
#include "Simplex.h"

// Constructor
CGJKSimplex::CGJKSimplex() : m_CurBits(0x0), m_AllBits(0x0) 
{

}

// Destructor
CGJKSimplex::~CGJKSimplex() 
{

}

void CGJKSimplex::AddPoint(const CVector3D& point, const CVector3D& suppPointA, const CVector3D& suppPointB) 
{
    assert(!IsFull());

    m_LastFound = 0;
    m_LastBit = 0x1;

    // Look for the bit corresponding to one of the four point that is not in
    // the current simplex
    while ( overlap(m_CurBits, m_LastBit) ) 
	{
        m_LastFound++;
        m_LastBit <<= 1;
    }

    assert(m_LastFound >= 0 && m_LastFound < 4);

    // Add the point into the simplex
    m_Points[m_LastFound] = point;
	m_LengthSqr[m_LastFound] = point.LengthSqr();
    m_AllBits = m_CurBits | m_LastBit;

    UpdateDiffLengths();    
    CalcDeterminants();
    
    // Add the support points of objects A and B
    m_SuppPointsA[m_LastFound] = suppPointA;
    m_SuppPointsB[m_LastFound] = suppPointB;
}

bool CGJKSimplex::IsDegenerate(const CVector3D& point) const 
{
	Bits bit;

    for ( int i = 0, bit = 0x1; i < 4; i++, bit <<= 1 ) 
	{
        if ( overlap(m_AllBits, bit) && point == m_Points[i] ) 
			return true;
    }

    return false;
}

void CGJKSimplex::UpdateDiffLengths() 
{
    int i;
    Bits bit;

    for ( i=0, bit = 0x1; i < 4; i++, bit <<= 1 ) 
	{
        if ( overlap(m_CurBits, bit) ) 
		{
            m_DiffLength[i][m_LastFound] = m_Points[i] - m_Points[m_LastFound];
            m_DiffLength[m_LastFound][i] = -m_DiffLength[i][m_LastFound];
			m_DiffLengthSqr[i][m_LastFound] = m_DiffLengthSqr[m_LastFound][i] = m_DiffLength[i][m_LastFound].LengthSqr();
        }
    }
}

int CGJKSimplex::GetPoints(std::vector<CVector3D>& suppPointsA, std::vector<CVector3D>& suppPointsB, std::vector<CVector3D>& points) const 
{
	assert(suppPointsA.size() == 0 );
	assert(suppPointsB.size() == 0 );
	assert(points.size() == 0 );

	int count = 0;
	int i;
	Bits bit;

	for ( i = 0, bit = 0x1; i < 4; i++, bit <<= 1 ) 
	{
		if ( overlap(m_CurBits, bit) ) 
		{
			suppPointsA.push_back(m_SuppPointsA[count]);
			suppPointsB.push_back(m_SuppPointsB[count]);
			points.push_back(m_Points[count]);
			count++;
		}
	}

	return count;
}

void CGJKSimplex::CalcDeterminants() 
{
    m_Det[m_LastBit][m_LastFound] = 1.0;

	if ( isEmpty() )
		return;

    Bits bitI;
	Bits bitJ;

    for ( int i = 0, bitI = 0x1; i < 4; i++, bitI <<= 1 ) 
	{
        if ( overlap(m_CurBits, bitI) ) 
		{
            Bits bit2 = bitI | m_LastBit;

			m_Det[bit2][i] = m_DiffLength[m_LastFound][i].Dot(m_Points[m_LastFound]);
            m_Det[bit2][m_LastFound] = m_DiffLength[i][m_LastFound].Dot(m_Points[i]);

            for ( int j=0, bitJ = 0x1; j<i; j++, bitJ <<= 1 ) 
			{
                if ( overlap(m_CurBits, bitJ) ) 
				{
                    int k;
                    Bits bit3 = bitJ | bit2;

                    k = m_DiffLengthSqr[i][j] < m_DiffLengthSqr[m_LastFound][j] ? i : m_LastFound;
                    m_Det[bit3][j] = m_Det[bit2][i] * m_DiffLength[k][j].Dot(m_Points[i]) +
                                    m_Det[bit2][m_LastFound] * m_DiffLength[k][j].Dot(m_Points[m_LastFound]);

                    k = m_DiffLengthSqr[j][i] < m_DiffLengthSqr[m_LastFound][i] ? j : m_LastFound;
                    m_Det[bit3][i] = m_Det[bitJ | m_LastBit][j] * m_DiffLength[k][i].Dot(m_Points[j]) +
                                    m_Det[bitJ | m_LastBit][m_LastFound] * m_DiffLength[k][i].Dot(m_Points[m_LastFound]);

                    k = m_DiffLengthSqr[i][m_LastFound] < m_DiffLengthSqr[j][m_LastFound] ? i : j;
                    m_Det[bit3][m_LastFound] = m_Det[bitJ | bitI][j] * m_DiffLength[k][m_LastFound].Dot(m_Points[j]) +
                                            m_Det[bitJ | bitI][i] * m_DiffLength[k][m_LastFound].Dot(m_Points[i]);
                }
            }
        }
    }

    if ( m_AllBits == 0xf ) 
	{
        int k;

        k = m_DiffLengthSqr[1][0] < m_DiffLengthSqr[2][0] ? (m_DiffLengthSqr[1][0] < m_DiffLengthSqr[3][0] ? 1 : 3) : (m_DiffLengthSqr[2][0] < m_DiffLengthSqr[3][0] ? 2 : 3);
        m_Det[0xf][0] = m_Det[0xe][1] * m_DiffLength[k][0].Dot(m_Points[1]) +
                    m_Det[0xe][2] * m_DiffLength[k][0].Dot(m_Points[2]) +
                    m_Det[0xe][3] * m_DiffLength[k][0].Dot(m_Points[3]);

        k = m_DiffLengthSqr[0][1] < m_DiffLengthSqr[2][1] ? (m_DiffLengthSqr[0][1] < m_DiffLengthSqr[3][1] ? 0 : 3) : (m_DiffLengthSqr[2][1] < m_DiffLengthSqr[3][1] ? 2 : 3);
        m_Det[0xf][1] = m_Det[0xd][0] * m_DiffLength[k][1].Dot(m_Points[0]) +
                    m_Det[0xd][2] * m_DiffLength[k][1].Dot(m_Points[2]) +
                    m_Det[0xd][3] * m_DiffLength[k][1].Dot(m_Points[3]);

        k = m_DiffLengthSqr[0][2] < m_DiffLengthSqr[1][2] ? (m_DiffLengthSqr[0][2] < m_DiffLengthSqr[3][2] ? 0 : 3) : (m_DiffLengthSqr[1][2] < m_DiffLengthSqr[3][2] ? 1 : 3);
        m_Det[0xf][2] = m_Det[0xb][0] * m_DiffLength[k][2].Dot(m_Points[0]) +
                    m_Det[0xb][1] * m_DiffLength[k][2].Dot(m_Points[1]) +
                    m_Det[0xb][3] * m_DiffLength[k][2].Dot(m_Points[3]);

        k = m_DiffLengthSqr[0][3] < m_DiffLengthSqr[1][3] ? (m_DiffLengthSqr[0][3] < m_DiffLengthSqr[2][3] ? 0 : 2) : (m_DiffLengthSqr[1][3] < m_DiffLengthSqr[2][3] ? 1 : 2);
        m_Det[0xf][3] = m_Det[0x7][0] * m_DiffLength[k][3].Dot(m_Points[0]) +
                    m_Det[0x7][1] * m_DiffLength[k][3].Dot(m_Points[1]) +
                    m_Det[0x7][2] * m_DiffLength[k][3].Dot(m_Points[2]);
    }
}

bool CGJKSimplex::IsAffinelyIndependent() const 
{
    double sum = 0.0;
    int i;
    Bits bit;

    for ( i=0, bit = 0x1; i < 4; i++, bit <<= 1 ) 
	{
        if ( overlap(m_AllBits, bit) ) 
		{
            sum += m_Det[m_AllBits][i];
        }
    }

    return (sum > 0.0);
}

// To be a valid subset, all m_Det[subset][i] (i is part of 'subset') should be > 0 and 
// all other m_Det[subset][j] (j is not part of 'subset') should be <= 0. 
bool CGJKSimplex::IsValidSubset(Bits subset) const 
{
    int i;
    Bits bit;

    for ( i = 0, bit = 0x1; i < 4; i++, bit <<= 1 ) 
	{
        if ( overlap(m_AllBits, bit) ) 
		{
            if ( overlap(subset, bit) ) 
			{
                if ( m_Det[subset][i] <= 0.0 )
                    return false;
            }
            else if ( m_Det[subset | bit][i] > 0.0 ) 
			{
                return false;
            }
        }
    }

    return true;
}

void CGJKSimplex::ClosestPointAandB(CVector3D& pA, CVector3D& pB) const {
    double sum = 0.0;
    pA.Set(0.0, 0.0, 0.0);
    pB.Set(0.0, 0.0, 0.0);
    int i;
    Bits bit;

    for ( i=0, bit=0x1; i<4; i++, bit <<= 1 ) 
	{
        if ( overlap(m_CurBits, bit) ) 
		{
            sum += m_Det[m_CurBits][i];
            pA += m_Det[m_CurBits][i] * m_SuppPointsA[i];
            pB += m_Det[m_CurBits][i] * m_SuppPointsB[i];
        }
    }

    assert(sum > 0.0);
    double factor = 1.0 / sum;
    pA *= factor;
    pB *= factor;
}

// Run Johnson's Algorithm and compute 'v' which is a closest point to the origin in this simplex. 
// If this function succeeds, returns true. Otherwise, returns false.
bool CGJKSimplex::RunJohnsonAlgorithm(CVector3D& v) 
{
    Bits subset;

	// Iterates all possible sub sets
    for ( subset = m_CurBits; subset != 0x0; subset-- ) 
	{
        if ( IsSubset(m_CurBits, subset) && IsValidSubset(subset | m_LastBit) ) 
		{
			m_CurBits = subset | m_LastBit;   

			v.Set(0, 0, 0);
			m_MaxLengthSqr = 0.0;
			double sum = 0.0;
			Bits bit;

			for ( int i=0, bit = 0x1; i < 4; i++, bit <<= 1 ) 
			{
				if ( overlap(m_CurBits, bit) ) 
				{
					sum += m_Det[m_CurBits][i];
					v += m_Det[m_CurBits][i] * m_Points[i];

					if ( m_MaxLengthSqr < m_LengthSqr[i] ) 
						m_MaxLengthSqr = m_LengthSqr[i];
				}
			}

			assert(sum > 0.0);
			 
			v = v / sum;
			return true;
        }
    }

    if ( IsValidSubset(m_LastBit) ) 
	{
        m_CurBits = m_LastBit;                  
        m_MaxLengthSqr = m_LengthSqr[m_LastFound];
        v = m_Points[m_LastFound];                
        return true;
    }

	// Original GJK uses the backup procedure here.

    return false;
}

// Return the closest point "v" in the convex hull of the points in the subset
// represented by the bits "subset"
CVector3D CGJKSimplex::computeClosestPointForSubset(Bits subset) 
{
    CVector3D v(0.0, 0.0, 0.0);      // Closet point v = sum(lambda_i * points[i])
    m_MaxLengthSqr = 0.0;
    double sum = 0.0;
    int i;
    Bits bit;

    for (i=0, bit=0x1; i<4; i++, bit <<= 1) 
	{
        // If the current point is in the subset
        if (overlap(subset, bit)) 
		{
            // deltaX = sum of all m_Det[subset][i]
            sum += m_Det[subset][i];

            if ( m_MaxLengthSqr < m_LengthSqr[i] ) 
                m_MaxLengthSqr = m_LengthSqr[i];

            // Closest point v = sum(lambda_i * points[i])
            v += m_Det[subset][i] * m_Points[i];
        }
    }

    assert(sum > 0.0);

    return (v / sum);
}