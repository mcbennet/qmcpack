///////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source
// License.  See LICENSE file in top directory for details.
//
// Copyright (c) 2020 QMCPACK developers.
//
// File developed by: Fionn Malone, malone14@llnl.gov, LLNL
//
// File created by: Fionn Malone, malone14@llnl.gov, LLNL
////////////////////////////////////////////////////////////////////////////////

#ifndef AFQMC_AXPY_BATCHED_GPU_KERNELS_HPP
#define AFQMC_AXPY_BATCHED_GPU_KERNELS_HPP

#include <cassert>
#include <complex>

namespace kernels
{
void axpy_batched_gpu(int n,
                      std::complex<double>* x,
                      const std::complex<double>** a,
                      int inca,
                      std::complex<double>** b,
                      int incb,
                      int batchSize);

void sumGw_batched_gpu(int n,
                       std::complex<double>* x,
                       const std::complex<double>** a,
                       int inca,
                       std::complex<double>** b,
                       int incb,
                       int b0,
                       int bw,
                       int batchSize);

} // namespace kernels

#endif
