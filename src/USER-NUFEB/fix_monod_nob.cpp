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
#include "fix_monod_nob.h"
#include "atom.h"
#include "force.h"
#include "error.h"
#include "grid.h"
#include "group.h"
#include "grid_masks.h"
#include "update.h"
#include "math_const.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

/* ---------------------------------------------------------------------- */

FixMonodNOB::FixMonodNOB(LAMMPS *lmp, int narg, char **arg) :
  FixMonod(lmp, narg, arg)
{
  dynamic_group_allow = 1;

  io2 = -1;
  ino2 = -1;
  ino3 = -1;

  o2_affinity = 0.0;
  no2_affinity = 0.0;

  growth = 0.0;
  yield = 1.0;
  maintain = 0.0;
  decay = 0.0;
  
  int iarg = 3;
  while (iarg < narg) {
    if (strcmp(arg[iarg], "o2") == 0) {
      io2 = grid->find(arg[iarg+1]);
      if (io2 < 0)
	error->all(FLERR, "Can't find substrate name");
      o2_affinity = force->numeric(FLERR, arg[iarg+2]);
      iarg += 3;
    } else if (strcmp(arg[iarg], "no2") == 0) {
      ino2 = grid->find(arg[iarg+1]);
      if (ino2 < 0)
	error->all(FLERR, "Can't find substrate name");
      no2_affinity = force->numeric(FLERR, arg[iarg+2]);
      iarg += 3;
    } else if (strcmp(arg[iarg], "no3") == 0) {
      ino3 = grid->find(arg[iarg+1]);
      if (ino3 < 0)
	error->all(FLERR, "Can't find substrate name");
      iarg += 2;
    } else if (strcmp(arg[iarg], "growth") == 0) {
      growth = force->numeric(FLERR, arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg], "yield") == 0) {
      yield = force->numeric(FLERR, arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg], "maintain") == 0) {
      maintain = force->numeric(FLERR, arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg], "decay") == 0) {
      decay = force->numeric(FLERR, arg[iarg+1]);
      iarg += 2;
    } else {
      error->all(FLERR, "Illegal fix nufeb/monod/nob command");
    }
  }
}

/* ---------------------------------------------------------------------- */

int FixMonodNOB::setmask()
{
  int mask = 0;
  mask |= POST_INTEGRATE;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixMonodNOB::post_integrate()
{
  if (!compute_flag)
    return;
  
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

template <int Reaction, int Growth>
void FixMonodNOB::update_cells()
{
  double **conc = grid->conc;
  double **reac = grid->reac;
  double **dens = grid->dens;

  for (int i = 0; i < grid->ncells; i++) {
    if (!(grid->mask[i] & GHOST_MASK)) {
      double tmp1 = growth * conc[ino2][i] / (no2_affinity + conc[ino2][i]) * conc[io2][i] / (o2_affinity + conc[io2][i]);
      double tmp2 = maintain * conc[io2][i] / (o2_affinity + conc[io2][i]);

      if (Reaction) {
	reac[ino2][i] -= 1 / yield * tmp1 * dens[igroup][i];
	reac[io2][i] -= (1.15 - yield) / yield * tmp1 * dens[igroup][i] + tmp2 * dens[igroup][i];
	reac[ino3][i] += 1 / yield * tmp1 * dens[igroup][i];
      }
  
      if (Growth) {
	grid->growth[igroup][i][0] = tmp1 - tmp2 - decay;
      }
    }
  }
}

void FixMonodNOB::update_atoms()
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

  for (int i = 0; i < atom->nlocal; i++) {
    if (atom->mask[i] & groupbit) {
      const int t = atom->type[i];
      const int cell = grid->cell(x[i]);

      const double density = rmass[i] /
	(four_thirds_pi * radius[i] * radius[i] * radius[i]);
      rmass[i] = rmass[i] * (1 + growth[t][cell][0] * update->dt);
      radius[i] = pow(three_quarters_pi * (rmass[i] / density), third);
      outer_mass[i] = rmass[i];
      outer_radius[i] = radius[i];
    }
  }
}
