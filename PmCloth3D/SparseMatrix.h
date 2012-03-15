#pragma once

#include <omp.h>
#include <vector>

template<class T>
class CColumnVector
{
public:
	CColumnVector(void) { }
	~CColumnVector(void) { }
	  
private:
	std::vector<T> m_Values;

public:
	void Create(unsigned int n)
	{
		Clear();
		m_Values.resize(n);
	}
	
	void Create(unsigned int n, const T& val)
	{
		Clear();
		m_Values.resize(n, val);
	}

	unsigned int Size() const { return m_Values.size(); }

	void Clear()
	{
		m_Values.clear();
	}

	void ZeroWithoutResize()
	{
		m_Values.clear();
	}

	void Zero()
	{
		m_Values.resize(0);
	}

	const T& GetElement(unsigned int i) const
	{
		assert(0 <= i && i < Size());
		return m_Values[i];
	}

	T& GetElement(unsigned int i)
	{
		assert(0 <= i && i < Size());
		return m_Values[i];
	}

	void SetElement(unsigned int i, const T& value)
	{		
		assert(0 <= i && i < Size());
		m_Values[i] = value;
	}

	void SetAllElement(const T& value)
	{			
		unsigned int size = Size();

		for ( int i = 0; i < size; i++ )
			m_Values[i] = value;
	}

	void AddToElement(unsigned int i, const T& incrementValue)
	{
		assert(0 <= i && i < Size());
		m_Values[i] += value;
	}

	CColumnVector<T> operator*(const T& val)
	{
		CColumnVector<T> A;
		unsigned int size = Size();

		for ( int i = 0; i < size; i++ )
			A.SetElement(i, m_Values[i] * val);

		return A;
	}

	friend CColumnVector<T> operator*(const T& val, const CColumnVector<T>& other);

	const T& operator[](unsigned int i) const
	{
		return GetElement(i);
	}

	T& operator[](unsigned int i) 
	{
		return GetElement(i);
	}
};

template<class T>
CColumnVector<T> operator*(const T& val, const CColumnVector<T>& other)
{
	return other * val;
}

template<class T>
CColumnVector<T> operator*(double val, const CColumnVector<T>& other)
{
	CColumnVector<T> A;
	
	unsigned int size = other.Size();
	A.Create(size);

	for ( unsigned int i = 0; i < size; i++ )
		A.SetElement(i, other.GetElement(i) * val);

	return A;
}

template<class T>
class CDiagonalMatrix
{
public:
	CDiagonalMatrix(void) : m_N(0) { }

	explicit CDiagonalMatrix(unsigned int n)
	{
		Create(n);
	}

	~CDiagonalMatrix(void)
	{
	}
	  
private:
	unsigned int m_N; // square matrix with dimension of m_N * m_N
	std::vector<T> m_Values;

public:
	void Create(unsigned int n)
	{
		Clear();

		m_N = n;
		m_Values.resize(m_N);
	}

	unsigned int Size() const { return m_N; }

	void Clear()
	{
		m_N = 0;
		m_Values.clear();
	}

	void ZeroWithoutResize()
	{
		m_Values.clear();
	}

	void Zero()
	{
		m_Values.resize(0);
	}

	const T& GetElement(unsigned int i, unsigned int j) const
	{
		assert(i == j);
		assert(0 <= i && i < m_N);
		assert(0 <= j && j < m_N);

		return m_Values[i];
	}

	T& GetElement(unsigned int i, unsigned int j)
	{
		assert(i == j);
		assert(0 <= i && i < m_N);
		assert(0 <= j && j < m_N);

		return m_Values[i];
	}

	void SetElement(unsigned int i, unsigned int j, const T& value)
	{		
		assert(i == j);
		assert(0 <= i && i < m_N);
		assert(0 <= j && j < m_N);

		m_Values[i] = value;
	}

	void SetAllElement(const T& value)
	{		
		assert(m_Values.size() == m_N);

		for ( int i = 0; i < m_N; i++ )
			m_Values[i] = value;
	}

	void AddToElement(unsigned int i, unsigned int j, const T& incrementValue)
	{
		assert(i == j);
		assert(0 <= i && i < m_N);
		assert(0 <= j && j < m_N);

		m_Values[i] += value;
	}

	/*void Inverse()
	{
		assert(m_Values.size() == m_N);

		for ( int i = 0; i < m_N; i++ )
		{
			if ( m_Values[i] != 0 )
			m_Values[i] = 1.0 / m_Values[i];
		}
	}*/

