#include <stdio.h>
#include <stdlib.h>
#include "ipp.h"

// 定义级联数和每个Biquad的系数数量 (b0, b1, b2, a1, a2)
#define NUM_STAGES 4
#define COEFFS_PER_STAGE 5
#define TAPS_LENGTH (NUM_STAGES * COEFFS_PER_STAGE)

// 假设的输入信号长度
#define DATA_LENGTH 100

// 检查 IPP 函数调用状态的宏
#define CHECK_STATUS(status, msg) \
    if (status != ippStsNoErr) { \
        fprintf(stderr, "IPP Error: %s failed with status %d\n", msg, status); \
        goto cleanup; \
    }

static void run_biquad_cascade_filter() {
    IppStatus status = ippStsNoErr;
    
    // --- 1. 定义和准备系数 ---
    // 系数顺序: [b0, b1, b2, a1, a2] for stage 1, 
    //           [b0, b1, b2, a1, a2] for stage 2, ...
    // !!! 注意：a1 和 a2 是反馈系数，通常需要是负反馈（在公式 y[n] = ... - a1*y[n-1] - a2*y[n-2] 中）!!!
    // 这里使用一组假设的系数进行演示。在实际应用中，您需要使用滤波器设计工具（如 MATLAB 或 SciPy）来生成它们。
    Ipp32f pTaps[TAPS_LENGTH] = {
        // Stage 1 (Biquad 1) - 假设的低通滤波器
        0.1f, 0.2f, 0.1f,   -1.5f, 0.8f, 
        // Stage 2 (Biquad 2)
        0.2f, 0.4f, 0.2f,   -1.0f, 0.5f,
        // Stage 3 (Biquad 3)
        0.3f, 0.6f, 0.3f,   -0.8f, 0.3f,
        // Stage 4 (Biquad 4)
        0.4f, 0.8f, 0.4f,   -0.5f, 0.1f
    };

    // --- 2. 准备输入和输出数据 ---
    // pSrc (输入) 和 pDst (输出) 都是 Ipp32f (float) 类型
    Ipp32f *pSrc = ippsMalloc_32f(DATA_LENGTH);
    Ipp32f *pDst = ippsMalloc_32f(DATA_LENGTH);
    
    if (!pSrc || !pDst) {
        fprintf(stderr, "Memory allocation failed for input/output data.\n");
        goto cleanup;
    }

    // 初始化输入数据 (例如，一个单位脉冲或随机数据)
    ippsSet_32f(0.0f, pSrc, DATA_LENGTH);
    pSrc[0] = 1.0f; // 脉冲输入
    
    // --- 3. 获取状态结构所需大小 ---
    int stateSize = 0;
    status = ippsIIRGetStateSize_BiQuad_32f(NUM_STAGES, &stateSize);
    CHECK_STATUS(status, "ippsIIRGetStateSize_BiQuad_32f");

    // --- 4. 分配状态缓冲区和状态结构 ---
    Ipp8u* pBuffer = ippsMalloc_8u(stateSize);
    IppsIIRState_32f* pState = NULL;
    
    if (!pBuffer) {
        fprintf(stderr, "Memory allocation failed for state buffer.\n");
        goto cleanup;
    }

    // --- 5. 初始化 IIR 滤波器状态结构 ---
    // pDlyLine = NULL 表示使用默认的零延迟线初始化
    Ipp32f* pDlyLine = NULL; 
    
    status = ippsIIRInit_BiQuad_32f(
        &pState,           // [out] 指向状态结构的指针
        pTaps,             // [in]  系数数组
        NUM_STAGES,        // [in]  级联数 (4)
        pDlyLine,          // [in]  初始延迟线（可选，设为 NULL）
        pBuffer            // [in]  工作缓冲区
    );
    CHECK_STATUS(status, "ippsIIRInit_BiQuad_32f");

    // --- 6. 执行滤波 ---
    status = ippsIIR_32f(
        pSrc,              // [in]  输入向量
        pDst,              // [out] 输出向量
        DATA_LENGTH,       // [in]  样本数
        pState             // [in/out] 滤波器状态结构
    );
    CHECK_STATUS(status, "ippsIIR_32f");

    // --- 7. 验证结果 (打印前几个输出) ---
    printf("Filter execution successful.\n");
    printf("First 10 output samples (Ipp32f):\n");
    for (int i = 0; i < 10 && i < DATA_LENGTH; i++) {
        printf("y[%d] = %f\n", i, pDst[i]);
    }

// --- 8. 清理资源 ---
cleanup:
    ippsFree(pSrc);
    ippsFree(pDst);
    ippsFree(pBuffer); // 释放工作缓冲区，pState 指向其中的一部分
    // ippsIIRFree(pState) 用于 ippsIIRInitAlloc_BiQuad 初始化的状态，
    // 对于 ippsIIRInit_BiQuad，只需释放 pBuffer。
}

int main() {
    // 设置 IPP 运行环境 (可选)
    // ippSetNumThreads(4); 
    
    run_biquad_cascade_filter();

    printf("\nProgram finished.\n");
    return 0;
}