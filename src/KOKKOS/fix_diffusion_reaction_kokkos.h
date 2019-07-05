/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifdef FIX_CLASS

FixStyle(nufeb/diffusion_reaction/kk,FixDiffusionReactionKokkos<LMPDeviceType>)
FixStyle(nufeb/diffusion_reaction/kk/device,FixDiffusionReactionKokkos<LMPDeviceType>)
FixStyle(nufeb/diffusion_reaction/kk/host,FixDiffusionReactionKokkos<LMPHostType>)

#else
#ifndef LMP_FIX_DIFFUSION_REACTION_KOKKOS_H
#define LMP_FIX_DIFFUSION_REACTION_KOKKOS_H

#include "fix_diffusion_reaction.h"
#include "kokkos_type.h"

namespace LAMMPS_NS {

struct FixDiffusionReactionInitialTag {};
struct FixDiffusionReactionFinalTag {};
struct FixDiffusionReactionScalarTag {};
struct FixDiffusionReactionPreFinalTag {};
  
template<class DeviceType>
class FixDiffusionReactionKokkos : public FixDiffusionReaction {
 public:
  FixDiffusionReactionKokkos(class LAMMPS *, int, char **);
  virtual ~FixDiffusionReactionKokkos();
  virtual void init();
  void grow_arrays(int);
  virtual double compute_scalar();
  virtual void compute_initial();
  virtual void compute_final();

  KOKKOS_INLINE_FUNCTION
  void operator()(FixDiffusionReactionInitialTag, int) const;
  KOKKOS_INLINE_FUNCTION
  void operator()(FixDiffusionReactionPreFinalTag, int) const;
  KOKKOS_INLINE_FUNCTION
  void operator()(FixDiffusionReactionFinalTag, int) const;
  KOKKOS_INLINE_FUNCTION
  void operator()(FixDiffusionReactionScalarTag, int, double &) const;

 protected:
  double cell_size;
  int subbox[3];
  int boundary[6];
  
  typedef ArrayTypes<DeviceType> AT;
  
  typename AT::tdual_float_1d k_prev;

  typename AT::t_float_1d d_prev;
  typename AT::t_float_2d d_conc;
  typename AT::t_float_2d d_reac;
  typename AT::t_int_1d d_mask;
};

}

#endif
#endif

/* ERROR/WARNING messages:
*/
