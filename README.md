<h1 align="center">JPEG Encoder</h1>
<p align="center"><i> A JPEG Encoder implementation with OpenCL.</i></p>

**Authors**: Manpa Barman , Pritom Gogoi, Shoumik Dey (*The authors are alphabetically ordered.*)

**Course:** *High Performance Programming with Graphic Cards*

**Semester:** *Summer semester 2023*

## :bulb: Project Abstract
A Joint Photographic Experts Group (JPEG) encoder is a crucial component in digital image processing, designed to efficiently compress and encode images in the JPEG standard. This project aims to implement the JPEG encoding process in the C++ programming language, comparing how the various stages can be accelerated on a Graphics Processing Unit (GPU). The various stages of the JPEG encoding process, color space conversion, downsampling or chroma subsampling, level shifting, discrete cosine transform, quantization, zigzag scan, run length encoding and Huffman encoding are implemented as C++ functions as well as Open Computing Language (OpenCL) kernels. Both the CPU and GPU implementations are compared and the speedup are evaluated which justifies the parallel processing capabilities of modern GPUs. The results also support the presumption that the stages of the JPEG encoding process are highly parallelizable.


## Software and Hardware Requirements
1. CMake
2. OpenCL
3. Boost Library
4. G++ or Clang compiler
5. OpenCL compatible and supported GPU.

## Directory Structure
| Path | Description |
| --- | --- |
| `src` | Contains the source code of the project. |
| `src/utils.hpp` | Contains the header files of the utility Functions. |
| `lib` | Contains the library files of the project. |
| `lib/utils.cpp` | Contains the implementation of the utility Functions. |
| `lib/huffman.hpp` | Contains the huffman tables according to the JPEG standard. |
| `lib/quantization.hpp` | Contains the quantization tables according to the JPEG standard. |
| `lib/OpenCLProject_JpegEncoder.cpp` | Contains the main function of the project for excecuting both the CPU and GPU implementations. |
| `lib/OpenCLProject_JpegEncoder.cl` | Contains the OpenCL kernels for the GPU implementation. |

## Running the code

1. Create a `build` folder in the root directory (src) of the project:
   `mkdir build`
2. Go to `build` folder:
   `cd build`
3. Run cmake inside the build folder:
   `cmake ..`
4. Compile the code with make command:
   `make` 
5. Run the executable file which will be named by the project name which you have
   written in CMakeLists.txt
   For example: ./jpeg-encoder-opencl in this project.

