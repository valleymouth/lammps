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

#ifdef FIX_CLASS

FixStyle(nufeb/eps_adhesion,FixEPSAdhesion)

#else

#ifndef LMP_FIX_EPS_ADHESION_H
#define LMP_FIX_EPS_ADHESION_H

#include "fix.h"

namespace LAMMPS_NS {

class FixEPSAdhesion : public Fix {
 public:
  FixEPSAdhesion(class LAMMPS *, int, char **);
  ~FixEPSAdhesion() {}
  int setmask();
  virtual void post_force(int);

 protected:
  int ieps;
  double ke;
  int disp;

  template <int DISP>
  void compute(int);
};

}

#endif
#endif

/* ERROR/WARNING messages:
*/
