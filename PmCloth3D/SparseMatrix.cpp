#include <cassert>
#include "SparseMatrix.h"
//
//template<class T>
//CSparseMatrix<T>::CSparseMatrix(void) : m_N(0)
//{
//}
//
//template<class T>
//CSparseMatrix<T>::~CSparseMatrix(void)
//{
//}
//
//template<class T>
//const T& CSparseMatrix<T>::operator()(unsigned int i, unsigned int j) const
//{
//	return GetElement(i, j);
//}
//
//template<class T>
//T& CSparseMatrix<T>::operator()(unsigned int i, unsigned int j)
//{
//	return GetElement(i, j);
//}

void Multiply(const CSparseMatrix<double>& A, const CSparseMatrix<double>& B, CSparseMatrix<double>& result)
{
	if ( A.bSquare )
		assert(A.Size() == B.Size());
	else
		assert(A.m_M == B.m_N);

	result.Create(A.m_N);

	for ( unsigned int i = 0; i < A.m_N; i++ )
	{
		for ( unsigned int j = 0; j < B.m_N; j++ )
		{
			double sum = 0;

			for ( unsigned int p = 0; p < A.m_Indexes[i].size(); p++ )
			{
				double valThis = A.GetElement(i, A.m_Indexes[i][p]);				
				double valOther = B.GetElement(A.m_Indexes[i][p], j);
				sum += valThis * valOther;
			}

			if ( sum != 0 )
				result.SetElement(i, j, sum);
		}
	}
}