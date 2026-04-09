# CSE4170 and CSEG483 at Sogang University
[![Static Badge](https://img.shields.io/badge/C%2B%2B-blue?logo=c%2B%2B)](https://cppreference.com/w/Main_Page.html)
[![Static Badge](https://img.shields.io/badge/github-freeglut-blue?logo=github)](https://github.com/freeglut/freeglut)
[![Static Badge](https://img.shields.io/badge/github-GLM-orange?logo=github)](https://github.com/g-truc/glm)
[![Static Badge](https://img.shields.io/badge/CUDA%20Toolkit-13.1-brightgreen?logo=nvidia)](https://docs.nvidia.com/cuda/archive/13.1.1/)  

**CSE4170: Compatible with freeglut 3.8.0.**  
  
This repository contains the code submitted for  
**Introduction to Computer Graphics (CSE4170)** and **Introduction to GPU Programming (CSEG483)**.  
Direct copy is not recommended, but taking reference to *whisk up* some code of your own will be just fine.  

---
### CSE4170
**HW1: Polygon Editor Development with GLUT Toolkit**  
* Switch between Standby, Creation, Selection and Animation modes.
* A maximum of 10 polygons can exist at once; polygons must be distinguishable by their colored edges.
* Creation Mode: Press 's' in Standby Mode to enter. Place vertices using SHIFT + LMB and press 'e' to finish; the polygon's centroid will then appear.
* Selection Mode: Click the LMB on a polygon's centroid in Standby Mode. Only one polygon can be selected at once. The selected polygon can be deleted with 'c', moved with the LMB, rotated with the RMB, scaled with the mouse wheel, and flipped with the arrow keys.
* Animation Mode: Press 'a' in Selection Mode to enter. The polygon will rotate and scale simultaneously.

---
### CSEG483
**HW1: 1D Gaussian Filtering**
* Write a kernel function for 1D Gaussian filtering using grid-stride loop.
* After issuing a dummy call, measure the average execution time of the kernel function over multiple iterations.
* Verify that both the host and kernel functions produce the same results.
* Experiment with variables that affect the overall performance, and provide a summary and analysis of the results.
