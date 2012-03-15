#pragma once

#include <omp.h>
#include "SparseMatrix.h"
#include <vector>


template <class T1, class T2>
class CCGSolver
{
public:
	CCGSolver(void) {};
	~CCGSolver(void) {};

private:
	std::vector<T2> r;
	std::vector<T2> z; // for preconditioned CG
	std::vector<T2> p;
	std::vector<T2> Ap;

public:
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, bool bUseXAsInitialGuess = false, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());
		
		if ( p.size() != b.size() )
			p.resize(b.size());
		
		if ( Ap.size() != b.size() )
			Ap.resize(b.size());

		// TODO:add option to use the initial guess, not just zeroing x[].
		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			T2 r0 = b[i];
			
			// r(0) = r0
			r[i] = r0;
			
			// p(0) = r0
			p[i] = r0;

			// initialize x
			if ( !bUseXAsInitialGuess )
				x[i] = 0;
		}
	
		while ( count++ < max_iter )
		{	
			// Ap = A*p(l)
			Multiply(A, p, Ap);

			double prod0 = InnerProd(r, r);
			double prod1 = InnerProd(p, Ap);
	
			// alpha = r(l) * r(l) / p(l) * A * p(l)
			double alpha = 0;

			if ( prod1 != 0 )
				alpha = prod0/prod1;

			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				// x(l+1) = x(l) + alpha * p(l)
				x[i] += p[i] * alpha;	

				// r(l+1) = r(l) - alpha * A * p(l)
				r[i] -= Ap[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			double error = InnerProd(r, r);
			 
			if ( error < tolerance )
				break;
						
			// beta = r(l+1) * r(l+1) / r(l) * r(l)
			double beta = InnerProd(r, r)/prod0;	
		
			// p(l+1) = r(l+1) + beta * p(l)
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
				p[i] = r[i] + p[i] * beta;
		}

		return count;
	}

	// Pinv is inverse of preconditioner
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, const CSparseMatrix<T1>& Pinv, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());

		if ( z.size() != b.size() )
			z.resize(b.size());
		
		if ( p.size() != b.size() )
			p.resize(b.size());
		
		if ( Ap.size() != b.size() )
			Ap.resize(b.size());

		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			T2 r0 = b[i];
			
			// r(0) = r0
			r[i] = r0;					
		
			// initialize x
			x[i] = 0;
		}

		// z(0) = Pinv * r0
		Multiply(Pinv, r, z);

		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			// p(0) = z(0)
			p[i] = z[i];
		}
				
		while ( count++ < max_iter )
		{	
			// Ap = A*p(l)
			Multiply(A, p, Ap);

			double prod0 = InnerProd(r, z);
			double prod1 = InnerProd(p, Ap);
	
			// alpha = r(l) * z(l) / p(l) * A * p(l)
			double alpha;

			if ( prod1 != 0 )
				alpha = prod0/prod1;
			else
				alpha = 0;

			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				// x(l+1) = x(l) + alpha * p(l)
				x[i] += p[i] * alpha;	

				// r(l+1) = r(l) - alpha * A * p(l)
				r[i] -= Ap[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			double error = InnerProd(r, r);

			//if ( error.Length() < tolerance )
			if ( error < tolerance )
				break;

			// z(l+1) = Pinv * r(l+1)
			Multiply(Pinv, r, z);
					
			// beta = z(l+1) * r(l+1) / z(l) * r(l)
			double beta = InnerProd(z, r)/prod0;	
		
			// p(l+1) = z(l+1) + beta * p(l)
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
				p[i] = z[i] + p[i] * beta;
		}

		return count;
	}

