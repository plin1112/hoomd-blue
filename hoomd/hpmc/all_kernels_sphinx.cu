// Copyright (c) 2009-2017 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

#include "ComputeFreeVolumeGPU.cuh"
#include "IntegratorHPMCMonoGPU.cuh"
#include "IntegratorHPMCMonoImplicitGPU.cuh"

#include "ShapeSphere.h"
#include "ShapeConvexPolygon.h"
#include "ShapePolyhedron.h"
#include "ShapeConvexPolyhedron.h"
#include "ShapeSpheropolyhedron.h"
#include "ShapeSpheropolygon.h"
#include "ShapeSimplePolygon.h"
#include "ShapeEllipsoid.h"
#include "ShapeFacetedSphere.h"
#include "ShapeSphinx.h"
#include "ShapeUnion.h"

namespace hpmc
{

namespace detail
{
#ifdef ENABLE_SPHINX_GPU
//! HPMC kernels for ShapeSphinx
template cudaError_t gpu_hpmc_free_volume<ShapeSphinx>(const hpmc_free_volume_args_t &args,
                                                       const typename ShapeSphinx::param_type *d_params);
template cudaError_t gpu_hpmc_update<ShapeSphinx>(const hpmc_args_t& args,
                                                  const typename ShapeSphinx::param_type *d_params);
template void gpu_hpmc_implicit_count_overlaps<ShapeSphinx>(const hpmc_implicit_args_t& args,
                                                  const typename ShapeSphinx::param_type *d_params);
template cudaError_t gpu_hpmc_implicit_accept_reject<ShapeSphinx>(const hpmc_implicit_args_t& args,
                                                  const typename ShapeSphinx::param_type *d_params);
#endif
}; // end namespace detail

} // end namespace hpmc