	CDiagonalMatrix<T> operator*(T val)
	{
		CDiagonalMatrix<T> A;
			
		for ( int i = 0; i < m_N; i++ )
			A.SetElement(i, i, m_Values[i] * val);

		return A;
	}

	friend CDiagonalMatrix<T> operator*(T val, const CDiagonalMatrix<T>& other);
};

template<class T>
CDiagonalMatrix<T> operator*(T val, const CDiagonalMatrix<T>& other)
{
	return other * val;
}

template<class T>
class CSparseMatrix
{
public:
	CSparseMatrix(void) : m_N(0), m_M(0), m_NumOfNonzeroPerRow(0)
	{
	}

	explicit CSparseMatrix(unsigned int n, unsigned int numOfNonzeroPerRow = 0) : m_M(0)
	{
		Create(n, numOfNonzeroPerRow);
	}

	explicit CSparseMatrix(unsigned int n, unsigned int m, unsigned int numOfNonzeroPerRow)
	{
		Create(n, m, numOfNonzeroPerRow);
	}

	~CSparseMatrix(void)
	{
	}

	//friend template<class T1, class T2>
	//void Multiply(const CSparseMatrix<T1>& m, const std::vector<T2>& a, std::vector<T2>& result);

//protected:
	bool bSquare; 
	unsigned int m_N; // if this is square matrix, dimension is m_N * m_N. If non-square matrix, m_N * m_M
	unsigned int m_M;
	unsigned int m_NumOfNonzeroPerRow;
	std::vector<std::vector<unsigned int> > m_Indexes; // sorted
	std::vector<std::vector<T> > m_Values; 

public:
	// n x n square sparse matrix
	void Create(unsigned int n, unsigned int numOfNonzeroPerRow = 0)
	{
		Clear();

		bSquare = true;
		m_N = n;
		m_NumOfNonzeroPerRow = numOfNonzeroPerRow;

		m_Indexes.resize(n);
		m_Values.resize(n);

		for ( unsigned int i = 0; i < n; ++i )
		{
			m_Indexes[i].reserve(numOfNonzeroPerRow);
			m_Values[i].reserve(numOfNonzeroPerRow);
		}
	}

	// n x m non-square sparese matrix (n: num of rows, m: num of columns)
	void Create(unsigned int n, unsigned int m, unsigned int numOfNonzeroPerRow)
	{
		if ( n == m )
			return Create(n, numOfNonzeroPerRow);

		Clear();

		bSquare = false;
		m_N = n;
		m_M = m;
		m_NumOfNonzeroPerRow = numOfNonzeroPerRow;

		m_Indexes.resize(n);
		m_Values.resize(n);

		for ( unsigned int i = 0; i < n; ++i )
		{
			m_Indexes[i].reserve(numOfNonzeroPerRow);
			m_Values[i].reserve(numOfNonzeroPerRow);
		}
	}

	// TODO: need to chaning following squareness of matrix
	unsigned int Size() const { return m_N; }

	unsigned int NumOfRows() const { return m_N; }
	unsigned int NumOfColumns() const { return m_M; }
	
	T GetElement(unsigned int i, unsigned int j) const
	{
		for ( unsigned int k=0; k < m_Indexes[i].size(); ++k )
		{
			 if ( m_Indexes[i][k] == j ) 
				 return m_Values[i][k];
			 else if ( m_Indexes[i][k] > j ) 
				 return 0;
		}

		return 0;
	}

	T GetElement(unsigned int i, unsigned int j)
	{
		for ( unsigned int k=0; k < m_Indexes[i].size(); ++k )
		{
			 if ( m_Indexes[i][k] == j ) 
				 return m_Values[i][k];
			 else if ( m_Indexes[i][k] > j ) 
				 return 0;
		}

		return 0;
	}

	void SetElement(unsigned int i, unsigned int j, const T& value)
	{		
		for (unsigned int k = 0; k < m_Indexes[i].size(); ++k )
		{
			if ( m_Indexes[i][k] == j )
			{
				m_Values[i][k] = value;
				return;
			}
			else if( m_Indexes[i][k] > j )
			{
				insert(m_Indexes[i], k, j);
				insert(m_Values[i], k, value);
				return;
			}
		}

		m_Indexes[i].push_back(j);
		m_Values[i].push_back(value);
	}

	void AddToElement(unsigned int i, unsigned int j, const T& incrementValue)
	{
		for (unsigned int k = 0; k < m_Indexes[i].size(); ++k )
		{
			if ( m_Indexes[i][k] == j )
			{
				m_Values[i][k] += incrementValue;
				return;
			}
			else if( m_Indexes[i][k] > j )
			{
				insert(m_Indexes[i], k, j);
				insert(m_Values[i], k, incrementValue);
				return;
			}
		}
	
		m_Indexes[i].push_back(j);
		m_Values[i].push_back(incrementValue);
	}

