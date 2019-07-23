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

FixStyle(nufeb/monod/aob/kk,FixMonodAOBKokkos<LMPDeviceType>)
FixStyle(nufeb/monod/aob/kk/device,FixMonodAOBKokkos<LMPDeviceType>)
FixStyle(nufeb/monod/aob/kk/host,FixMonodAOBKokkos<LMPHostType>)

#else

#ifndef LMP_FIX_MONOD_AOB_KOKKOS_H
#define LMP_FIX_MONOD_AOB_KOKKOS_H

#include "fix_monod_aob.h"
#include "kokkos_type.h"

namespace LAMMPS_NS {

template <int, int>
struct FixMonodAOBCellsTag {};
struct FixMonodAOBAtomsTag {};

template <class DeviceType>
class FixMonodAOBKokkos: public FixMonodAOB {
 public:
  FixMonodAOBKokkos(class LAMMPS *, int, char **);
  virtual ~FixMonodAOBKokkos() {}
  virtual void compute();

  template <int, int> void update_cells();
  void update_atoms();

  struct Functor
  {
    int igroup;
    int inh4;
    int io2;
    int ino2;
    
    double nh4_affinity;
    double o2_affinity;

    double growth;
    double yield;
    double maintain;
    double decay;

    typedef ArrayTypes<DeviceType> AT;
    typename AT::t_int_1d d_mask;
    typename AT::t_float_2d d_conc;
    typename AT::t_float_2d d_reac;
    typename AT::t_float_2d d_dens;
    typename AT::t_float_3d d_growth;

    Functor(FixMonodAOBKokkos *ptr);
    
    template <int Reaction, int Growth>
    KOKKOS_INLINE_FUNCTION
    void operator()(FixMonodAOBCellsTag<Reaction, Growth>, int) const;
  };

 protected:
  typedef ArrayTypes<DeviceType> AT;

  typename AT::t_int_1d d_mask;
  typename AT::t_float_2d d_conc;
  typename AT::t_float_2d d_reac;
  typename AT::t_float_2d d_dens;
  typename AT::t_float_3d d_growth;
};

}

#endif
#endif

/* ERROR/WARNING messages:
*/
