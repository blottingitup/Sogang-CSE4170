# CSEG483 at Sogang University
[![Static Badge](https://img.shields.io/badge/C%2B%2B-blue?logo=c%2B%2B)](https://cppreference.com/w/Main_Page.html)
[![Static Badge](https://img.shields.io/badge/CUDA%20Toolkit-13.1-brightgreen?logo=nvidia)](https://docs.nvidia.com/cuda/archive/13.1.1/)  
  
This repository contains the code submitted for **Introduction to GPU Programming (CSEG483)**.  
The main objective is to enhance the given base code.
Direct copy is not recommended, but taking reference to *whisk up* some code of your own will be just fine.  

**HW1: 1D Gaussian Filtering**
* Write a kernel function for 1D Gaussian filtering using a grid-stride loop.
* After issuing a dummy call, measure the average execution time of the kernel function over multiple iterations.
* Experiment with variables that affect the overall performance, and provide a summary and analysis of the results.  

**HW2: 1D Gaussian Filtering with Wrap Condition**
* Write a kernel function for 1D Gaussian filtering using the wrap-around condition.
* Also write two additional kernel functions: one using shared memory and the other using a grid-stride loop.
* Use Nsight Compute to analyze register and shared memory usage, achieved warp occupancy and other metrics(L1, L2 cache hit rate, warp state statistics) relative to thread block dimensions.

**HW3: Implementation of Jacobi Iteration using CUDA Cores and Tensor Cores**
* Write two kernel function for Jacobi Iteration: one using CUDA Core MMA and the other using Tensor Core WMMA.
* Analyze execution time and precision for all implementations.