	void GetTranspose(CSparseMatrix<T>* other)
	{
		assert(m_N == other->m_M);

		for ( unsigned int i = 0; i < Size(); i++ )
		{
			for ( unsigned int j = 0; j < m_Indexes[i].size(); j++ )
			{
				//T val = GetElement(i, m_Indexes[i][j]);
				T val = m_Values[i][j];
				other->SetElement(m_Indexes[i][j], i, val);
			}
		}
	}
	
	void Clear()
	{
		m_N = 0;
		m_Indexes.clear();
		m_Values.clear();
	}

	void ZeroWithoutResize()
	{
		for ( unsigned int i = 0; i < m_N; ++i ) 
		{
			for ( unsigned int j = 0; j < m_Values[i].size(); ++j )
				m_Values[i][j] = 0.0;
		}
	}

	void Zero()
	{
		for ( unsigned int i = 0; i < m_N; ++i ) 
		{
			m_Indexes[i].resize(0);
			m_Values[i].resize(0);
		}
	}

	void Resize(int n)
	{
		m_N = n;

		for ( unsigned int i = 0; i < m_N; ++i ) 
		{
			m_Indexes[i].resize(n);
			m_Values[i].resize(n);
		}
	}

	/*void SaveForMatlab(std::ostream &output, const char *variable_name)
   {
	   std::ofstream outfile ("sparse_matrix.txt",std::ofstream::binary);

	  output<<variable_name<<"=sparse([";
	  for(unsigned int i=0; i<n; ++i){
		 for(unsigned int j=0; j<index[i].size(); ++j){
			output<<i+1<<" ";
		 }
	  }
	  output<<"],...\n  [";
	  for(unsigned int i=0; i<n; ++i){
		 for(unsigned int j=0; j<index[i].size(); ++j){
			output<<index[i][j]+1<<" ";
		 }
	  }
	  output<<"],...\n  [";
	  for(unsigned int i=0; i<n; ++i){
		 for(unsigned int j=0; j<value[i].size(); ++j){
			output<<value[i][j]<<" ";
		 }
	  }
	  output<<"], "<<n<<", "<<n<<");"<<std::endl;
   }*/

protected:
	template<class T>
	void insert(std::vector<T> &a, unsigned int index, T e)
	{
	   a.push_back(a.back());
	   for(unsigned int i=(unsigned int)a.size()-1; i>index; --i)
		  a[i]=a[i-1];
	   a[index]=e;
	}

public:
	T operator()(unsigned int i, unsigned int j) const
	{
		return GetElement(i, j);
	}

	T operator()(unsigned int i, unsigned int j)
	{
		return GetElement(i, j);
	}

	CSparseMatrix<T> operator-(const CSparseMatrix<T>& other)
	{
		assert(Size() == other.Size());

		CSparseMatrix<T> A;
		A = (*this);

		for ( unsigned int i = 0; i < other.Size(); i++ )
		{
			for ( unsigned int j = 0; j < other.m_Indexes[i].size(); j++ )
			{
				T newVal = GetElement(i, other.m_Indexes[i][j]) - other.m_Values[i][j];
				A.SetElement(i, other.m_Indexes[i][j], newVal);
			}
		}

		return A;
	}

	CSparseMatrix<T> operator*(const CSparseMatrix<T>& other)
	{
		if ( bSquare )
			assert(Size() == other.Size());
		else
			assert(m_M == other.m_N);
		
		CSparseMatrix<T> A;
		
		if ( bSquare )
		{
			A.Create(Size());

			for ( unsigned int i = 0; i < Size(); i++ )
			{
				for ( unsigned int j = 0; j < other.Size(); j++ )
				{
					T sum = 0;

					for ( unsigned int p = 0; p < m_Indexes[i].size(); p++ )
					{
						T valThis = GetElement(i, m_Indexes[i][p]);				
						T valOther = other.GetElement(m_Indexes[i][p], j);
						sum += valThis * valOther;
					}

					if ( sum != 0 )
						A.SetElement(i, j, sum);
				}
			}
		}
		else
		{
			A.Create(m_N);

			for ( unsigned int i = 0; i < m_N; i++ )
			{
				for ( unsigned int j = 0; j < other.m_N; j++ )
				{
					T sum = 0;

					for ( unsigned int p = 0; p < m_Indexes[i].size(); p++ )
					{
						T valThis = GetElement(i, m_Indexes[i][p]);				
						T valOther = other.GetElement(m_Indexes[i][p], j);
						sum += valThis.Dot(valOther);
					}

					if ( sum != 0 )
						A.SetElement(i, j, sum);
				}
			}
		}

		return A;
	}

