 /* 
 * 
 * HW1: 1D Gaussian Filtering   
 *
 */

//
// Written for Sogang University CSEG483/CSE5483
//

#define NUMBER_OF_TESTS_ON_GPU 10
#define NUMBER_OF_ELEMENTS (1 << 30)
#define THREADS_PER_BLOCK 256
#define GRID_REDUCTION_FACTOR 1

#include <stdio.h>
#include <cmath>
#include <random>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include "measure_host_time_3.h"

const float GAUSSIAN_KERNEL_1D[36] = {
    1.0f,
    0.25f, 0.5f, 0.25f,
    0.0625f, 0.2500f, 0.3750f, 0.2500f, 0.0625f,
	0.015625f, 0.09375f, 0.234375f, 0.3125f, 0.234375f, 0.09375f, 0.015625f,
    0.0039f, 0.0312f, 0.1094f, 0.2188f, 0.2734f, 0.2188f, 0.1094f, 0.0312f, 0.0039f,
    0.000977f, 0.009766f, 0.043945f, 0.117188f, 0.205078f, 0.246094f, 0.205078f, 0.117188f, 0.043945f, 0.009766f, 0.000977f
};

void gen_uniform_distribution_int(int* array, int a, int b, int N) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(a, b);

    for (int i = 0; i < N; i++)  
        array[i] = dist(gen);
}

#define _MAX_KERNEL_SIZE_DEVICE 11
__constant__ float d_kernel_weights[_MAX_KERNEL_SIZE_DEVICE];

void apply_1D_Gaussian_host(const int* image_in, int* image_out, int N, int kernel_width) {
    int half_width = kernel_width / 2;
    const float* kernel = GAUSSIAN_KERNEL_1D + half_width * half_width;

    for (int i = 0; i < N; i++) {
        float sum = 0.0f;

        for (int j = -half_width; j <= half_width; j++) {
            int index = i + j;
            // boarder handling: mirror reflection
            if (index < 0) {
                index = -index;
            }
            else if (index >= N) {
                index = 2 * N - 2 - index;
            }
            sum += image_in[index] * kernel[j + half_width];
        }
		image_out[i] = (int)(sum + 0.5f); // looks faster than std::round(), and it is sufficient for this application  
        // image_out[i] = static_cast<int>(std::round(sum));
    }
}

// Kernel function for 1D Gaussian filtering
__global__ void apply_1D_Gaussian_device(const int* image_in, int* image_out, int N, int kernel_width) {
    int half_width = kernel_width / 2;
    int start_i = blockDim.x * blockIdx.x + threadIdx.x;  // Starting position of threads
    int stride_i = gridDim.x * blockDim.x;  // stride

    for (int i = start_i; i < N; i += stride_i) {
        float sum = 0.5f;
        for (int j = -half_width; j <= half_width; j++) {
            int index = i + j;
            if (index < 0) index = -index;
            else if (index >= N) index = 2 * N - 2 - index;
            sum += image_in[index] * d_kernel_weights[j + half_width];
        }
        image_out[i] = (int)sum;
    }
}

// Check difference between calculations in host and device, considering floating point errors
int tot_failed = 0, tot_verify = 0;  // Total number of failed tests, total number of tests
bool verify_gaussian(const int* image1, const int* image2, int N) {
    tot_verify++;
    for (int i = 0; i < N; i++) {
        if (abs(image1[i] - image2[i]) > 1) {
            fprintf(stderr, "*** Mismatch detected at index %d. Host=%d, Device=%d\n");
            tot_failed++;
            return false;
        }
    }
    return true;
}

