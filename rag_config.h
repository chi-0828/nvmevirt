#ifndef _RAG_CONFIG_H
#define _RAG_CONFIG_H


// ==============================================================================
// [通用參數]
// ==============================================================================
// [RAG CONFIG]
#define K_TOP_RESULTS       100
#define CHUNK_SIZE_BYTES    4096 
#define RAG_RESULT_SIZE     (K_TOP_RESULTS * CHUNK_SIZE_BYTES)

// ==============================================================================
// [實驗選擇區] 請在此處 "取消註解" 您要跑的那個資料集 (一次只選一個)
// ==============================================================================
// #define DATASET_NQ
// #define DATASET_HOTPOTQA
// #define DATASET_BIOASQ
#define DATASET_SIGNAL1M

#define OPT 1

// ==============================================================================
// [參數定義區] 自動根據上方選擇設定參數
// ==============================================================================

// 假設條件：Page Size = 16KB, Chunk Size = 4KB (即 1 Page 掃描 4 個 Chunks)
#define CHUNKS_PER_PAGE  4

#if defined(DATASET_NQ)
    // Natural Questions: 2.68M Chunks
    
    # if OPT
        #define TOTAL_PAGES_TO_SCAN    ((2680000 / CHUNKS_PER_PAGE) / 128)
    # else
        #define TOTAL_PAGES_TO_SCAN     (2680000 / CHUNKS_PER_PAGE)
    #endif
    #define TEST_QUERY_COUNT        3452                        // 真實 Query 數量

#elif defined(DATASET_HOTPOTQA)
    // HotpotQA: 5.23M Chunks
    # if OPT
        #define TOTAL_PAGES_TO_SCAN    ((5230000 / CHUNKS_PER_PAGE) / 128)
    # else
        #define TOTAL_PAGES_TO_SCAN     (5230000 / CHUNKS_PER_PAGE) // 1,307,500 Pages
    #endif
    #define TEST_QUERY_COUNT        7405

#elif defined(DATASET_BIOASQ)
    // BioASQ: 14.91M Chunks (最大)
    # if OPT
        #define TOTAL_PAGES_TO_SCAN    ((14910000 / CHUNKS_PER_PAGE) / 128)
    # else
        #define TOTAL_PAGES_TO_SCAN     (14910000 / CHUNKS_PER_PAGE) // 3,727,500 Pages
    # endif 

    #define TEST_QUERY_COUNT        500

#elif defined(DATASET_SIGNAL1M)
    // Signal-1M: 2.86M Chunks
    # if OPT
        #define TOTAL_PAGES_TO_SCAN    ((2860000 / CHUNKS_PER_PAGE) / 128)
    # else
        #define TOTAL_PAGES_TO_SCAN     (2860000 / CHUNKS_PER_PAGE) // 715,000 Pages
    # endif
    #define TEST_QUERY_COUNT        97

#else
    #error "Error: Please uncomment one DATASET_xxx in rag_config.h"
#endif



#endif // _RAG_CONFIG_H