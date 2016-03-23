/*      function  InitEloss */
/*
 *_begin_doc
 *  RCS ID string
 *  $Id: InitEloss.c,v 1.6 2010/05/10 14:43:06 pasyuk Exp $
*/
/*
 *  Documentation for subroutine INITELOSS
 *
 *  Purpose:
 *  --------
 *
 *  Calling Sequence:
 *  ----------------
 *
 *  Input Parameters:  (Name - Type - Meaning)
 *  ----------------
 *
 *  Output Parameters:  (Name - Type - Meaning)
 *  -----------------
 *
 *  Called from:
 *  ------------
 *
 *  Other routines:
 *  ---------------
 *
 *  Notes:
 *  ------
 *
 *  Author:   Eugene Pasyuk      Created:  Thu Jun  6 14:48:56 EDT 2002
 *  -------
 *
 *  Major revisions:
 *  ----------------
 *     
 *
 *_end_doc
 */
/*
 *_begin_inc
 *  include files :
 *  ---------------------
 * BOS common block  uncomment the next line for BOS include file */
/*  #include <ntypes.h> */
/*  #include <bostypes.h>  */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ntypes.h>
#include <math.h>
#include <kinematics.h>
#include <map_manager.h>
#include <eloss.h>


/*_end_inc */
/*
 *  Local pre-defined variables:
 *  ---------------------------  
 *  RCS information: 
*/
#define USE(var) static void * use_##var = (void *) &var
/* keep this all on one line so CVS can recognize this */
static char crcsid[] =
   "$Id: InitEloss.c,v 1.6 2010/05/10 14:43:06 pasyuk Exp $";
USE(crcsid);  /* make sure it is not optimized away */

/*   Module information: */
      static char crname[] = "InitEloss";
      static char crauth[] = "Eugene Pasyuk";
/*
 *  executable code for InitEloss:
 *  -------------------------------------
*/

static int initialyzed=0;
static int CurrentRun=-1;

void initeloss_(int *run){
  /* Fortran wrapper */
  InitEloss(*run);
  return;
}


/*--------------------------------------------*/
void InitEloss(int RunNo){

  char *dir;
  char map[100];
  float offset[18];
  int firsttime;
  float T[3];
  
  if( CurrentRun != RunNo ){ 
    
    CurrentRun = RunNo;

    /* get ST offset from the GEOMETRY.map */
    dir = getenv("CLAS_PARMS");
    sprintf(map, "%s/Maps/GEOMETRY.map", dir);
    map_get_float(map, "st", "offset", 18, offset, RunNo, &firsttime);
    
    if(firsttime < 0) {
      fprintf(stderr, "\nInitEloss-W-: Unable to get ST-offset for Run %d\n", RunNo);
      eloss_geom_.st_offset = 0; /* set to default at CLAS center */
    }
    else {
      eloss_geom_.st_offset = offset[2];
    }
    fprintf(stderr, "\nInitEloss-I-: ST-offset is set to %f\n",  eloss_geom_.st_offset); 
    
    
    /* get Target offset */
    map_get_float(map, "target", "position", 3, T, RunNo, &firsttime);
   
    if(firsttime < 0) {
      fprintf(stderr, "InitEloss-W-: Unable to get Target Position for Run %d\n", RunNo);
      eloss_geom_.target_offset[0] = 0.0; /* set to default at CLAS center */
      eloss_geom_.target_offset[1] = 0.0;
      eloss_geom_.target_offset[2] = 0.0;
    }
    else {
      eloss_geom_.target_offset[0] = T[0];
      eloss_geom_.target_offset[1] = T[1];
      eloss_geom_.target_offset[2] = T[2];
    }

    fprintf(stderr, "InitEloss-I-: Target position is set to %f, %f, %f\n\n",  \
	    eloss_geom_.target_offset[0], eloss_geom_.target_offset[1], \
	    eloss_geom_.target_offset[2]); 
   
  }
  return;

}
/*
 *-------------------------------------------------------------------------
 */