int main(void) {
	int n_elements = NUMBER_OF_ELEMENTS;
    size_t size = (size_t)n_elements * sizeof(int);
   
    // Allocate the host memory
    int* h_image_in = (int*)malloc(size);
    int* h_image_out_host = (int*)malloc(size);
    int* h_image_out_device = (int*)malloc(size);
    if (h_image_in == NULL || h_image_out_host == NULL || h_image_out_device == NULL) {
        fprintf(stderr, "^^^ Failed to allocate host vectors!\n");
        exit(EXIT_FAILURE);
    }

    gen_uniform_distribution_int(h_image_in, 0, 255, n_elements);
    // Allocate the device memory
    int* d_image_in = NULL;
    cudaMalloc((void**)&d_image_in, size);
    int* d_image_out = NULL;
    cudaMalloc((void**)&d_image_out, size);

    // Experiment 1: Change filter width and compare host and device
    fprintf(stdout, "*********************************************************************************\n");
    fprintf(stdout, "Experiment 1: Change filter width and compare host and device\n");
    fprintf(stdout, "Control Variable: NUMBER_OF_ELEMENTS = (1 << 26), GRID_REDUCTION_FACTOR = 1\n");
    fprintf(stdout, "Manipulated: kernel_width = (1, 3, 5, 7, 9, 11)\n");
    fprintf(stdout, "*********************************************************************************\n");

    // Define new default size of elements for Experiment 1
    n_elements = (1 << 26);
    size = n_elements * sizeof(int);

 	// int kernel_width = GF_KERNEL_WIDTH; // 1, 3, 5, 7, 9, or 11  
    for (int kernel_width = 1; kernel_width <= _MAX_KERNEL_SIZE_DEVICE; kernel_width += 2) {
        fprintf(stdout, "\n[1D Gaussian filtering of image of %d pixels(KERNEL WIDTH = %d)]",
            n_elements, kernel_width);
        // Add vectors on the host
        CHECK_TIME_START(_start, _freq);
        apply_1D_Gaussian_host(h_image_in, h_image_out_host, n_elements, kernel_width);
        CHECK_TIME_END(_start, _end, _freq, _compute_time);
        fprintf(stdout, "\n^^^ Time to filter an image of %d pixels on the host = %.3f(ms)\n",
            n_elements, _compute_time);
		float host_time = _compute_time;

		cudaFree(0);  
        // Call to cudaFree(0) is not necessary, but it is a good practice to initialize the CUDA runtime 
        // before starting the timer, to avoid including the initialization time in the measurement.   
        int half_width = kernel_width / 2;
        // Copy the kernel weights to the constant memory on the device
        cudaMemcpyToSymbol(d_kernel_weights,
            (const float*)GAUSSIAN_KERNEL_1D + half_width * half_width,
            kernel_width * sizeof(float));
        
        cudaDeviceSynchronize(); // actually not necessary here, but it is a good practice to ensure that all preceding CUDA calls have completed before proceeding, especially when measuring time.
        // Launch the 1D GF kernel
        int threads_per_block = THREADS_PER_BLOCK;
        int blocks_per_grid = (n_elements + threads_per_block - 1) / threads_per_block;
        int blocks_per_grid_reduced = blocks_per_grid / GRID_REDUCTION_FACTOR;
        fprintf(stdout, "\n^^^ CUDA kernel launch with %d(%d) blocks of %d threads\n", blocks_per_grid_reduced, blocks_per_grid,
            threads_per_block);

		// Do something here!!!
        cudaMemcpy(d_image_in, h_image_in, size, cudaMemcpyHostToDevice);  // input image

        // Dummy call
        // Always use 'blocks_per_grid_reduced', GRID_REDUCTION_FACTOR = 1 when not using grid-stride loop
        apply_1D_Gaussian_device << <blocks_per_grid_reduced, threads_per_block >> > (d_image_in, d_image_out, n_elements, kernel_width);  // kernel launch
        cudaDeviceSynchronize();

        // Actual time measure on GPU
        CHECK_TIME_START(_start, _freq);
        for (int i = 0; i < NUMBER_OF_TESTS_ON_GPU; i++) {
            apply_1D_Gaussian_device << <blocks_per_grid_reduced, threads_per_block >> > (d_image_in, d_image_out, n_elements, kernel_width);
        }
        cudaDeviceSynchronize();
        CHECK_TIME_END(_start, _end, _freq, _compute_time);

        float device_time = _compute_time / NUMBER_OF_TESTS_ON_GPU; // Average time on GPU
        fprintf(stdout, "^^^ Time to filter an image of %d pixels on the device = %.3f(ms)\n", n_elements, device_time);
        fprintf(stdout, "^^^ Speedup: %.2fx\n", host_time / device_time);  // Speedup rate

        // Verify that the result from the device is correct
        // How?
        cudaMemcpy(h_image_out_device, d_image_out, size, cudaMemcpyDeviceToHost);  // Copy device result back to host
        if(verify_gaussian(h_image_out_host, h_image_out_device, n_elements))
            fprintf(stdout, "^^^ Test PASSED\n");
        else fprintf(stdout, "^^^ Test FAILED\n");
    }

    // Experiment 2: Change data size and threads per block (no grid-stride loop yet)
    fprintf(stdout, "\n*********************************************************************************\n");
    fprintf(stdout, "Experiment 2: Change data size and threads per block (no grid-stride loop yet)\n");
    fprintf(stdout, "Control Variable: kernel_width = 11, GRID_REDUCTION_FACTOR = 1\n");
    fprintf(stdout, "Manipulated: NUMBER_OF_ELEMENTS = (1 << 22, 1 << 26, 1 << 30)\n");
    fprintf(stdout, "             THREADS_PER_BLOCK = (128, 256, 1024)\n");
    fprintf(stdout, "*********************************************************************************\n");

    // Number of elements: 1 << 22, 1 << 26, 1 << 30
    // Threads per block: 128, 256, 1024
    for (int i = 22; i <= 30; i += 4) {
        int tpb[3] = { 128, 256, 1024 };  // threads per block
        int n_elements = (1 << i);
        size_t size = (size_t)n_elements * sizeof(int);

        fprintf(stdout, "\nRunning host code for verification...\n");
        apply_1D_Gaussian_host(h_image_in, h_image_out_host, n_elements, 11);
        fprintf(stdout, "Finished!!\n");

        for (int j = 0; j < 3; j++) {
            fprintf(stdout, "\n^^^ Total data size: %d, threads per block: %d\n", n_elements, tpb[j]);

            // No need to copy kernel_width to kernel, the kernel width is fixed to 11 from now on

            cudaDeviceSynchronize();
            int threads_per_block = tpb[j];
            int blocks_per_grid = (n_elements + threads_per_block - 1) / threads_per_block;
            int blocks_per_grid_reduced = blocks_per_grid / GRID_REDUCTION_FACTOR;
            fprintf(stdout, "^^^ CUDA kernel launch with %d(%d) blocks of %d threads\n", blocks_per_grid_reduced, blocks_per_grid,
                threads_per_block);

            cudaMemcpy(d_image_in, h_image_in, size, cudaMemcpyHostToDevice);

            apply_1D_Gaussian_device << <blocks_per_grid_reduced, threads_per_block >> > (d_image_in, d_image_out, n_elements, 11);
            cudaDeviceSynchronize();

            CHECK_TIME_START(_start, _freq);
            for (int k = 0; k < NUMBER_OF_TESTS_ON_GPU; k++) {
                apply_1D_Gaussian_device << <blocks_per_grid_reduced, threads_per_block >> > (d_image_in, d_image_out, n_elements, 11);
            }
            cudaDeviceSynchronize();
            CHECK_TIME_END(_start, _end, _freq, _compute_time);

            float device_time = _compute_time / NUMBER_OF_TESTS_ON_GPU;
            fprintf(stdout, "^^^ Time to filter an image of %d pixels on the device = %.3f(ms)\n", n_elements, device_time);

            
            cudaMemcpy(h_image_out_device, d_image_out, size, cudaMemcpyDeviceToHost);
            if (verify_gaussian(h_image_out_host, h_image_out_device, n_elements))
                fprintf(stdout, "^^^ Test PASSED\n");
            else fprintf(stdout, "^^^ Test FAILED\n");
        }
    }

    // Experiment 3: Change data size and test grid-stride loop
    fprintf(stdout, "\n*********************************************************************************\n");
    fprintf(stdout, "Experiment 3: Change data size and test grid-stride loop\n");
    fprintf(stdout, "Control Variable: kernel_width = 11, THREADS_PER_BLOCK = 256\n");
    fprintf(stdout, "Manipulated: NUMBER_OF_ELEMENTS = (1 << 22, 1 << 26, 1 << 30)\n");
    fprintf(stdout, "             GRID_REDUCTION_FACTOR = (1, 2, 4, 8, 16)\n");
    fprintf(stdout, "*********************************************************************************\n");

    // Number of elements: 1 << 22, 1 << 26, 1 << 30
    // Grid reduction factor: 1, 2, 4, 8, 16
    for (int i = 22; i <= 30; i += 4) {
        int grf[5] = { 1, 2, 4, 8, 16 };  // grid reduction factor
        int n_elements = (1 << i);
        size_t size = (size_t)n_elements * sizeof(int);

        fprintf(stdout, "\nRunning host code for verification...\n");
        apply_1D_Gaussian_host(h_image_in, h_image_out_host, n_elements, 11);
        fprintf(stdout, "Finished!!\n");

        for (int j = 0; j < 5; j++) {
            fprintf(stdout, "\n^^^ Total data size: %d, grid reduction factor: %d\n", n_elements, grf[j]);

            cudaDeviceSynchronize();
            int threads_per_block = THREADS_PER_BLOCK;
            int blocks_per_grid = (n_elements + threads_per_block - 1) / threads_per_block;
            int blocks_per_grid_reduced = blocks_per_grid / grf[j];
            fprintf(stdout, "^^^ CUDA kernel launch with %d(%d) blocks of %d threads\n", blocks_per_grid_reduced, blocks_per_grid,
                threads_per_block);

            cudaMemcpy(d_image_in, h_image_in, size, cudaMemcpyHostToDevice);

            apply_1D_Gaussian_device << <blocks_per_grid_reduced, threads_per_block >> > (d_image_in, d_image_out, n_elements, 11);
            cudaDeviceSynchronize();

            CHECK_TIME_START(_start, _freq);
            for (int k = 0; k < NUMBER_OF_TESTS_ON_GPU; k++) {
                apply_1D_Gaussian_device << <blocks_per_grid_reduced, threads_per_block >> > (d_image_in, d_image_out, n_elements, 11);
            }
            cudaDeviceSynchronize();
            CHECK_TIME_END(_start, _end, _freq, _compute_time);

            float device_time = _compute_time / NUMBER_OF_TESTS_ON_GPU;
            fprintf(stdout, "^^^ Time to filter an image of %d pixels on the device = %.3f(ms)\n", n_elements, device_time);


            cudaMemcpy(h_image_out_device, d_image_out, size, cudaMemcpyDeviceToHost);
            if (verify_gaussian(h_image_out_host, h_image_out_device, n_elements))
                fprintf(stdout, "^^^ Test PASSED\n");
            else fprintf(stdout, "^^^ Test FAILED\n");
        }
    }

	// Free device memory   
    cudaFree(d_image_in);
	cudaFree(d_image_out);  

    // Free host memory
    free(h_image_in);
    free(h_image_out_host);
    free(h_image_out_device);

    fprintf(stdout, "\n\n^^^ Total number of tests: %d\n", tot_verify);
    fprintf(stdout, "^^^ Passed: %d, Failed: %d\n", tot_verify - tot_failed, tot_failed);
    fprintf(stdout, "\n^^^ Done\n");
    return 0;
}
