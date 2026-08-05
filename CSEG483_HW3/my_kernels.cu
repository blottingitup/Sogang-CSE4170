#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <mma.h>
#include "Jacobi.h"

#define TS 16
#define WPT 4
#define RTS (TS / WPT)

// Kernel based on gputiled_more_work, with extra addition at the end
// All the matrices have size (matrix_size, matrix_size), thus use 'n' instead of 'Ay', 'Ax', 'Bx'
__global__ void jacobi_cc(float* __restrict D, float* __restrict A, float* __restrict B, float* __restrict C, int n) {
	__shared__ float Atile[TS][TS];
	__shared__ float Btile[TS][TS];
	float accum[WPT];
	for (int w = 0; w < WPT; w++) accum[w] = 0.0f;

	int tx = threadIdx.x, ty = threadIdx.y;
	int ocx = blockDim.x * blockIdx.x, ocy = WPT * blockDim.y * blockIdx.y;

	int ax = tx, ay = ocy + ty;
	int bx = ocx + tx, by = ty;

	for (int t = 0; t < n / TS; t++) {
		for (int w = 0; w < WPT; w++) {
			Atile[ty + w * RTS][tx] = A[(ay + w * RTS) * n + ax];
			Btile[ty + w * RTS][tx] = B[(by + w * RTS) * n + bx];
		}
		__syncthreads();

		for (int k = 0; k < TS; k++) {
			float tmp = Btile[k][tx];
			for (int w = 0; w < WPT; w++) accum[w] += Atile[ty + w * RTS][k] * tmp;
		}
		__syncthreads();

		ax += TS, by += TS;
	}
	
	// D = A * B + C
	for (int w = 0; w < WPT; w++)
		D[(ay + w * RTS) * n + bx] = accum[w] + C[(ay + w * RTS) * n + bx];
}

// Kernel based on matmulTS, with use of TF32 and separate accumulator
// All the matrices have size (matrix_size, matrix_size), thus use 'n' instead of 'Ay', 'Ax', 'Bx'
__global__ void jacobi_tc(float* __restrict D, float* __restrict A, float* __restrict B, float* __restrict C, int n) {
	__shared__ float as[256];
	__shared__ float bs[8][256];

	if (blockDim.x != 256) return;
	
	int warp = (blockDim.x * blockIdx.x + threadIdx.x) / warpSize;
	int cx = warp % (n / 16), cy = warp / (n / 16);
	
	int Atile_pos = cy * 16 * n, Btile_pos = cx * 16;

	int wb = threadIdx.x / 32, trw = threadIdx.x % 32;
	int txw = trw % 16, tyw = trw / 16;
	int idx = threadIdx.x % 16, idy = threadIdx.x / 16;


	nvcuda::wmma::fragment<nvcuda::wmma::matrix_a, 16, 16, 8, nvcuda::wmma::precision::tf32, nvcuda::wmma::row_major> a_frag;
	nvcuda::wmma::fragment<nvcuda::wmma::matrix_b, 16, 16, 8, nvcuda::wmma::precision::tf32, nvcuda::wmma::row_major> b_frag;
	nvcuda::wmma::fragment<nvcuda::wmma::accumulator, 16, 16, 8, float> c_frag;
	nvcuda::wmma::load_matrix_sync(c_frag, &C[(cy * n + cx) * 16], n, nvcuda::wmma::mem_row_major);

	for (int k = 0; k < n / 16; k++) {
		as[idy * 16 + idx] = A[Atile_pos + idy * n + idx];
		__syncthreads();
		for (int p = 0; p < 8; p++)
			bs[wb][32 * p + tyw * 16 + txw] = B[2 * p * n + Btile_pos + tyw * n + txw];
		__syncwarp();

		nvcuda::wmma::load_matrix_sync(a_frag, &as[0], 16);
		nvcuda::wmma::load_matrix_sync(b_frag, &bs[wb][0], 16);
		nvcuda::wmma::mma_sync(c_frag, a_frag, b_frag, c_frag);

		nvcuda::wmma::load_matrix_sync(a_frag, &as[8], 16);
		nvcuda::wmma::load_matrix_sync(b_frag, &bs[wb][8 * 16], 16);
		nvcuda::wmma::mma_sync(c_frag, a_frag, b_frag, c_frag);

		__syncthreads();
		Atile_pos += 16;
		Btile_pos += 16 * n;
	}
	nvcuda::wmma::store_matrix_sync(&D[(cy * n + cx) * 16], c_frag, n, nvcuda::wmma::mem_row_major);
}

void launch_jacobi_cc(dim3 grid_dim, dim3 block_dim, float* D, float* A, float* B, float* C, int n) {
	jacobi_cc <<<grid_dim, block_dim >>> (D, A, B, C, n);
}

void launch_jacobi_tc(dim3 grid_dim, dim3 block_dim, float* D, float* A, float* B, float* C, int n) {
	jacobi_tc <<<grid_dim, block_dim >>> (D, A, B, C, n);
}