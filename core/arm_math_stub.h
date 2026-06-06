//
// arm_math_stub.h
//
// Minimal stub replacing CMSIS arm_math.h for non-CM4 targets (AArch64).
// Implements only the arm_biquad_cascade_df1_f32 subset used by CloudSeedCore.
// MIT licensed — stub only; no CMSIS source is reproduced.
//
#pragma once
#include <cstdint>
#include <cstring>

typedef float         float32_t;
typedef uint32_t      uint32_t;

// Direct Form I biquad cascade instance.
// Coefficients layout per stage: { b0, b1, b2, a1, a2 } (a1/a2 stored negated by CMSIS).
typedef struct {
    uint32_t           numStages;
    float32_t         *pState;       // 2 × numStages state values
    const float32_t   *pCoeffs;      // 5 × numStages coefficients
} arm_biquad_casd_df1_inst_f32;

inline void arm_biquad_cascade_df1_init_f32(arm_biquad_casd_df1_inst_f32 *S,
                                             uint32_t numStages,
                                             const float32_t *pCoeffs,
                                             float32_t *pState)
{
    S->numStages = numStages;
    S->pCoeffs   = pCoeffs;
    S->pState    = pState;
    memset(pState, 0, sizeof(float32_t) * 2 * numStages);
}

inline void arm_biquad_cascade_df1_f32(const arm_biquad_casd_df1_inst_f32 *S,
                                        const float32_t *pSrc,
                                        float32_t       *pDst,
                                        uint32_t         blockSize)
{
    const float32_t *pCoeffs = S->pCoeffs;
    float32_t       *pState  = S->pState;

    for (uint32_t stage = 0; stage < S->numStages; stage++)
    {
        float32_t b0 = pCoeffs[0], b1 = pCoeffs[1], b2 = pCoeffs[2];
        float32_t a1 = pCoeffs[3], a2 = pCoeffs[4];  // CMSIS stores negated
        pCoeffs += 5;

        float32_t s0 = pState[0], s1 = pState[1];
        const float32_t *src = (stage == 0) ? pSrc : pDst;

        for (uint32_t n = 0; n < blockSize; n++)
        {
            float32_t x   = src[n];
            float32_t y   = b0 * x + s0;
            s0 = b1 * x - a1 * y + s1;
            s1 = b2 * x - a2 * y;
            pDst[n] = y;
        }

        pState[0] = s0;
        pState[1] = s1;
        pState += 2;
    }
}