	CSparseMatrix<T> operator*(T val)
	{
		CSparseMatrix<T> A;
			
		for ( unsigned int i = 0; i < Size(); i++ )
		{
			for ( unsigned int j = 0; j < m_Indexes[i].size(); j++ )
			{
				T elem = GetElement(i, m_Indexes[i][j]);
				A.SetElement(m_Indexes[i][j], i, elem * val);
			}
		}

		return A;
	}

	friend CSparseMatrix<T> operator*(T val, const CSparseMatrix<T>& other);

	CSparseMatrix<T> operator*(const CDiagonalMatrix<T>& diagMat)
	{
		CSparseMatrix<T> A;
			
		for ( unsigned int i = 0; i < Size(); i++ )
		{
			for ( unsigned int j = 0; j < m_Indexes[i].size(); j++ )
			{
				T elem = GetElement(i, m_Indexes[i][j]);
				A.SetElement(m_Indexes[i][j], i, elem * diagMat.GetElement(i));
			}
		}

		return A;
	}

	friend CSparseMatrix<T> operator*(const CDiagonalMatrix<T>& diagMat, const CSparseMatrix<T>& other);

	CSparseMatrix<double> Mult(const CSparseMatrix<T>& other)
	{
		if ( bSquare )
			assert(Size() == other.Size());
		else
			assert(m_M == other.m_N);
		
		CSparseMatrix<double> A;
		A.Create(m_N);

		for ( unsigned int i = 0; i < m_N; i++ )
		{
			for ( unsigned int j = 0; j < other.m_N; j++ )
			{
				double sum = 0;

				for ( unsigned int p = 0; p < m_Indexes[i].size(); p++ )
				{
					T valThis = GetElement(i, m_Indexes[i][p]);				
					T valOther = other.GetElement(m_Indexes[i][p], j);
					sum += valThis.Dot(valOther);
				}

				if ( sum != 0 )
					A.SetElement(i, j, sum);
			}
		}

		return A;
	}


	
	// NOTE: Assign operator that compiler provides might be good enough.

	/*T& operator=(const T& other)
	{
		m_N = other.m_N;
		m_NumOfNonzeroPerRow = other.m_NumOfNonzeroPerRow;

		Create(m_N, m_NumOfNonzeroPerRow);

		for ( unsigned int i = 0; i < other.Size(); i++ )
		{
			for ( unsigned int j = 0; j < other.m_Indexes[i].size(); j++ )
			{
				m_Values[i][j] = other.m_Values[i][j];
				m_Indexes[i][j] = other.m_Indexes[i][j];
			}
		}
	}*/
};

template<class T1, class T2>
void Multiply(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& result)
{
	//assert(A.Size() == b.size());
	result.resize(A.NumOfRows());

	for ( unsigned int i = 0; i < A.Size(); i++ )
	{
		result[i] = 0;

		//#pragma omp parallel for
		for ( int j = 0; j < (int)A.m_Indexes[i].size(); j++ )
		{
			result[i] += A.m_Values[i][j] * b[A.m_Indexes[i][j]];
		}
	}
}

template<class T1, class T2, class T3>
void Multiply(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T3>& result)
{
	//assert(A.Size() == b.size());
	result.resize(A.NumOfRows());

	for ( unsigned int i = 0; i < A.Size(); i++ )
	{
		result[i] = 0;

		//#pragma omp parallel for
		for ( int j = 0; j < (int)A.m_Indexes[i].size(); j++ )
		{
			result[i] += A.m_Values[i][j] * b[A.m_Indexes[i][j]];
		}
	}
}

// result = A * B
void Multiply(const CSparseMatrix<double>& A, const CSparseMatrix<double>& B, CSparseMatrix<double>& result);

template<class T>
void Add(const std::vector<T>& a, const std::vector<T>& b, std::vector<T>& result)
{
	assert(a.size() == b.size());
	result.resize(a.size());

	for ( unsigned int i = 0; i < a.size(); i++ )
	{
		result[i] = a[i] + b[i];
	}
}

template<class T>
CSparseMatrix<T> operator*(T val, const CSparseMatrix<T>& other)
{
	return other * val;
}

template<class T>
CSparseMatrix<T> operator*(const CDiagonalMatrix<T>& diagMat, const CSparseMatrix<T>& other)
{
	return other * diagMat;
}