public:

	// Pinv is inverse of preconditioner
	unsigned int SolveFiltered(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, const CSparseMatrix<T1>& Pinv, void (*Filter)(std::vector<T2>&, const void*) = NULL, const void* pCloth = NULL, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());

		if ( z.size() != b.size() )
			z.resize(b.size());
		
		if ( p.size() != b.size() )
			p.resize(b.size());
		
		if ( Ap.size() != b.size() )
			Ap.resize(b.size());

		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			T2 r0 = b[i];
			
			// r(0) = r0
			r[i] = r0;					
		
			// initialize x
			x[i] = 0;
		}

		if ( Filter )
		{
			Filter(r, pCloth);
			Filter(x, pCloth);
		}

		// z(0) = Pinv * r0
		Multiply(Pinv, r, z);

		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			// p(0) = z(0)
			p[i] = z[i];
		}
				
		while ( count++ < max_iter )
		{	
			// Ap = A*p(l)
			Multiply(A, p, Ap);

			if ( Filter )
				Filter(Ap, pCloth);

			double prod0 = InnerProd(r, z);
			double prod1 = InnerProd(p, Ap);
	
			// alpha = r(l) * z(l) / p(l) * A * p(l)
			double alpha = 0;

			if ( prod1 != 0 )
				alpha = prod0/prod1;

			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				// x(l+1) = x(l) + alpha * p(l)
				x[i] += p[i] * alpha;	

				// r(l+1) = r(l) - alpha * A * p(l)
				r[i] -= Ap[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			double error = InnerProd(r, r);

			if ( error < tolerance )
				break;

			// z(l+1) = Pinv * r(l+1)
			Multiply(Pinv, r, z);
					
			// beta = z(l+1) * r(l+1) / z(l) * r(l)
			double beta = InnerProd(z, r)/prod0;	
		
			// p(l+1) = z(l+1) + beta * p(l)
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
				p[i] = z[i] + p[i] * beta;

			if ( Filter )
				Filter(p, pCloth);
		}

		return count;
	}

	static double InnerProd(const std::vector<T2>& a, const std::vector<T2>& b)
	{
		assert(a.size() == b.size());

		double result = 0;

		#pragma omp parallel for reduction(+:result)
		for ( int i = 0; i < (int)a.size(); i++ )
		{
			result += a[i] * b[i]; // inner dot product				
		}

		return result;
	}
};

// For IncompressibleCollisionResolutionGlobally
template <class T1, class T2>
class CCGSolver2
{
public:
	CCGSolver2(void) {};
	~CCGSolver2(void) {};

private:
	std::vector<T2> r;
	std::vector<T2> p;
	std::vector<T2> Ap;

public:
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());
		
		if ( p.size() != b.size() )
			p.resize(b.size());
		
		if ( Ap.size() != b.size() )
			Ap.resize(b.size());

		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			T2 r0 = b[i];
			
			// r(0) = r0
			r[i] = r0;
			
			// p(0) = r0
			p[i] = r0;

			// initialize x
			x[i] = 0;
		}
	
		while ( count++ < max_iter )
		{	
			// Ap = A*p(l)
			Multiply(A, p, Ap);

			double prod0 = InnerProd(r, r);
			double prod1 = InnerProd(p, Ap);
	
			// alpha = r(l) * r(l) / p(l) * A * p(l)
			T2 alpha;

			if ( prod1 != 0 )
				alpha = prod0/prod1;
			else
				alpha = 0;

			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				// x(l+1) = x(l) + alpha * p(l)
				x[i] += p[i] * alpha;	

				// r(l+1) = r(l) - alpha * A * p(l)
				r[i] -= Ap[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			double error = InnerProd(r, r);

			if ( error < tolerance )
				break;
					
			// beta = r(l+1) * r(l+1) / r(l) * r(l)
			double beta = InnerProd(r, r)/prod0;	
		
			// p(l+1) = r(l+1) + beta * p(l)
			for ( int i = 0; i < (int)b.size(); i++ ) 
				p[i] = r[i] + p[i] * beta;
		}

		return (count-1);
	}

	T2 InnerProd(const std::vector<T2>& a, const std::vector<T2>& b)
	{
		assert(a.size() == b.size());

		T2 result = 0;

		for ( unsigned int i = 0; i < a.size(); i++ )
			result += a[i] * b[i];

		return result;
	}
};

// For indefinite matrix 
template <class T1, class T2>
class CCGSolver3
{
public:
	CCGSolver3(void) {};
	~CCGSolver3(void) {};

private:
	std::vector<T2> R;
	std::vector<T2> T;
	std::vector<T2> AT;

public:
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		T2 beta = 0;
		
		if ( R.size() != b.size() )
			R.resize(b.size());
		
		if ( T.size() != b.size() )
			T.resize(b.size());
		
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{			
			R[i] = b[i];

			// initialize x
			x[i] = 0;
		}
	
		while ( count++ < max_iter )
		{	
			T2 alpha = InnerProd(R, R);

			if ( beta != 0 )
			{
				// T = R +  (alpha / beta) * T
				for ( int i = 0; i < (int)b.size(); i++ ) 
					T[i] = R[i] + (alpha / beta) * T[i];
			}
			else
			{
				for ( int i = 0; i < (int)b.size(); i++ ) 
					T[i] = R[i]; // T = R
			}

			// beta = Tr(T) * A * T
			Multiply(A, T, AT);
			beta = InnerProd(T, AT);

			// R = R -  (alpha / beta) * A * T
			for ( int i = 0; i < (int)b.size(); i++ ) 
					R[i] = R[i] - (alpha / beta) * AT[i];

			// x = x +  (alpha / beta) * T
			for ( int i = 0; i < (int)b.size(); i++ ) 
					x[i] = x[i] + (alpha / beta) * T[i];

			beta = alpha;
		}

