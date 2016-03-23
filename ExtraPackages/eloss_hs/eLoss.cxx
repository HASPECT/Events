/* c wrapper for energy loss routine
returns new four vector
DJT JUL-99

use include file include/eloss.h for function proto-type in calling routine

note that mom_p_out means after (out of) the absorber and not output of the routine
ergo mom_in (into the absorber) is the mom out(put) from the routine

 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
//#include <kinematics.h>
#include "eLoss.h"
//#include "FROST.h"

extern "C" void momcor_(float*,float*,float*,int*,int*,float*);
extern "C" eloss_geom_t eloss_geom_;
//extern "C" void initeloss(float,float,float,float);
//extern "C" void loss_(int*, float*, float*, float*, float*);
//extern "C" long double FROST(double*, double*, long double*);
//extern "C" void frost_(float*, float*, float*); 

TLorentzVector *eLoss(TLorentzVector *p_out, float pmass, TVector3 vertex, int iflag, int icell){
  static TLorentzVector p;
  TLorentzVector *p_in=&p;
  
  float mom_p_out[3];                // proton momentum after targ loss etc (ie detected)
  float mom_p_in[3];                 // proton momentum before targ loss etc (ie "true momentum")
  float vert[3];                     // vertex

  mom_p_out[0] = (float)p_out->X();
  mom_p_out[1] = (float)p_out->Y();
  mom_p_out[2] = (float)p_out->Z();

  vert[0] = vertex.X();
  vert[1] = vertex.Y();
  vert[2] = vertex.Z();

  // this is the frottran funstion that doies the work 
  momcor_(mom_p_out,&pmass,vert,&iflag,&icell,mom_p_in);

  //put the results in the lorenzt vector
  p_in->SetXYZM((double)mom_p_in[0],(double)mom_p_in[1],(double)mom_p_in[2],pmass);

  return (p_in);
}

//initialize the targ and start counter
void initELoss(float targx, float targy, float targz, float st_offset){

    eloss_geom_.st_offset = st_offset;
  
    eloss_geom_.target_offset[0] = targx; /* set to default at CLAS center */
    eloss_geom_.target_offset[1] = targy;
    eloss_geom_.target_offset[2] = targz;
}

