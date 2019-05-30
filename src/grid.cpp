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

// #include <mpi.h>
// #include <cmath>
// #include <cstdio>
// #include <cstdlib>
#include <cstring>
// #include <climits>
#include "grid.h"
#include "style_grid.h"
#include "grid_vec.h"
// #include "atom_vec_ellipsoid.h"
#include "comm.h"
#include "comm_grid.h"
// #include "neighbor.h"
#include "force.h"
// #include "modify.h"
// #include "fix.h"
// #include "compute.h"
// #include "output.h"
// #include "thermo.h"
// #include "update.h"
#include "domain.h"
// #include "group.h"
// #include "input.h"
// #include "variable.h"
// #include "molecule.h"
// #include "atom_masks.h"
#include "math_const.h"
#include "memory.h"
#include "error.h"

using namespace LAMMPS_NS;
using namespace MathConst;

/* ---------------------------------------------------------------------- */

Grid::Grid(LAMMPS *lmp) : Pointers(lmp)
{
  grid_style = NULL;
  gvec = NULL;

  gvec_map = new GridVecCreatorMap();

#define GRID_CLASS
#define GridStyle(key,Class) \
  (*gvec_map)[#key] = &gvec_creator<Class>;
#include "style_grid.h"
#undef GridStyle
#undef GRID_CLASS

  grid_exist = false;
  
  nmax = 0;
  nsubs = 0;
  sub_names = NULL;
  cell_size = 1.0;
  box[0] = box[1] = box[2] = 0;
  ncells = 0;

  mask = NULL;

  conc = NULL;
  reac = NULL;
  dens = NULL;
  growth = NULL;
}

/* ---------------------------------------------------------------------- */

Grid::~Grid()
{
  delete [] grid_style;
  delete gvec;
  delete gvec_map;

  memory->destroy(mask);

  memory->destroy(conc);
  memory->destroy(reac);
}

/* ---------------------------------------------------------------------- */

void Grid::modify_params(int narg, char **arg)
{
  if (strcmp(arg[0], "set") == 0) {
    if (narg < 9) error->all(FLERR, "Invalid grid_modify command");
    lmp->init();
    grid->setup();
    gvec->set(find(arg[1]), force->numeric(FLERR, arg[2]),
	      force->numeric(FLERR, arg[3]), force->numeric(FLERR, arg[4]),
	      force->numeric(FLERR, arg[5]), force->numeric(FLERR, arg[6]),
	      force->numeric(FLERR, arg[7]), force->numeric(FLERR, arg[8]));
  }
}

/* ----------------------------------------------------------------------
   create an GridVec style
   called from lammps.cpp, input script, restart file, replicate
------------------------------------------------------------------------- */

void Grid::create_gvec(const char *style, int narg, char **arg, int trysuffix)
{
  delete [] grid_style;
  if (gvec) delete gvec;
  grid_style = NULL;
  gvec = NULL;

  // create instance of GridVec
  // use grow() to initialize arrays to length 1

  int sflag;
  gvec = new_gvec(style,trysuffix,sflag);
  gvec->store_args(narg,arg);
  gvec->process_args(narg,arg);
  gvec->grow(1);

  if (sflag) {
    char estyle[256];
    if (sflag == 1) sprintf(estyle,"%s/%s",style,lmp->suffix);
    else sprintf(estyle,"%s/%s",style,lmp->suffix2);
    int n = strlen(estyle) + 1;
    grid_style = new char[n];
    strcpy(grid_style,estyle);
  } else {
    int n = strlen(style) + 1;
    grid_style = new char[n];
    strcpy(grid_style,style);
  }

  grid_exist = true;
}

/* ----------------------------------------------------------------------
   generate an AtomVec class, first with suffix appended
------------------------------------------------------------------------- */

GridVec *Grid::new_gvec(const char *style, int trysuffix, int &sflag)
{
  if (trysuffix && lmp->suffix_enable) {
    if (lmp->suffix) {
      sflag = 1;
      char estyle[256];
      sprintf(estyle,"%s/%s",style,lmp->suffix);
      if (gvec_map->find(estyle) != gvec_map->end()) {
        GridVecCreator gvec_creator = (*gvec_map)[estyle];
        return gvec_creator(lmp);
      }
    }

    if (lmp->suffix2) {
      sflag = 2;
      char estyle[256];
      sprintf(estyle,"%s/%s",style,lmp->suffix2);
      if (gvec_map->find(estyle) != gvec_map->end()) {
        GridVecCreator gvec_creator = (*gvec_map)[estyle];
        return gvec_creator(lmp);
      }
    }
  }

  sflag = 0;
  if (gvec_map->find(style) != gvec_map->end()) {
    GridVecCreator gvec_creator = (*gvec_map)[style];
    return gvec_creator(lmp);
  }

  error->all(FLERR,"Unknown grid style");
  return NULL;
}

/* ----------------------------------------------------------------------
   one instance per GridVec style in style_grid.h
------------------------------------------------------------------------- */

template <typename T>
GridVec *Grid::gvec_creator(LAMMPS *lmp)
{
  return new T(lmp);
}

/* ---------------------------------------------------------------------- */

void Grid::init()
{
  gvec->init();
}

/* ---------------------------------------------------------------------- */

void Grid::setup()
{
  gvec->setup();
}

/* ---------------------------------------------------------------------- */

int Grid::find(const char *name)
{
  for (int i = 0; i < nsubs; i++) {
    if (strcmp(name, sub_names[i]) == 0)
      return i;
  }
  return -1;
}

/* ---------------------------------------------------------------------- */

int Grid::cell(double *x)
{
  int c[3];
  const double small = 1e-12;
  for (int i = 0; i < 3; i++) {
    c[i] = static_cast<int>((x[i] - domain->boxlo[i]) /
			    grid->cell_size + small) - grid->sublo[i];
  }
  return c[0] + c[1] * grid->subbox[0] +
    c[2] * grid->subbox[0] * grid->subbox[1];
}