		return (count-1);
	}

	T2 InnerProd(const std::vector<T2>& a, const std::vector<T2>& b)
	{
		assert(a.size() == b.size());

		T2 result = 0;

		for ( unsigned int i = 0; i < a.size(); i++ )
			result += a[i].Dot(b[i]);

		return result;
	}
};

template <class T1, class T2>
class CCGSolver4
{
public:
	CCGSolver4(void) {};
	~CCGSolver4(void) {};

private:
	std::vector<T2> r;
	std::vector<T2> z; // for preconditioned CG
	std::vector<T2> p;
	std::vector<T2> Ap;

public:
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, bool bUseXAsInitialGuess = false, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());
		
		if ( p.size() != b.size() )
			p.resize(b.size());
		
		if ( Ap.size() != b.size() )
			Ap.resize(b.size());

		// TODO:add option to use the initial guess, not just zeroing x[].
		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			T2 r0 = b[i];
			
			// r(0) = r0
			r[i] = r0;
			
			// p(0) = r0
			p[i] = r0;

			// initialize x
			if ( !bUseXAsInitialGuess )
				x[i] = 0;
		}
	
		while ( count++ < max_iter )
		{	
			// Ap = A*p(l)
			Multiply(A, p, Ap);

			T2 prod0 = InnerProd(r, r);
			T2 prod1 = InnerProd(p, Ap);
	
			// alpha = r(l) * r(l) / p(l) * A * p(l)
			T2 alpha;

			if ( prod1 != 0 )
				alpha = prod0/prod1;
			else
				alpha = 0;

			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				// x(l+1) = x(l) + alpha * p(l)
				x[i] += p[i] * alpha;	

				// r(l+1) = r(l) - alpha * A * p(l)
				r[i] -= Ap[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			T2 error = InnerProd(r, r);
			 
			if ( error < tolerance )
				break;
						
			// beta = r(l+1) * r(l+1) / r(l) * r(l)
			T2 beta = InnerProd(r, r)/prod0;	
		
			// p(l+1) = r(l+1) + beta * p(l)
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
				p[i] = r[i] + p[i] * beta;
		}

		return count;
	}

	// Pinv is inverse of preconditioner
	unsigned int Solve(const CSparseMatrix<T1>& A, const std::vector<T2>& b, std::vector<T2>& x, const CSparseMatrix<T1>& Pinv, double tolerance = 1e-6, unsigned int max_iter = 200)
	{
		unsigned int count = 0;
		
		if ( r.size() != b.size() )
			r.resize(b.size());

		if ( z.size() != b.size() )
			z.resize(b.size());
		
		if ( p.size() != b.size() )
			p.resize(b.size());
		
		if ( Ap.size() != b.size() )
			Ap.resize(b.size());

		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			T2 r0 = b[i];
			
			// r(0) = r0
			r[i] = r0;					
		
			// initialize x
			x[i] = 0;
		}

		// z(0) = Pinv * r0
		Multiply(Pinv, r, z);

		#pragma omp parallel for
		for ( int i = 0; i < (int)b.size(); i++ ) 
		{
			// p(0) = z(0)
			p[i] = z[i];
		}
				
		while ( count++ < max_iter )
		{	
			// Ap = A*p(l)
			Multiply(A, p, Ap);

			T2 prod0 = InnerProd(r, z);
			T2 prod1 = InnerProd(p, Ap);
	
			// alpha = r(l) * z(l) / p(l) * A * p(l)
			T2 alpha;

			if ( prod1 != 0 )
				alpha = prod0/prod1;
			else
				alpha = 0;

			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
			{
				// x(l+1) = x(l) + alpha * p(l)
				x[i] += p[i] * alpha;	

				// r(l+1) = r(l) - alpha * A * p(l)
				r[i] -= Ap[i] * alpha;
			}
		
			// ||r(l+1)|| <= tolerance
			T2 error = InnerProd(r, r);

			if ( error.Length() < tolerance )
			//if ( error < tolerance )
				break;

			// z(l+1) = Pinv * r(l+1)
			Multiply(Pinv, r, z);
					
			// beta = z(l+1) * r(l+1) / z(l) * r(l)
			T2 beta = InnerProd(z, r)/prod0;	
		
			// p(l+1) = z(l+1) + beta * p(l)
			#pragma omp parallel for
			for ( int i = 0; i < (int)b.size(); i++ ) 
				p[i] = z[i] + p[i] * beta;
		}

		return count;
	}

public:
	

	T2 InnerProd(const std::vector<T2>& a, const std::vector<T2>& b)
	{
		assert(a.size() == b.size());

		T2 result = 0;

		//#pragma omp parallel for reduction(+:result)
		for ( int i = 0; i < (int)a.size(); i++ )
		{
			//#pragma omp single nowait
			{
				result += a[i] * b[i];				
			}
		}

		return result;
	}
};