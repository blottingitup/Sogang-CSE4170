#pragma once
#include <vector>
#include <cuda_runtime.h>

#include "Jacobi_debug_mode.h"

#define _USE_DEVICE_DEBUG 0  // option for checking the error for device kernels, recommend MAX_MATRIX_SIZE = MATRIX_SIZE in Jacobi_main.cpp

constexpr int JACOBI_ITERATION_SUCCESS = 0x00000000;
constexpr int JACOBI_ITERATION_FAIL_MAX_ITERATION = 0x00000001;
constexpr int JACOBI_ITERATION_FAIL_DIVERGENCE_ERROR = 0x00000002;
// What else?

struct Jacobi {
	enum struct Status : uint32_t {
		SUCCESS = 0x00000000,
		FAIL_MAX_ITERATION = 0x00000001,
		FAIL_DIVERGENCE_ERROR = 0x00000002
		// What else?
	};
	size_t matrix_size = 0;
	struct {
		float epsilon = 1.0e-7; // for relative residual norm
		float delta = 1.0e-7; // for difference between successive iterates (currently not used in stopping criteria) 
		int max_iterations = 20; // for maximum number of iterations (must be an even number)
	} stopping_criteria;
	float V_norm = 0.0f; // for the infinity norm of V to calculate the relative residual norm in stopping criteria.
	float final_relative_residual_norm = 0.0f; // for the infinity norm of the final residual to report the final accuracy after finishing Jacobi iteration.	
	
	// Matrices in the linear system UX = V
	std::vector<float> U;
	std::vector<float> V;
	// Matrices in D = A * B + C or B = A * D + C in Jacobi iteration
	std::vector<float> A;
	std::vector<float> B;
	std::vector<float> C;
	std::vector<float> D;
	// Matrix of the residual R = UX - V for norm calculation
	std::vector<float> R; // for the residual matrix in norm calculation
	std::vector<float> T; // for the temporary matrix to calculate the difference between successive iterates in stopping criteria.	
	Jacobi() = default;
	explicit Jacobi(size_t n) : matrix_size(n),
		U(n* n), V(n* n),
		A(n* n), B(n* n), C(n* n), D(n* n),
		R(n* n), T(n* n)
	{
	}
	void set_stopping_criteria(const float epsilon, const float delta, const int max_iterations) {
		stopping_criteria.epsilon = epsilon;
		stopping_criteria.delta = delta; // currently not used in stopping criteria	
		stopping_criteria.max_iterations = ((max_iterations % 2 == 0) ? max_iterations : max_iterations + 1);
	}
	void generate_linear_systems_strictly_row_diagonal_dominant(const float lower_bound, const float upper_bound);
	void prepare_Jacobi_iteration();
	int finish_Jacobi_iteration(const int next_iteration, Status& flag);
	void Jacobi_iteration_HOST(Status& flag);
	void Jacobi_iteration_HOST_debug_mode();
	float Jacobi_iteration_CC(Status& flag);
	float Jacobi_iteration_TC(Status& flag);
};

void launch_jacobi_cc(dim3 grid_dim, dim3 block_dim, float* D, float* A, float* B, float* C, int n);
void launch_jacobi_tc(dim3 grid_dim, dim3 block_dim, float* D, float* A, float* B, float* C, int n);