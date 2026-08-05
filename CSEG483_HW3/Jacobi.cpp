#include <stdio.h>
#include <math.h>
#include <random>
#include "Jacobi.h"
#include "Matrix_utility.h"
#include "measure_host_time_3.h"

#define _USE_KAHAN_SUM 1

void Jacobi::prepare_Jacobi_iteration() {
	// prepare the matrices for Jacobi iteration by normalizing the rows of A and C.
	for (size_t i = 0; i < matrix_size; i++) {
		double inverse_diagonal = 1.0 / U[i * matrix_size + i]; // Use the diagonal element for normalization	
		for (size_t j = 0; j < matrix_size; j++) {
			A[i * matrix_size + j] = -U[i * matrix_size + j] * inverse_diagonal; // normalize row i of U to create A.	
			C[i * matrix_size + j] = V[i * matrix_size + j] * inverse_diagonal; // normalize row i of V to create C.	
			if (i == j)
				A[i * matrix_size + j] += 1.0f; // set diagonal of A to 0.	
		}
	}
	// initialize B to the identity matrix for the initial guess of the solution.
	for (size_t i = 0; i < matrix_size; i++) {
		for (size_t j = 0; j < matrix_size; j++) {
			if (i == j)
				B[i * matrix_size + j] = 1.0f; 
			else
				B[i * matrix_size + j] = 0.0f;
		}
	}
}

int Jacobi::finish_Jacobi_iteration(int next_iteration, Status& flag) {
#ifdef _USE_KAHAN_SUM
	matrix_residual_Kahan_sum(R.data(), U.data(), B.data(), V.data(), matrix_size);
#else
	matrix_residual(R.data(), U.data(), B.data(), V.data(), matrix_size);
#endif
	final_relative_residual_norm = matrix_infinity_norm(R.data(), matrix_size) / V_norm;
	if (next_iteration >= stopping_criteria.max_iterations) {
		flag = Status::FAIL_MAX_ITERATION;		
		return 1; // stop if the number of iterations exceeds the maximum allowed.
	}
	if (final_relative_residual_norm < stopping_criteria.epsilon) {
		flag = Status::SUCCESS;
		return 1; // stop if the relative residual norm is small enough.
	}
	return 0;
}

void Jacobi::Jacobi_iteration_HOST(Status& flag) {
	fprintf(stdout, ">>> [HOST]\n>>> Trying to solve for X in UX = V of size %d using %d maximum Jacobi iterations...\n",
		matrix_size, stopping_criteria.max_iterations);
	// return the approximate solution in B after performing n_iterations Jacobi iterations.
	fprintf(stdout, "    ");
	V_norm = matrix_infinity_norm(V.data(), matrix_size); // should take care of the case when V_norm is near zero.

	int iteration_count = 0;
	CHECK_TIME_START(_start, _freq);
	while (1) { 
		if (finish_Jacobi_iteration(iteration_count, flag)) break;
		// each Jacobi iteration updates the approximate solution two times. 	
		matrix_fma(D.data(), A.data(), B.data(), C.data(), matrix_size); // Compute D = A * B + C
		fprintf(stdout, ".");
		matrix_fma(B.data(), A.data(), D.data(), C.data(), matrix_size); // Update B = A * D + C for the next iteration	
		fprintf(stdout, ".");
		iteration_count += 2;
	}
	CHECK_TIME_END(_start, _end, _freq, _compute_time);

	fprintf(stdout, "\n>>> The final relative residual error after %d iterations ||R||/||V|| = ||UX - V||/||V|| is %e.\n",
		iteration_count, final_relative_residual_norm);
	fprintf(stdout, "\n>>> Total execution time using host: %.3f ms", _compute_time);
}

void Jacobi::Jacobi_iteration_HOST_debug_mode() {
	fprintf(stdout, ">>> Trying to solve for X in UX = V of size %d using %d Jacobi iterations(DEBUG mode)...\n", 
		matrix_size, stopping_criteria.max_iterations);
	V_norm = matrix_infinity_norm(V.data(), matrix_size);	// should take care of the case when V_norm is near zero.

	float residual_norm = 0.0f;
	for (int iteration_count = 0; iteration_count < stopping_criteria.max_iterations; iteration_count += 2) {
		// each Jacobi iteration updates the approximate solution two times 	
		matrix_fma(D.data(), A.data(), B.data(), C.data(), matrix_size); // compute D = A * B + C.
#ifdef _USE_KAHAN_SUM
		matrix_residual_Kahan_sum(R.data(), U.data(), D.data(), V.data(), matrix_size);
#else
		matrix_residual(R.data(), U.data(), D.data(), V.data(), matrix_size);	
#endif
		residual_norm = matrix_infinity_norm(R.data(), matrix_size);
		fprintf(stdout, "    After the iteration %2d, the relative error is ||R||/||V|| = ||UX - V||/||V|| = %e.\n", 
			iteration_count + 1, residual_norm / V_norm);

		matrix_fma(B.data(), A.data(), D.data(), C.data(), matrix_size); // update B = A * D + C for the next iteration.
#ifdef _USE_KAHAN_SUM
		matrix_residual_Kahan_sum(R.data(), U.data(), B.data(), V.data(), matrix_size);
#else
		matrix_residual(R.data(), U.data(), B.data(), V.data(), matrix_size);
#endif
		residual_norm = matrix_infinity_norm(R.data(), matrix_size);
		fprintf(stdout, "    After the iteration %2d, the relative error is ||R||/||V|| = ||UX - V||/||V|| = %e.\n",
			iteration_count + 2, residual_norm / V_norm);
	}
	fprintf(stdout, ">>> Done!\n");
}

