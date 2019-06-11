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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "fix_divide.h"
#include "atom.h"
#include "atom_vec.h"
#include "error.h"
#include "force.h"
#include "lmptype.h"
#include "math_const.h"
#include "random_park.h"
#include "update.h"
#include "modify.h"
#include "domain.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

#define DELTA 1.005

/* ---------------------------------------------------------------------- */

FixDivide::FixDivide(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (narg < 6)
    error->all(FLERR, "Illegal fix nufeb/divide command");

  diameter = force->numeric(FLERR, arg[3]);
  eps_density = force->numeric(FLERR, arg[4]);
  seed = force->inumeric(FLERR, arg[5]);

  // Random number generator, same for all procs
  random = new RanPark(lmp, seed);

  force_reneighbor = 1;
}

/* ---------------------------------------------------------------------- */

FixDivide::~FixDivide()
{
  delete random;
}

/* ---------------------------------------------------------------------- */

int FixDivide::setmask()
{
  int mask = 0;
  mask |= POST_INTEGRATE;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixDivide::post_integrate()
{
  int nlocal = atom->nlocal;

  for (int i = 0; i < nlocal; i++) {
    if (atom->mask[i] & groupbit) {
      double density = atom->rmass[i] /
	(4.0 * MY_PI / 3.0 * atom->radius[i] * atom->radius[i] * atom->radius[i]);

      if (atom->radius[i] * 2 >= diameter) {
        double split = 0.4 + (random->uniform() * 0.2);
        double parent_mass = atom->rmass[i] * split;
        double child_mass = atom->rmass[i] - parent_mass;

        double parent_outer_mass = atom->outer_mass[i] * split;
        double child_outer_mass = atom->outer_mass[i] - parent_outer_mass;

        double theta = random->uniform() * 2 * MY_PI;
        double phi = random->uniform() * (MY_PI);

        double oldx = atom->x[i][0];
        double oldy = atom->x[i][1];
        double oldz = atom->x[i][2];

        // update parent
        atom->rmass[i] = parent_mass;
        atom->outer_mass[i] = parent_outer_mass;
        atom->radius[i] = pow(((6 * atom->rmass[i]) / (density * MY_PI)), (1.0 / 3.0)) * 0.5;
        atom->outer_radius[i] = pow((3.0 / (4.0 * MY_PI)) * ((atom->rmass[i] / density) + (parent_outer_mass / eps_density)), (1.0 / 3.0));
        double newx = oldx + (atom->outer_radius[i] * cos(theta) * sin(phi) * DELTA);
        double newy = oldy + (atom->outer_radius[i] * sin(theta) * sin(phi) * DELTA);
        double newz = oldz + (atom->outer_radius[i] * cos(phi) * DELTA);
        if (newx - atom->outer_radius[i] < domain->boxlo[0]) {
          newx = domain->boxlo[0] + atom->outer_radius[i];
        } else if (newx + atom->outer_radius[i] > domain->boxhi[0]) {
          newx = domain->boxhi[0] - atom->outer_radius[i];
        }
        if (newy - atom->outer_radius[i] < domain->boxlo[1]) {
          newy = domain->boxlo[1] + atom->outer_radius[i];
        } else if (newy + atom->outer_radius[i] > domain->boxhi[1]) {
          newy = domain->boxhi[1] - atom->outer_radius[i];
        }
        if (newz - atom->outer_radius[i] < domain->boxlo[2]) {
          newz = domain->boxlo[2] + atom->outer_radius[i];
        } else if (newz + atom->outer_radius[i] > domain->boxhi[2]) {
          newz = domain->boxhi[2] - atom->outer_radius[i];
        }
        atom->x[i][0] = newx;
        atom->x[i][1] = newy;
        atom->x[i][2] = newz;

        // create child
        double child_radius = pow(((6 * child_mass) / (density * MY_PI)), (1.0 / 3.0)) * 0.5;
        double child_outer_radius = pow((3.0 / (4.0 * MY_PI)) * ((child_mass / density) + (child_outer_mass / eps_density)), (1.0 / 3.0));
        double *coord = new double[3];
        newx = oldx - (child_outer_radius * cos(theta) * sin(phi) * DELTA);
        newy = oldy - (child_outer_radius * sin(theta) * sin(phi) * DELTA);
        newz = oldz - (child_outer_radius * cos(phi) * DELTA);
        if (newx - child_outer_radius < domain->boxlo[0]) {
          newx = domain->boxlo[0] + child_outer_radius;
        } else if (newx + child_outer_radius > domain->boxhi[0]) {
          newx = domain->boxhi[0] - child_outer_radius;
        }
        if (newy - child_outer_radius < domain->boxlo[1]) {
          newy = domain->boxlo[1] + child_outer_radius;
        } else if (newy + child_outer_radius > domain->boxhi[1]) {
          newy = domain->boxhi[1] - child_outer_radius;
        }
        if (newz - child_outer_radius < domain->boxlo[2]) {
          newz = domain->boxlo[2] + child_outer_radius;
        } else if (newz + child_outer_radius > domain->boxhi[2]) {
          newz = domain->boxhi[2] - child_outer_radius;
        }
        coord[0] = newx;
        coord[1] = newy;
        coord[2] = newz;

        atom->avec->create_atom(atom->type[i], coord);
        int n = atom->nlocal - 1;

        atom->tag[n] = 0;
        atom->mask[n] = atom->mask[i];
        atom->v[n][0] = atom->v[i][0];
        atom->v[n][1] = atom->v[i][1];
        atom->v[n][2] = atom->v[i][2];
        atom->omega[n][0] = atom->omega[i][0];
        atom->omega[n][1] = atom->omega[i][1];
        atom->omega[n][2] = atom->omega[i][2];
        atom->rmass[n] = child_mass;
        atom->radius[n] = child_radius;
        atom->outer_mass[n] = child_outer_mass;
        atom->outer_radius[n] = child_outer_radius;

        modify->create_attribute(n);

        delete[] coord;
      }
    }
  }

  bigint nblocal = atom->nlocal;
  MPI_Allreduce(&nblocal, &atom->natoms, 1, MPI_LMP_BIGINT, MPI_SUM, world);
  if (atom->natoms < 0 || atom->natoms >= MAXBIGINT)
    error->all(FLERR, "Too many total atoms");

  if (atom->tag_enable)
    atom->tag_extend();
  atom->tag_check();

  if (atom->map_style) {
    atom->nghost = 0;
    atom->map_init();
    atom->map_set();
  }

  // trigger immediate reneighboring
  next_reneighbor = update->ntimestep;
}
