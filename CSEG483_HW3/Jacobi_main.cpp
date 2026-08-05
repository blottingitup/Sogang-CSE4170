#include <stdio.h>
#include "Jacobi_debug_mode.h"
#include "Jacobi.h"
#include "Matrix_utility.h"

constexpr int MATRIX_SIZE = 1024;
// MAX_ITERATIONS must be an even number since we perform two updates in each Jacobi iteration
// constexpr int MAX_ITERATIONS = 6; 	
constexpr int MAX_ITERATIONS = 6;

void print_log(Jacobi::Status flag) {
	switch (flag) {
	case Jacobi::Status::SUCCESS:
		fprintf(stdout, "\n>>> Jacobi iteration completed successfully.\n");
		break;
	case Jacobi::Status::FAIL_MAX_ITERATION:
	case Jacobi::Status::FAIL_DIVERGENCE_ERROR:
		fprintf(stdout, "\n>>> Jacobi iteration failed to converge (Error Code: %d).\n", static_cast<int>(flag));
		break;
	default:
		break;
	}
}

int main() {
	cudaSetDeviceFlags(cudaDeviceScheduleSpin);

	int max_iterations = MAX_ITERATIONS;
	int max_matrix_size = _USE_DEVICE_DEBUG ? MATRIX_SIZE : MATRIX_SIZE * 4;  // control max size of matrix
	for (size_t matrix_size = MATRIX_SIZE; matrix_size <= max_matrix_size; matrix_size *= 2) {
		//size_t matrix_size = MATRIX_SIZE;	
		
		Jacobi::Status flag = Jacobi::Status::SUCCESS;
		Jacobi jacobi{ matrix_size };

		jacobi.set_stopping_criteria(1.0e-7, 1.0e-7, max_iterations);
		jacobi.generate_linear_systems_strictly_row_diagonal_dominant(-1.0f, 1.0f);
		//print_square_matrix(jacobi.U.data(), matrix_size);
		//print_square_matrix(jacobi.V.data(), matrix_size);
		jacobi.prepare_Jacobi_iteration();
		//print_square_matrix(jacobi.A.data(), matrix_size);
		//print_square_matrix(jacobi.C.data(), matrix_size);
#if defined(_JACOBI_ITERATION_DEBUG_MODE)	
		jacobi.Jacobi_iteration_HOST_debug_mode();
#else
		jacobi.Jacobi_iteration_HOST(flag);
		print_log(flag);
		flag = Jacobi::Status::SUCCESS;
		jacobi.prepare_Jacobi_iteration();

		int rep = _USE_DEVICE_DEBUG ? 1 : 10;  // no extra iteration when debugging
		float cc_time = 0.0f, tc_time = 0.0f;
		for (int i = 0; i < rep; i++) {
			cc_time += jacobi.Jacobi_iteration_CC(flag);
			print_log(flag);
			flag = Jacobi::Status::SUCCESS;
			jacobi.prepare_Jacobi_iteration();

			tc_time += jacobi.Jacobi_iteration_TC(flag);
			print_log(flag);
			flag = Jacobi::Status::SUCCESS;
			jacobi.prepare_Jacobi_iteration();
		}
		fprintf(stdout, "\n>>> Average execution time on CUDA core: %.3f", cc_time / (rep * 1.0f));
		fprintf(stdout, "\n>>> Average execution time on Tensor core: %.3f", tc_time / (rep * 1.0f));
		fprintf(stdout, "\n\n");
#endif
	}
}
