#pragma once

#include <cuda_runtime_api.h>
#include <cusolverSp.h>
#include <cusparse.h>
#include <stdint.h>
#include "rxmesh/util/log.h"

#ifdef USE_CUDSS
#include <cudss.h>
#endif

namespace rxmesh {

typedef uint8_t flag_t;

// TRANSPOSE_ITEM_PER_THREAD
constexpr uint32_t TRANSPOSE_ITEM_PER_THREAD = 9;

constexpr int MAX_OVERLAP_CAVITIES = 4;

// used for integer rounding
#define DIVIDE_UP(num, divisor) (num + divisor - 1) / (divisor)

#define ROUND_UP_TO_NEXT_MULTIPLE(num, mult) (DIVIDE_UP(num, mult) * mult)

// unsigned 64-bit
#define INVALID64 0xFFFFFFFFFFFFFFFFu

// unsigned 32-bit
#define INVALID32 0xFFFFFFFFu

// unsigned 16-bit
#define INVALID16 0xFFFFu

// unsigned 8-bit
#define INVALID8 0xFFu

// 4-bit
#define INVALID4 0xFu

#define WARP_SIZE 32

#define BYTES_TO_MEGABYTES(bytes) (double(bytes) / double(1024.0 * 1024.0))

// http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
#define STRINGIFY(x) TOSTRING(x)
#define TOSTRING(x) #x


#ifndef myAssert
#ifdef __CUDA_ARCH__
#define myAssert(condition)                                                   \
    if (!(condition)) {                                                       \
        printf(                                                               \
            "**********Assertion failed: %s, file %s, line %d, blockId= %d, " \
            "thread= %d\n",                                                   \
            #condition,                                                       \
            __FILE__,                                                         \
            __LINE__,                                                         \
            blockIdx.x,                                                       \
            threadIdx.x);                                                     \
        asm("trap;");                                                         \
    }
#else
#define myAssert(condition)                                        \
    if (!(condition)) {                                            \
        printf("**********Assertion failed: %s, file %s, line %d", \
               #condition,                                         \
               __FILE__,                                           \
               __LINE__);                                          \
    }
#endif
#endif


#ifndef CUDA_ERROR
inline void HandleError(cudaError_t err, const char* file, int line)
{
    // Error handling micro, wrap it around function whenever possible
    if (err != cudaSuccess) {
        Log::get_logger()->error("Line {} File {}", line, file);
        Log::get_logger()->error("CUDA ERROR: {}", cudaGetErrorString(err));

        exit(EXIT_FAILURE);
    }
}
#define CUDA_ERROR(err) (HandleError(err, __FILE__, __LINE__))
#endif

#ifndef CUSPARSE_ERROR
inline void cusparseHandleError(cusparseStatus_t status,
                                const char*      file,
                                const int        line)
{
    if (status != CUSPARSE_STATUS_SUCCESS) {
        Log::get_logger()->error("Line {} File {}", line, file);
        Log::get_logger()->error("CUSPARSE ERROR: {}",
                                 cusparseGetErrorString(status));

        exit(EXIT_FAILURE);
    }
    return;
}
#define CUSPARSE_ERROR(err) (cusparseHandleError(err, __FILE__, __LINE__))
#endif

#ifndef CUSOLVER_ERROR
static inline void cusolverHandleError(cusolverStatus_t status,
                                       const char*      file,
                                       const int        line)
{
    if (status != CUSOLVER_STATUS_SUCCESS) {
        auto cusolverGetErrorString = [](cusolverStatus_t status) {
            switch (status) {
                case CUSOLVER_STATUS_SUCCESS:
                    return "CUSOLVER_STATUS_SUCCESS";
                case CUSOLVER_STATUS_NOT_INITIALIZED:
                    return "CUSOLVER_STATUS_NOT_INITIALIZED";
                case CUSOLVER_STATUS_ALLOC_FAILED:
                    return "CUSOLVER_STATUS_ALLOC_FAILED";
                case CUSOLVER_STATUS_INVALID_VALUE:
                    return "CUSOLVER_STATUS_INVALID_VALUE";
                case CUSOLVER_STATUS_ARCH_MISMATCH:
                    return "CUSOLVER_STATUS_ARCH_MISMATCH";
                case CUSOLVER_STATUS_EXECUTION_FAILED:
                    return "CUSOLVER_STATUS_EXECUTION_FAILED";
                case CUSOLVER_STATUS_INTERNAL_ERROR:
                    return "CUSOLVER_STATUS_INTERNAL_ERROR";
                case CUSOLVER_STATUS_MATRIX_TYPE_NOT_SUPPORTED:
                    return "CUSOLVER_STATUS_MATRIX_TYPE_NOT_SUPPORTED";
                default:
                    return "UNKNOWN_ERROR";
            }
        };

        Log::get_logger()->error("Line {} File {}", line, file);
        Log::get_logger()->error("CUSOLVER ERROR: {}",
                                 cusolverGetErrorString(status));

        exit(EXIT_FAILURE);
    }
    return;
}
#define CUSOLVER_ERROR(err) (cusolverHandleError(err, __FILE__, __LINE__))
#endif


#ifndef CUBLAS_ERROR
inline void cublasHandleError(cublasStatus_t status,
                              const char*    file,
                              const int      line)
{
    if (status != CUBLAS_STATUS_SUCCESS) {
        Log::get_logger()->error("Line {} File {}", line, file);
        Log::get_logger()->error("CUBLAS ERROR: {}",
                                 cublasGetStatusString(status));

        exit(EXIT_FAILURE);
    }
    return;
}
#define CUBLAS_ERROR(err) (cublasHandleError(err, __FILE__, __LINE__))
#endif


#ifdef USE_CUDSS
inline void cudssHandleError(cudssStatus_t status,
                             const char*   file,
                             const int     line)
{

    if (status != CUDSS_STATUS_SUCCESS) {
        auto cudssGetErrorString = [](cudssStatus_t status) {
            switch (status) {
                case CUDSS_STATUS_SUCCESS:
                    return "CUDSS_STATUS_SUCCESS";
                case CUDSS_STATUS_NOT_INITIALIZED:
                    return "CUDSS_STATUS_NOT_INITIALIZED";
                case CUDSS_STATUS_ALLOC_FAILED:
                    return "CUDSS_STATUS_ALLOC_FAILED";
                case CUDSS_STATUS_INVALID_VALUE:
                    return "CUDSS_STATUS_INVALID_VALUE";
                case CUDSS_STATUS_NOT_SUPPORTED:
                    return "CUDSS_STATUS_NOT_SUPPORTED";
                case CUDSS_STATUS_EXECUTION_FAILED:
                    return "CUDSS_STATUS_EXECUTION_FAILED";
                case CUDSS_STATUS_INTERNAL_ERROR:
                    return "CUDSS_STATUS_INTERNAL_ERROR";
                default:
                    return "UNKNOWN_ERROR";
            }
        };

        Log::get_logger()->error("Line {} File {}", line, file);
        Log::get_logger()->error("cuDSS ERROR: {}",
                                 cudssGetErrorString(status));

        exit(EXIT_FAILURE);
    }
}
#define CUDSS_ERROR(err) (cudssHandleError(err, __FILE__, __LINE__))
#endif


// GPU_FREE
#define GPU_FREE(ptr)              \
    if (ptr != nullptr) {          \
        CUDA_ERROR(cudaFree(ptr)); \
        ptr = nullptr;             \
    }

// Taken from https://stackoverflow.com/a/12779757/1608232
#if defined(__CUDACC__)  // NVCC
#define ALIGN(n) __align__(n)
#elif defined(__GNUC__)  // GCC
#define ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)  // MSVC
#define ALIGN(n) __declspec(align(n))
#else
#error "Please provide a definition for MY_ALIGN macro for your host compiler!"
#endif


// Taken from
// https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#extended-lambda-traits
#define IS_D_LAMBDA(X) __nv_is_extended_device_lambda_closure_type(X)
#define IS_HD_LAMBDA(X) __nv_is_extended_host_device_lambda_closure_type(X)

}  // namespace rxmesh