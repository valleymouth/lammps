/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include <cstdio>
#include <cstring>
#include <cmath>
#include "fix_monod_het_kokkos.h"
#include "atom_kokkos.h"
#include "grid_kokkos.h"
#include "grid_masks.h"
#include "math_const.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

/* ---------------------------------------------------------------------- */

template <class DeviceType>
FixMonodHETKokkos<DeviceType>::FixMonodHETKokkos(LAMMPS *lmp, int narg, char **arg) :
  FixMonodHET(lmp, narg, arg)
{
  kokkosable = 1;
  execution_space = ExecutionSpaceFromDevice<DeviceType>::space;
}

/* ---------------------------------------------------------------------- */

template <class DeviceType>
void FixMonodHETKokkos<DeviceType>::compute()
{ 
  if (reaction_flag && growth_flag) {
    update_cells<1, 1>();
    update_atoms();
  } else if (reaction_flag && !growth_flag) {
    update_cells<1, 0>();
  } else if (!reaction_flag && growth_flag) {
    update_cells<0, 1>();
    update_atoms();
  }
}

/* ---------------------------------------------------------------------- */

template <class DeviceType>
template <int Reaction, int Growth>
void FixMonodHETKokkos<DeviceType>::update_cells()
{
  d_mask = gridKK->k_mask.template view<DeviceType>();
  d_conc = gridKK->k_conc.template view<DeviceType>();
  d_reac = gridKK->k_reac.template view<DeviceType>();
  d_dens = gridKK->k_dens.template view<DeviceType>();
  d_growth = gridKK->k_growth.template view<DeviceType>();

  if (Reaction)
    gridKK->sync(execution_space, GMASK_MASK | CONC_MASK | DENS_MASK);
  else
    gridKK->sync(execution_space, GMASK_MASK | CONC_MASK);

  copymode = 1;
  Kokkos::parallel_for(
    Kokkos::RangePolicy<
    DeviceType,
    FixMonodHETCellsTag<Reaction, Growth> >(0, grid->ncells), *this);
  copymode = 0;

  if (Growth)
    gridKK->modified(execution_space, GROWTH_MASK);
  if (Reaction)
    gridKK->modified(execution_space, REAC_MASK);
}

/* ---------------------------------------------------------------------- */

template <class DeviceType>
void FixMonodHETKokkos<DeviceType>::update_atoms()
{
  double **x = atom->x;
  double *radius = atom->radius;
  double *rmass = atom->rmass;
  double *outer_radius = atom->outer_radius;
  double *outer_mass = atom->outer_mass;
  double ***growth = grid->growth;

  const double three_quarters_pi = (3.0 / (4.0 * MY_PI));
  const double four_thirds_pi = 4.0 * MY_PI / 3.0;
  const double third = 1.0 / 3.0;

  gridKK->sync(Host, GROWTH_MASK);
  
  for (int i = 0; i < atom->nlocal; i++) {
    if (atom->mask[i] & groupbit) {
      const int cell = grid->cell(x[i]);
      const double density = rmass[i] /
	(four_thirds_pi * radius[i] * radius[i] * radius[i]);
      double m = rmass[i];
      rmass[i] = m * (1 + growth[igroup][cell][0] * dt);
      outer_mass[i] = four_thirds_pi *
	(outer_radius[i] * outer_radius[i] * outer_radius[i] -
	 radius[i] * radius[i] * radius[i]) *
	eps_dens + growth[igroup][cell][1] * m * dt;
      radius[i] = pow(three_quarters_pi * (rmass[i] / density), third);
      outer_radius[i] = pow(three_quarters_pi *
			    (rmass[i] / density + outer_mass[i] / eps_dens),
			    third);
    }
  }
}

/* ---------------------------------------------------------------------- */

template <class DeviceType>
template <int Reaction, int Growth>
KOKKOS_INLINE_FUNCTION
void FixMonodHETKokkos<DeviceType>::operator()(FixMonodHETCellsTag<Reaction, Growth>, int i) const
{
  if (!(d_mask(i) & BOUNDARY_MASK || d_mask(i) & CORNER_MASK)) {
    double tmp1 = growth * d_conc(isub, i) / (sub_affinity + d_conc(isub, i)) * d_conc(io2, i) / (o2_affinity + d_conc(io2, i));
    double tmp2 = anoxic * growth * d_conc(isub, i) / (sub_affinity + d_conc(isub, i)) * d_conc(ino3, i) / (no3_affinity + d_conc(ino3, i)) * o2_affinity / (o2_affinity + d_conc(io2, i));
    double tmp3 = anoxic * growth * d_conc(isub, i) / (sub_affinity + d_conc(isub, i)) * d_conc(ino2, i) / (no2_affinity + d_conc(ino2, i)) * o2_affinity / (o2_affinity + d_conc(io2, i));
    double tmp4 = maintain * d_conc(io2, i) / (o2_affinity + d_conc(io2, i));
    double tmp5 = 1 / 2.86 * maintain * anoxic * d_conc(ino3, i) / (no3_affinity + d_conc(ino3, i)) * o2_affinity / (o2_affinity + d_conc(io2, i));
    double tmp6 = 1 / 1.17 * maintain * anoxic * d_conc(ino2, i) / (no2_affinity + d_conc(ino2, i)) * o2_affinity / (o2_affinity + d_conc(io2, i));

    if (Reaction) {
      d_reac(isub, i) -= 1 / yield * (tmp1 + tmp2 + tmp3) * d_dens(igroup, i);
      d_reac(io2, i) -= (1 - yield - eps_yield) / yield * tmp1 * d_dens(igroup, i) + tmp4 * d_dens(igroup, i);
      d_reac(ino2, i) -= (1 - yield - eps_yield) / (1.17 * yield) * tmp3 * d_dens(igroup, i) + tmp6 * d_dens(igroup, i);
      d_reac(ino3, i) -= (1 - yield - eps_yield) / (2.86 * yield) * tmp2 * d_dens(igroup, i) + tmp5 * d_dens(igroup, i);
    }
  
    if (Growth) {
      d_growth(igroup, i, 0) = tmp1 + tmp2 + tmp3 - tmp4 - tmp5 - tmp6 - decay;
      d_growth(igroup, i, 1) = (eps_yield / yield) * (tmp1 + tmp2 + tmp3);
    }
  }
}

/* ---------------------------------------------------------------------- */

namespace LAMMPS_NS {
template class FixMonodHETKokkos<LMPDeviceType>;
#ifdef KOKKOS_ENABLE_CUDA
template class FixMonodHETKokkos<LMPHostType>;
#endif
}
