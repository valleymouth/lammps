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
#include "fix_diffusion_reaction.h"
#include "atom.h"
#include "force.h"
#include "update.h"
#include "respa.h"
#include "error.h"
#include "grid.h"
#include "grid_masks.h"
#include "memory.h"
#include "comm.h"

using namespace LAMMPS_NS;
using namespace FixConst;

/* ---------------------------------------------------------------------- */

FixDiffusionReaction::FixDiffusionReaction(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (strcmp(style,"nufeb/diffusion_reaction") != 0 && narg < 4)
    error->all(FLERR,"Illegal fix nufeb/diffusion_reaction command");

  compute_flag = 1;
  dynamic_group_allow = 1;
  scalar_flag = 1;
  
  ncells = 0;
  prev = NULL;
  
  isub = grid->find(arg[3]);
  if (isub < 0)
    error->all(FLERR, "Can't find substrate for nufeb/diffusion_reaction");

  diff_coef = force->numeric(FLERR, arg[4]);
}

/* ---------------------------------------------------------------------- */

FixDiffusionReaction::~FixDiffusionReaction()
{
  memory->destroy(prev);
}

/* ---------------------------------------------------------------------- */

int FixDiffusionReaction::setmask()
{
  int mask = 0;
  mask |= INITIAL_INTEGRATE;
  mask |= FINAL_INTEGRATE;
  return mask;
}

/* ---------------------------------------------------------------------- */

int FixDiffusionReaction::modify_param(int narg, char **arg)
{
  int iarg = 0;
  while (iarg < narg) {
    if (strcmp(arg[iarg], "compute") == 0) {
      if (strcmp(arg[iarg+1], "yes") == 0) {
	compute_flag = 1;
      } else if (strcmp(arg[iarg+1], "no") == 0) {
	compute_flag = 0;
      } else {
	error->all(FLERR, "Illegal fix_modify command");
      }
      iarg += 2;
    }
  }
  return iarg;
}

/* ---------------------------------------------------------------------- */

void FixDiffusionReaction::init()
{
  ncells = grid->ncells;
  prev = memory->create(prev, ncells, "nufeb/diffusion_reaction:prev");
  for (int i = 0; i < ncells; i++)
    prev[i] = 0.0;
}

/* ---------------------------------------------------------------------- */

void FixDiffusionReaction::initial_integrate(int)
{
  if (!compute_flag)
    return;
  
  for (int i = 0; i < grid->ncells; i++)
    grid->reac[isub][i] = 0.0;
}

/* ---------------------------------------------------------------------- */

void FixDiffusionReaction::final_integrate()
{
  if (!compute_flag)
    return;

  if (ncells != grid->ncells) {
    ncells = grid->ncells;
    prev = memory->grow(prev, ncells, "nufeb/diffusion_reaction:prev");
  }

  int nxy = grid->subbox[0] * grid->subbox[1];
  for (int i = 0; i < grid->ncells; i++) {
    // Neumann boundary condition. THIS NEEDS TO GO SOMEWHERE ELSE!
    if (grid->mask[i] & Z_NB_MASK) {
      int pz = i + nxy;
      grid->conc[isub][i] = grid->conc[isub][pz];
    }
    prev[i] = grid->conc[isub][i];
  }

  for (int i = 0; i < grid->ncells; i++) {
    if (!(grid->mask[i] & GHOST_MASK)) {
      int nx = i - 1;
      int px = i + 1;
      int ny = i - grid->subbox[0];
      int py = i + grid->subbox[0];
      int nz = i - nxy;
      int pz = i + nxy;
      double dnx = diff_coef * (prev[i] - prev[nx]) / grid->cell_size;
      double dpx = diff_coef * (prev[px] - prev[i]) / grid->cell_size;
      double ddx = (dpx - dnx) / grid->cell_size;
      double dny = diff_coef * (prev[i] - prev[ny]) / grid->cell_size;
      double dpy = diff_coef * (prev[py] - prev[i]) / grid->cell_size;
      double ddy = (dpy - dny) / grid->cell_size;
      double dnz = diff_coef * (prev[i] - prev[nz]) / grid->cell_size;
      double dpz = diff_coef * (prev[pz] - prev[i]) / grid->cell_size;
      double ddz = (dpz - dnz) / grid->cell_size;
      // prevent negative concentrations
      grid->conc[isub][i] = MAX(0, prev[i] + update->dt * (ddx + ddy + ddz + grid->reac[isub][i]));
    }
  }
}

/* ---------------------------------------------------------------------- */

double FixDiffusionReaction::compute_scalar()
{
  double result = 0.0;
  for (int i = 0; i < grid->ncells; i++) {
    if (!(grid->mask[i] & GHOST_MASK)) {
      result = MAX(result, fabs((grid->conc[isub][i] - prev[i]) / prev[i]));
    }
  }
  return result;
}
