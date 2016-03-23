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
#include <kinematics.h>


vector4_t c_momcor(vector4_t p_out, float pmass, vector3_t vertex, int iflag, \
int icell)
{
  vector4_t p_in = {0.,0.,0.,0.};
  float mom_p_out[3];                /* proton momentum */
  float mom_p_in[3];                /* proton momentum */
  float vert[3];                /* vertex */

  /*printf("p_out =  before corr "); v4print(stdout,p_out);*/
  mom_p_out[0] = p_out.space.x;
  mom_p_out[1] = p_out.space.y;
  mom_p_out[2] = p_out.space.z;
  vert[0] = vertex.x;
  vert[1] = vertex.y;
  vert[2] = vertex.z;
/*  iflag = 1; */
  momcor_(&mom_p_out,&pmass,&vert,&iflag,&icell,&mom_p_in);
  p_in.space.x = mom_p_in[0];
  p_in.space.y = mom_p_in[1];
  p_in.space.z = mom_p_in[2];
  p_in = v3_to_v4(&p_in.space,pmass);
  /*printf("p_in = after corr "); v4print(stdout,p_in);*/

  return(p_in);
}
