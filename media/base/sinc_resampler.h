// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SINC_RESAMPLER_H_
#define MEDIA_BASE_SINC_RESAMPLER_H_

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/aligned_memory.h"
#include "base/memory/scoped_ptr.h"
#include "build/build_config.h"
#include "media/base/media_export.h"

namespace media {

// SincResampler is a high-quality single-channel sample-rate converter.
class MEDIA_EXPORT SincResampler {
 public:
  enum {
    // The kernel size can be adjusted for quality (higher is better) at the
    // expense of performance.  Must be a multiple of 32.
    // TODO(dalecurtis): Test performance to see if we can jack this up to 64+.
    kKernelSize = 32,

    // The number of destination frames generated per processing pass.  Affects
    // how often and for how much SincResampler calls back for input.  Must be
    // greater than kKernelSize.
    kBlockSize = 512,

    // The kernel offset count is used for interpolation and is the number of
    // sub-sample kernel shifts.  Can be adjusted for quality (higher is better)
    // at the expense of allocating more memory.
    kKernelOffsetCount = 32,
    kKernelStorageSize = kKernelSize * (kKernelOffsetCount + 1),

    // The size (in samples) of the internal buffer used by the resampler.
    kBufferSize = kBlockSize + kKernelSize,

    // The maximum number of samples that may be requested from the callback
    // ahead of the current position in the stream.
    kMaximumLookAheadSize = kBufferSize
  };

  // Callback type for providing more data into the resampler.  Expects |frames|
  // of data to be rendered into |destination|; zero padded if not enough frames
  // are available to satisfy the request.
  typedef base::Callback<void(float* destination, int frames)> ReadCB;

  // Constructs a SincResampler with the specified |read_cb|, which is used to
  // acquire audio data for resampling.  |io_sample_rate_ratio| is the ratio of
  // input / output sample rates.
  SincResampler(double io_sample_rate_ratio, const ReadCB& read_cb);
  virtual ~SincResampler();

  // Resample |frames| of data from |read_cb_| into |destination|.
  void Resample(float* destination, int frames);

  // The maximum size in frames that guarantees Resample() will only make a
  // single call to |read_cb_| for more data.
  int ChunkSize() const;

  // Flush all buffered data and reset internal indices.
  void Flush();

 private:
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, Convolve);
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, ConvolveBenchmark);

  void InitializeKernel();

  // Compute convolution of |k1| and |k2| over |input_ptr|, resultant sums are
  // linearly interpolated using |kernel_interpolation_factor|.  On x86, the
  // underlying implementation is chosen at run time based on SSE support.  On
  // ARM, NEON support is chosen at compile time based on compilation flags.
  static float Convolve_C(const float* input_ptr, const float* k1,
                          const float* k2, double kernel_interpolation_factor);
#if defined(ARCH_CPU_X86_FAMILY)
  static float Convolve_SSE(const float* input_ptr, const float* k1,
                            const float* k2,
                            double kernel_interpolation_factor);
#elif defined(ARCH_CPU_ARM_FAMILY) && defined(USE_NEON)
  static float Convolve_NEON(const float* input_ptr, const float* k1,
                             const float* k2,
                             double kernel_interpolation_factor);
#endif

  // The ratio of input / output sample rates.
  const double io_sample_rate_ratio_;

  // An index on the source input buffer with sub-sample precision.  It must be
  // double precision to avoid drift.
  double virtual_source_idx_;

  // The buffer is primed once at the very beginning of processing.
  bool buffer_primed_;

  // Source of data for resampling.
  ReadCB read_cb_;

  // Contains kKernelOffsetCount kernels back-to-back, each of size kKernelSize.
  // The kernel offsets are sub-sample shifts of a windowed sinc shifted from
  // 0.0 to 1.0 sample.
  scoped_ptr_malloc<float, base::ScopedPtrAlignedFree> kernel_storage_;

  // Data from the source is copied into this buffer for each processing pass.
  scoped_ptr_malloc<float, base::ScopedPtrAlignedFree> input_buffer_;

  // Stores the runtime selection of which Convolve function to use.
#if defined(ARCH_CPU_X86_FAMILY) && !defined(__SSE__)
  typedef float (*ConvolveProc)(const float*, const float*, const float*,
                                double);
  const ConvolveProc convolve_proc_;
#endif

  // Pointers to the various regions inside |input_buffer_|.  See the diagram at
  // the top of the .cc file for more information.
  float* const r0_;
  float* const r1_;
  float* const r2_;
  float* const r3_;
  float* const r4_;
  float* const r5_;

  DISALLOW_COPY_AND_ASSIGN(SincResampler);
};

}  // namespace media

#endif  // MEDIA_BASE_SINC_RESAMPLER_H_
