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

FixStyle(nufeb/monod/eps/kk,FixMonodEPSKokkos<LMPDeviceType>)
FixStyle(nufeb/monod/eps/kk/device,FixMonodEPSKokkos<LMPDeviceType>)
FixStyle(nufeb/monod/eps/kk/host,FixMonodEPSKokkos<LMPHostType>)

#else

#ifndef LMP_FIX_MONOD_EPS_KOKKOS_H
#define LMP_FIX_MONOD_EPS_KOKKOS_H

#include "fix_monod_eps.h"
#include "kokkos_type.h"

namespace LAMMPS_NS {

template <int, int>
struct FixMonodEPSCellsTag {};
struct FixMonodEPSAtomsTag {};

template <class DeviceType>
class FixMonodEPSKokkos: public FixMonodEPS {
 public:
  FixMonodEPSKokkos(class LAMMPS *, int, char **);
  virtual ~FixMonodEPSKokkos() {}
  virtual void compute();

  template <int, int> void update_cells();
  void update_atoms();

  struct Functor
  {
    int igroup;
    int isub;
    double decay;

    typedef ArrayTypes<DeviceType> AT;
    typename AT::t_int_1d d_mask;
    typename AT::t_float_2d d_reac;
    typename AT::t_float_2d d_dens;
    typename AT::t_float_3d d_growth;

    Functor(FixMonodEPSKokkos *ptr);
    
    template <int Reaction, int Growth>
    KOKKOS_INLINE_FUNCTION
    void operator()(FixMonodEPSCellsTag<Reaction, Growth>, int) const;
  };

 protected:
  typedef ArrayTypes<DeviceType> AT;

  typename AT::t_int_1d d_mask;
  typename AT::t_float_2d d_reac;
  typename AT::t_float_2d d_dens;
  typename AT::t_float_3d d_growth;
};

}

#endif
#endif

/* ERROR/WARNING messages:
*/