#define JACOBI_DIAGONAL_DOMINANCE_FACTOR 1.1f	// Do not change this factor!
void Jacobi::generate_linear_systems_strictly_row_diagonal_dominant(const float lower_bound, const float upper_bound) {
	// UX= V can be solved using Jacobi iteration, where U is a positive definite matrix and V is a random matrix.
	// generate a random positive definite matrix U and a random matrix V.	
#if defined(_JACOBI_MATRIX_GENERATION_DEBUG_MODE)	
	unsigned int fixed_seed = 20260601;
	std::mt19937 gen(fixed_seed);
	std::uniform_real_distribution<> dist(lower_bound, upper_bound);
#else
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<> dist(lower_bound, upper_bound);
#endif

	for (size_t i = 0; i < matrix_size * matrix_size; i++)
		U[i] = dist(gen);
	// make U positive definite by having the diagonal elements larger than the sum of 
	// the absolute values of the non-diagonal elements in the same row.	
	for (size_t i = 0; i < matrix_size; i++) {
		double row_sum = 0.0;
		for (size_t j = 0; j < matrix_size; j++) {
			if (i != j) {
				row_sum += fabs(U[i * matrix_size + j]);
			}
		}
		U[i * matrix_size + i] = row_sum * JACOBI_DIAGONAL_DOMINANCE_FACTOR;
	}
	for (size_t i = 0; i < matrix_size * matrix_size; i++)
		V[i] = dist(gen);
}

float Jacobi::Jacobi_iteration_CC(Status& flag) {
	fprintf(stdout, "\n>>> [CUDA CORE]\n>>> Trying to solve for X in UX = V of size %d using %d maximum Jacobi iterations...\n",
		matrix_size, stopping_criteria.max_iterations);
	// return the approximate solution in B after performing n_iterations Jacobi iterations.
	fprintf(stdout, "    ");
	V_norm = matrix_infinity_norm(V.data(), matrix_size); // should take care of the case when V_norm is near zero.

	// Set dimensions of block and grid
	constexpr int TS = 16, WPT = 4;
	constexpr int RTS = TS / WPT;
	dim3 block_dim(TS, RTS, 1), grid_dim(matrix_size / TS, matrix_size / TS, 1);

	// Copy matrix to device
	size_t size = matrix_size * matrix_size * sizeof(float);
	float* d_A = NULL, *d_B = NULL, *d_C = NULL, *d_D = NULL;
	cudaMalloc((void**)&d_A, size);
	cudaMalloc((void**)&d_B, size);
	cudaMalloc((void**)&d_C, size);
	cudaMalloc((void**)&d_D, size);

	cudaMemcpy(d_A, A.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_B, B.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_C, C.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_D, D.data(), size, cudaMemcpyHostToDevice);

	std::vector<float> backup(matrix_size * matrix_size);  // Save the last calculated result

	// Dummy call
	launch_jacobi_cc(grid_dim, block_dim, d_D, d_A, d_B, d_C, matrix_size);
	cudaDeviceSynchronize();

	int iteration_count = 0;
	float device_time = 0.0f;
#if !_USE_DEVICE_DEBUG
	CHECK_TIME_START(_start, _freq);
#endif
	while (iteration_count < stopping_criteria.max_iterations) {
#if _USE_DEVICE_DEBUG
		CHECK_TIME_START(_start, _freq);
#endif
		launch_jacobi_cc(grid_dim, block_dim, d_D, d_A, d_B, d_C, matrix_size);
		launch_jacobi_cc(grid_dim, block_dim, d_B, d_A, d_D, d_C, matrix_size);
#if _USE_DEVICE_DEBUG
		cudaDeviceSynchronize();
		CHECK_TIME_END(_start, _end, _freq, _compute_time);
		device_time += _compute_time;
#endif
		//fprintf(stdout, "..");
		iteration_count += 2;

#if _USE_DEVICE_DEBUG
		// Check invalid operations per loop
		if (iteration_count % 2 == 0) {
			cudaMemcpy(B.data(), d_B, size, cudaMemcpyDeviceToHost);

			bool is_finished = (finish_Jacobi_iteration(iteration_count, flag));
			if (std::isnan(final_relative_residual_norm) || std::isinf(final_relative_residual_norm)) {
				flag = Status::FAIL_DIVERGENCE_ERROR;
				B = backup;
				break;
			}

			fprintf(stdout, "\n    After %02d iterations, the relative error is: ||R||/||V|| = %e", iteration_count, final_relative_residual_norm);

			if (is_finished) break;
			backup = B;
		}
#endif
	}
#if !_USE_DEVICE_DEBUG
	cudaDeviceSynchronize();
	CHECK_TIME_END(_start, _end, _freq, device_time);

	cudaMemcpy(B.data(), d_B, size, cudaMemcpyDeviceToHost);
	finish_Jacobi_iteration(iteration_count, flag);
	if (final_relative_residual_norm < stopping_criteria.epsilon)
		flag = Status::SUCCESS;
#endif

	fprintf(stdout, "\n>>> The final relative residual error after %d iterations ||R||/||V|| = ||UX - V||/||V|| is %e.\n",
		iteration_count, final_relative_residual_norm);
	fprintf(stdout, "\n>>> Total execution time using CUDA core: %.3f ms", device_time);

	cudaFree(d_A);
	cudaFree(d_B);
	cudaFree(d_C);
	cudaFree(d_D);

	return device_time;
}

