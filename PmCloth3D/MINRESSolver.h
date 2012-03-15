#pragma once

#include <omp.h>
#include "SparseMatrix.h"
#include <vector>

template <class T1, class T2>
class CMINRESSolver
{
public:
	CMINRESSolver(void) {};
	~CMINRESSolver(void) {};

private:
	std::vector<T2> r;
	std::vector<T2> z; 
	std::vector<T2> q;
	std::vector<T2> s;
	std::vector<T2> t;

public:
	// Pinv is inverse of preconditioner
	// Matrix A must be symmetric but it may be definite or indefinite or singular.
	// MINRES solves least-squares problem: 
	//        minimize ||Ax-b||
	// TODO:preconditioner doesn't work. Need to fix it. 
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, const CSparseMatrix<T1>* Pinv = NULL, bool bUseXAsInitialGuess = false, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());

		if ( z.size() != b.size() )
			z.resize(b.size());
		
		if ( q.size() != b.size() )
			q.resize(b.size());
		
		if ( s.size() != b.size() )
			s.resize(b.size());

		if ( t.size() != b.size() )
			t.resize(b.size());

		// TODO:add option to use the initial guess, not just zeroing x[].
		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{			
			// r(0) = b
			r[i] = b[i];	

			z[i] = 0;
			q[i] = 0;
			s[i] = 0;
			t[i] = 0;
		
			// initialize x
			if ( !bUseXAsInitialGuess )
				x[i] = 0;
		}
		
		if ( Pinv )
			Multiply(*Pinv, r, z); // z(0) = Pinv * r0
		else
		{
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
				s[i] = r[i];
		}

		Multiply(A, s, t);
		double rho = InnerProd(r, t);

		if ( rho == 0 )
			return 0; 
				
		while ( count++ < max_iter )
		{	
			double t2 = InnerProd(t, t);

			if ( t2 == 0 )
				return 0; 
				
			double alpha = rho / t2;

			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				x[i] += s[i] * alpha;	
				r[i] -= t[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			double error = InnerProd(r, r);

			//if ( error.Length() < tolerance )
			//	break; // converged

			if ( error < tolerance )
				break; // converged

			if ( Pinv )
				Multiply(*Pinv, r, z); 
			else
			{
				#pragma omp parallel for
				for ( int i = 0; i < (int)b.size(); i++ ) 
					z[i] = r[i];
			}
			
			Multiply(A, r, q); 
			double rhoNew = InnerProd(r, q);

			if ( rhoNew == 0 )
				return 0; 

			double beta = rhoNew/rho;	
		
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				z[i] += s[i] * beta;
				T2 temp = s[i];
				s[i] = z[i];
				z[i] = temp;

				q[i] += t[i] * beta;
				temp = q[i];
				q[i] = t[i];
				t[i] = temp;
			}

			rho = rhoNew;
		}

		return count;
	}


	double InnerProd(const std::vector<T2>& a, const std::vector<T2>& b)
	{
		assert(a.size() == b.size());

		double sum = 0;

		#pragma omp parallel for reduction(+:sum)
		for ( int i = 0; i < (int)a.size(); i++ )
		{
			//#pragma omp single nowait
			{
				sum += a[i].Dot(b[i]);
				//result += a[i]*(b[i]);
			}
		}

		return sum;
	}
};
