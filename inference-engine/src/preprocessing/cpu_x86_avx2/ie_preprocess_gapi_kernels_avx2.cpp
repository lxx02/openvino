// Copyright (C) 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <algorithm>
#include <utility>

#include "ie_preprocess_gapi_kernels_avx2.hpp"

#include <immintrin.h>

#ifdef CV_AVX2
#undef CV_AVX2
#endif

#define CV_AVX2 1

#define CV_CPU_HAS_SUPPORT_SSE2 1

#ifdef CV_SIMD256
#undef CV_SIMD256
#endif

#define CV_SIMD256 1

#include "opencv_hal_intrin.hpp"
#include "ie_preprocess_gapi_kernels_simd_impl.hpp"

#if   !CV_SIMD256
#error CV_SIMD256 is required!
#endif

#include <cstring>

using namespace cv;

namespace InferenceEngine {

namespace gapi {

namespace kernels {

namespace avx {

void mergeRow_8UC2(const uint8_t in0[], const uint8_t in1[],
                   uint8_t out[], int length) {
    mergeRow_8UC2_Impl(in0, in1, out, length);
}

void mergeRow_8UC3(const uint8_t in0[], const uint8_t in1[],
                   const uint8_t in2[], uint8_t out[], int length) {
    mergeRow_8UC3_Impl(in0, in1, in2, out, length);
}

void mergeRow_8UC4(const uint8_t in0[], const uint8_t in1[], const uint8_t in2[],
                   const uint8_t in3[], uint8_t out[], int length) {
    mergeRow_8UC4_Impl(in0, in1, in2, in3, out, length);
}

void mergeRow_32FC2(const float in0[], const float in1[],
                    float out[], int length) {
    mergeRow_32FC2_Impl(in0, in1, out, length);
}

void mergeRow_32FC3(const float in0[], const float in1[], const float in2[],
                    float out[], int length) {
    mergeRow_32FC3_Impl(in0, in1, in2, out, length);
}

void mergeRow_32FC4(const float in0[], const float in1[],
                    const float in2[], const float in3[],
                    float out[], int length) {
    mergeRow_32FC4_Impl(in0, in1, in2, in3, out, length);
}

void splitRow_8UC2(const uint8_t in[], uint8_t out0[],
                   uint8_t out1[], int length) {
    splitRow_8UC2_Impl(in, out0, out1, length);
}

void splitRow_8UC3(const uint8_t in[], uint8_t out0[],
                   uint8_t out1[], uint8_t out2[], int length) {
    splitRow_8UC3_Impl(in, out0, out1, out2, length);
}

void splitRow_8UC4(const uint8_t in[], uint8_t out0[], uint8_t out1[],
                   uint8_t out2[], uint8_t out3[], int length) {
    splitRow_8UC4_Impl(in, out0, out1, out2, out3, length);
}

void splitRow_32FC2(const float in[], float out0[], float out1[], int length) {
    splitRow_32FC2_Impl(in, out0, out1, length);
}

void splitRow_32FC3(const float in[], float out0[], float out1[],
                    float out2[], int length) {
    splitRow_32FC3_Impl(in, out0, out1, out2, length);
}

void splitRow_32FC4(const float in[], float out0[], float out1[],
                    float out2[], float out3[], int length) {
    splitRow_32FC4_Impl(in, out0, out1, out2, out3, length);
}

void calculate_nv12_to_rgb(const  uchar **srcY,
                           const  uchar *srcUV,
                                  uchar **dstRGBx,
                                    int width) {
    calculate_nv12_to_rgb_impl(srcY, srcUV, dstRGBx, width);
}

void calculate_i420_to_rgb(const  uchar **srcY,
                           const  uchar *srcU,
                           const  uchar *srcV,
                                  uchar **dstRGBx,
                                    int width) {
    calculate_i420_to_rgb_impl(srcY, srcU, srcV, dstRGBx, width);
}

void calcRowArea_8U(uchar dst[], const uchar *src[], const Size& inSz,
                    const Size& outSz, Q0_16 yalpha, const MapperUnit8U &ymap,
                    int xmaxdf, const short xindex[], const Q0_16 xalpha[],
                    Q8_8 vbuf[]) {
    calcRowArea_impl(dst, src, inSz, outSz, yalpha, ymap, xmaxdf, xindex, xalpha, vbuf);
}

void calcRowArea_32F(float dst[], const float *src[], const Size& inSz,
                     const Size& outSz, float yalpha, const MapperUnit32F& ymap,
                     int xmaxdf, const int xindex[], const float xalpha[],
                     float vbuf[]) {
    calcRowArea_impl(dst, src, inSz, outSz, yalpha, ymap, xmaxdf, xindex, xalpha, vbuf);
}

template<int chanNum>
void calcRowLinear_8UC_Impl(std::array<std::array<uint8_t*, 4>, chanNum> &dst,
                            const uint8_t *src0[],
                            const uint8_t *src1[],
                            const short    alpha[],
                            const short    clone[],  // 4 clones of alpha
                            const short    mapsx[],
                            const short    beta[],
                                uint8_t    tmp[],
                             const Size    &inSz,
                             const Size    &outSz,
                                    int    lpi) {
    constexpr int half_nlanes = (v_uint8::nlanes / 2);
    const int shift = (half_nlanes / 4);

    if (4 == lpi) {
        GAPI_DbgAssert(inSz.width >= half_nlanes);

        v_uint8 shuf_mask1 = v_setr_s8(0, 8,  4, 12, 1, 9,  5, 13,
                                       2, 10, 6, 14, 3, 11, 7, 15,
                                       0, 8,  4, 12, 1, 9,  5, 13,
                                       2, 10, 6, 14, 3, 11, 7, 15);

        v_uint8 shuf_mask2 = v_setr_s8(0, 4, 8, 12, 2, 6, 10, 14,
                                       1, 5, 9, 13, 3, 7, 11, 15,
                                       0, 4, 8, 12, 2, 6, 10, 14,
                                       1, 5, 9, 13, 3, 7, 11, 15);

        v_uint8 shuf_mask3 = v_setr_s8(0, 1, 8,  9,  2, 3, 10, 11,
                                       4, 5, 12, 13, 6, 7, 14, 15,
                                       0, 1, 8,  9,  2, 3, 10, 11,
                                       4, 5, 12, 13, 6, 7, 14, 15);

        // vertical pass
        v_int16 b0 = vx_setall_s16(beta[0]);
        v_int16 b1 = vx_setall_s16(beta[1]);
        v_int16 b2 = vx_setall_s16(beta[2]);
        v_int16 b3 = vx_setall_s16(beta[3]);

        for (int w = 0; w < inSz.width*chanNum; ) {
            for (; w <= inSz.width*chanNum - half_nlanes && w >= 0; w += half_nlanes) {
                v_int16 val0_0 = v_load_ccache_expand(&src0[0][w]);
                v_int16 val0_1 = v_load_ccache_expand(&src0[1][w]);
                v_int16 val0_2 = v_load_ccache_expand(&src0[2][w]);
                v_int16 val0_3 = v_load_ccache_expand(&src0[3][w]);

                v_int16 val1_0 = v_load_ccache_expand(&src1[0][w]);
                v_int16 val1_1 = v_load_ccache_expand(&src1[1][w]);
                v_int16 val1_2 = v_load_ccache_expand(&src1[2][w]);
                v_int16 val1_3 = v_load_ccache_expand(&src1[3][w]);

                v_int16 t0 = v_mulhrs(v_sub_wrap(val0_0, val1_0), b0);
                v_int16 t1 = v_mulhrs(v_sub_wrap(val0_1, val1_1), b1);
                v_int16 t2 = v_mulhrs(v_sub_wrap(val0_2, val1_2), b2);
                v_int16 t3 = v_mulhrs(v_sub_wrap(val0_3, val1_3), b3);

                v_int16 r0 = v_add_wrap(val1_0, t0);
                v_int16 r1 = v_add_wrap(val1_1, t1);
                v_int16 r2 = v_add_wrap(val1_2, t2);
                v_int16 r3 = v_add_wrap(val1_3, t3);

                v_uint8 q0 = v_packus(r0, r1);
                v_uint8 q1 = v_packus(r2, r3);

                v_uint8 q2 = v_blend_shiftleft<0xCC /*0b11001100*/, 4>(q0, q1);
                v_uint8 q3 = v_blend_shiftright<0xCC /*0b11001100*/, 4>(q0, q1);             

                v_uint8 q4 = v_shuffle_s8(q2, shuf_mask1);
                v_uint8 q5 = v_shuffle_s8(q3, shuf_mask1);

                v_uint8 q6 = v256_permute2x128<0x20>(q4, q5);
                v_uint8 q7 = v256_permute2x128<0x31>(q4, q5);

                vx_store(&tmp[4 * w + 0],  q6);
                vx_store(&tmp[4 * w + 2 * half_nlanes], q7);
            }

            if (w < inSz.width*chanNum) {
                w = inSz.width*chanNum - half_nlanes;
            }
        }

        // horizontal pass
        v_uint8 val_0, val_1, val_2, val_3;
        GAPI_DbgAssert(outSz.width >= half_nlanes);
        for (int x = 0; x < outSz.width; ) {
            for (; x <= outSz.width - half_nlanes && x >= 0; x += half_nlanes) {
                v_int16 a10 = vx_load(&clone[4 * x]);
                v_int16 a32 = vx_load(&clone[4 * (x + 4)]);
                v_int16 a54 = vx_load(&clone[4 * (x + 8)]);
                v_int16 a76 = vx_load(&clone[4 * (x + 12)]);

                for (int c = 0; c < chanNum; ++c) {
                    v_gather_channel(val_0, tmp, mapsx, chanNum, c, x, 0);
                    v_gather_channel(val_1, tmp, mapsx, chanNum, c, x, shift);
                    v_gather_channel(val_2, tmp, mapsx, chanNum, c, x, shift * 2);
                    v_gather_channel(val_3, tmp, mapsx, chanNum, c, x, shift * 3);

                    v_int16 val0_0 = v_reinterpret_as_s16(v_expand_low(val_0));
                    v_int16 val0_1 = v_reinterpret_as_s16(v_expand_low(val_1));
                    v_int16 val0_2 = v_reinterpret_as_s16(v_expand_low(val_2));
                    v_int16 val0_3 = v_reinterpret_as_s16(v_expand_low(val_3));

                    v_int16 val1_0 = v_reinterpret_as_s16(v_expand_high(val_0));
                    v_int16 val1_1 = v_reinterpret_as_s16(v_expand_high(val_1));
                    v_int16 val1_2 = v_reinterpret_as_s16(v_expand_high(val_2));
                    v_int16 val1_3 = v_reinterpret_as_s16(v_expand_high(val_3));

                    v_int16 t0 = v_mulhrs(v_sub_wrap(val0_0, val1_0), a10);
                    v_int16 t1 = v_mulhrs(v_sub_wrap(val0_1, val1_1), a32);
                    v_int16 t2 = v_mulhrs(v_sub_wrap(val0_2, val1_2), a54);
                    v_int16 t3 = v_mulhrs(v_sub_wrap(val0_3, val1_3), a76);

                    v_int16 r0 = v_add_wrap(val1_0, t0);
                    v_int16 r1 = v_add_wrap(val1_1, t1);
                    v_int16 r2 = v_add_wrap(val1_2, t2);
                    v_int16 r3 = v_add_wrap(val1_3, t3);

                    v_uint8 q0 = v_packus(r0, r1);
                    v_uint8 q1 = v_packus(r2, r3);                    

                    v_uint8 q2 = v_shuffle_s8(q0, shuf_mask2);
                    v_uint8 q3 = v_shuffle_s8(q1, shuf_mask2);
                    
                    v_uint8 q4 = v_blend_shiftleft<0xCC /*0b11001100*/, 4>(q2, q3);
                    v_uint8 q5 = v_blend_shiftright<0xCC /*0b11001100*/, 4>(q2, q3);

                    v_uint8 q6 = v256_permute4x64<0xD8>(q4);
                    v_uint8 q7 = v256_permute4x64<0xD8>(q5);                    

                    v_uint8 q8 = v_shuffle_s8(q6, shuf_mask3);
                    v_uint8 q9 = v_shuffle_s8(q7, shuf_mask3);
                    
                    v_store_low(&dst[c][0][x], q8);
                    v_store_high(&dst[c][1][x], q8);
                    v_store_low(&dst[c][2][x], q9);
                    v_store_high(&dst[c][3][x], q9);
                }
            }

            if (x < outSz.width) {
                x = outSz.width - half_nlanes;
            }
        }
    } else {  // if any lpi
        for (int l = 0; l < lpi; ++l) {
            short beta0 = beta[l];

            // vertical pass
            GAPI_DbgAssert(inSz.width*chanNum >= half_nlanes);
            for (int w = 0; w < inSz.width*chanNum; ) {
                for (; w <= inSz.width*chanNum - half_nlanes; w += half_nlanes) {
                    v_int16 s0 = v_reinterpret_as_s16(vx_load_expand(&src0[l][w]));
                    v_int16 s1 = v_reinterpret_as_s16(vx_load_expand(&src1[l][w]));
                    v_int16 t = v_mulhrs(s0 - s1, beta0) + s1;
                    v_pack_u_store(tmp + w, t);
                }

                if (w < inSz.width*chanNum) {
                    w = inSz.width*chanNum - half_nlanes;
                }
            }

            // horizontal pass
            GAPI_DbgAssert(outSz.width >= half_nlanes);

            for (int x = 0; x < outSz.width; ) {
                for (; x <= outSz.width - half_nlanes && x >= 0; x += half_nlanes) {
                    for (int c = 0; c < chanNum; ++c) {
                        v_int16 a0 = vx_load(&alpha[x]);        // as signed Q1.1.14
                        v_int16 sx = vx_load(&mapsx[x]);        // as integer (int16)
                        v_int16 t0 = v_gather_chan<chanNum>(tmp, sx, c, 0);
                        v_int16 t1 = v_gather_chan<chanNum>(tmp, sx, c, 1);
                        v_int16 d = v_mulhrs(t0 - t1, a0) + t1;
                        v_pack_u_store(&dst[c][l][x], d);
                    }
                }

                if (x < outSz.width) {
                    x = outSz.width - half_nlanes;
                }
            }
        }
    }
}

// Resize (bi-linear, 8UC3)
void calcRowLinear_8U(C3, std::array<std::array<uint8_t*, 4>, 3> &dst,
                      const uint8_t *src0[],
                      const uint8_t *src1[],
                      const short    alpha[],
                      const short    clone[],  // 4 clones of alpha
                      const short    mapsx[],
                      const short    beta[],
                      uint8_t  tmp[],
                      const Size    &inSz,
                      const Size    &outSz,
                      int      lpi) {
    constexpr const int chanNum = 3;

    calcRowLinear_8UC_Impl<chanNum>(dst, src0, src1, alpha, clone, mapsx, beta, tmp, inSz, outSz, lpi);
}

// Resize (bi-linear, 8UC4)
void calcRowLinear_8U(C4, std::array<std::array<uint8_t*, 4>, 4> &dst,
                      const uint8_t *src0[],
                      const uint8_t *src1[],
                      const short    alpha[],
                      const short    clone[],  // 4 clones of alpha
                      const short    mapsx[],
                      const short    beta[],
                      uint8_t  tmp[],
                      const Size    &inSz,
                      const Size    &outSz,
                      int      lpi) {
    constexpr const int chanNum = 4;

    calcRowLinear_8UC_Impl<chanNum>(dst, src0, src1, alpha, clone, mapsx, beta, tmp, inSz, outSz, lpi);
}

void copyRow_8U(const uint8_t in[], uint8_t out[], int length) {
    copyRow_8U_impl(in, out, length);
}
void copyRow_32F(const float in[], float out[], int length) {
    copyRow_32F_impl(in, out, length);
}

}  // namespace avx
}  // namespace kernels
}  // namespace gapi
}  // namespace InferenceEngine