float Jacobi::Jacobi_iteration_TC(Status& flag) {
	fprintf(stdout, "\n>>> [TENSOR CORE]\n>>> Trying to solve for X in UX = V of size %d using %d maximum Jacobi iterations...\n",
		matrix_size, stopping_criteria.max_iterations);
	// return the approximate solution in B after performing n_iterations Jacobi iterations.
	//fprintf(stdout, "    ");
	V_norm = matrix_infinity_norm(V.data(), matrix_size); // should take care of the case when V_norm is near zero.

	// Set dimensions of block and grid
	dim3 block_dim(256, 1, 1), grid_dim((matrix_size / 16) * (matrix_size / 16) / 8, 1, 1);

	// Copy matrix to device
	size_t size = matrix_size * matrix_size * sizeof(float);
	float* d_A = NULL, * d_B = NULL, * d_C = NULL, * d_D = NULL;
	cudaMalloc((void**)&d_A, size);
	cudaMalloc((void**)&d_B, size);
	cudaMalloc((void**)&d_C, size);
	cudaMalloc((void**)&d_D, size);

	cudaMemcpy(d_A, A.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_B, B.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_C, C.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_D, D.data(), size, cudaMemcpyHostToDevice);

	std::vector<float> backup(matrix_size * matrix_size);  // Save the last calculated result

	// Dummy call
	launch_jacobi_tc(grid_dim, block_dim, d_D, d_A, d_B, d_C, matrix_size);
	cudaDeviceSynchronize();

	int iteration_count = 0;
	float device_time = 0.0f;
#if !_USE_DEVICE_DEBUG
	CHECK_TIME_START(_start, _freq);
#endif
	while (iteration_count < stopping_criteria.max_iterations) {
#if _USE_DEVICE_DEBUG
		CHECK_TIME_START(_start, _freq);
#endif
		launch_jacobi_tc(grid_dim, block_dim, d_D, d_A, d_B, d_C, matrix_size);
		launch_jacobi_tc(grid_dim, block_dim, d_B, d_A, d_D, d_C, matrix_size);
#if _USE_DEVICE_DEBUG
		cudaDeviceSynchronize();
		CHECK_TIME_END(_start, _end, _freq, _compute_time);
		device_time += _compute_time;
#endif
		//fprintf(stdout, "..");
		iteration_count += 2;

#if _USE_DEVICE_DEBUG
		// Check invalid operations per loop
		if (iteration_count % 2 == 0) {
			cudaMemcpy(B.data(), d_B, size, cudaMemcpyDeviceToHost);

			bool is_finished = (finish_Jacobi_iteration(iteration_count, flag));
			if (std::isnan(final_relative_residual_norm) || std::isinf(final_relative_residual_norm)) {
				flag = Status::FAIL_DIVERGENCE_ERROR;
				B = backup;
				break;
			}

			fprintf(stdout, "\n    After %02d iterations, the relative error is: ||R||/||V|| = %e", iteration_count, final_relative_residual_norm);

			if (is_finished) break;
			backup = B;
		}
#endif
	}

#if !_USE_DEVICE_DEBUG
	cudaDeviceSynchronize();
	CHECK_TIME_END(_start, _end, _freq, device_time);

	cudaMemcpy(B.data(), d_B, size, cudaMemcpyDeviceToHost);
	finish_Jacobi_iteration(iteration_count, flag);
	if (final_relative_residual_norm < stopping_criteria.epsilon)
		flag = Status::SUCCESS;
#endif

	fprintf(stdout, "\n>>> The final relative residual error after %d iterations ||R||/||V|| = ||UX - V||/||V|| is %e.\n",
		iteration_count, final_relative_residual_norm);
	fprintf(stdout, "\n>>> Total execution time using Tensor core: %.3f ms", device_time);

	cudaFree(d_A);
	cudaFree(d_B);
	cudaFree(d_C);
	cudaFree(d_D);

	return device_time;
}