      SUBROUTINE SCAT_CHAMBER( iCell, vertex, cdir, dist, iwin, pos1 )
cc
c  Documentation for subroutine SCAT_CHAMBER
c
c  Purpose: Calcualte the energy loss in the scattering chamber
c  --------
c
c  Input Parameters:  (Name - Type - Meaning)
c  ----------------
c  vertex(3) (real) vertex coordinates
c  cdir(3)   (real) track direction cosines  
c
c  Output Parameters:  (Name - Type - Meaning)
c  -----------------
c  dist (real) track length within the scintillator
c  pos1 hit position on the outside of the SC
c
c  Other routines:
c  ---------------
c  IntersectionCylindre
c
c  Notes:
c  ------
c     
c
c_end_doc
c
      IMPLICIT NONE
      SAVE
c
c_begin_var
c  input/output variables:
c  -----------------------
      INTEGER icell
      REAL vertex(3)
      REAL cdir(3)
      REAL dist

c  Local pre-defined variables:
c  ---------------------------
      REAL centre(3)            
      REAL rayon                !radius of the chamber
      REAL rayon_win            !radius of the exit window
      REAL thickness            !thicknes of the pipe
      REAL thickness_win        !thicknes of the exit window
      REAL extremtube(2)        !half length
      REAL alpha                !cone angle
      REAL dir(3)               !direction of pipe axis
      REAL pi
      DATA dir/0., 0., 1./
      DATA pi/3.14159265359/
      
c  RCS information: 
      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
      PARAMETER (CFILE = '$RCSfile: scat_chamber.F,v $')
      PARAMETER (CREVIS = '$Revision: 1.4 $')
      PARAMETER (CSTATE = '$State: Exp $')
      PARAMETER (CDATE = '$Date: 2009/10/02 19:01:26 $')
      PARAMETER (CAUTHO = '$Author: btmorr $')
      DATA CRCSID/   
     1'$Id: scat_chamber.F,v 1.4 2009/10/02 19:01:26 btmorr Exp $'   
     2/   


c  Module information:
      CHARACTER*(*)  CRNAME, CRAUTH
      CHARACTER*100  CRMESS
      PARAMETER (CRNAME='SCAT_CHAMBER')
      PARAMETER (CRAUTH='Eugeny Golovach')
c
c  Local User defined variables:
c  -----------------------------
      INTEGER NWRITE
      INTEGER ierr,iwin
      INTEGER i
      REAL point(3)
      REAL pos1(3), pos2(3)
      REAL intersec(3)
      REAL r

      DATA NWRITE/0/

#ifdef Linux

c for Linux these functions are not implemeted in standard library
c we define them here
c
      REAL sind, cosd, tand, angle
      sind(angle) = sin( angle*pi/180. )
      cosd(angle) = cos( angle*pi/180. )
      tand(angle) = tan( angle*pi/180. )
#endif

#include "eloss_geom.PAR"


c_end_var
c
c  executable code for routine CCYLINDER:
c----6----------------------------------------------------------------72
c
c see JLab drawing 66840-04073 for details
c
      iwin = 0
      
      if ( iCell .EQ. 7 ) then 
c       g11 scattering chamber 
         thickness     =  0.978 !thicknes of the chamber
         thickness_win =  0.0071 !thicknes of the exit window
         rayon         =  4.235 !radius of the chamber
         rayon_win     =  2.000 !radius of the exit win
         extremtube(1) = 22.522 !z position of downstream end
         extremtube(2) =-24.724 !z position of upstream end
         alpha         =  0.    !cone angle
      endif
c
c  JLab Drawing 88650-E-04157
c
      if ( iCell .EQ. 8 ) then 
c         eg3 scatterring chamber
         thickness     =  0.978 !thicknes of the chamber
         thickness_win =  0.0071 !thicknes of the exit window
         rayon         =  4.235 !radius of the chamber
         rayon_win     =  2.000 !radius of the exit win
         extremtube(1) =  22.522 + 10.805 !z position of downstream end
         extremtube(2) = -20.000!z position of upstream end
         alpha         =  0.    !cone angle         
      endif
      

c------------------------------------------------------------------------------
      do i = 1,3
         point(i) = vertex(i) + 500.*cdir(i)
         centre(i)= target_offset(i)
      enddo
c---- 6----------------------------------------------------------------72
c     
      IF (NWRITE .LT. 1) THEN
         NWRITE = NWRITE + 1
         write(CRMESS,*) 
     + 'CCYLINDER: target position is: ', target_offset
c         CRMESS = 'First call to TARGCELL'
         CALL RECMES(CRNAME,'I',CRMESS)
         
      ENDIF
c     
c------------------------------------------------------------------------------

c no correction if vertex is not within scattering chamber
      if ( vertex(3) .gt. extremtube(1) + centre(3) .or.
     +     vertex(3) .lt. extremtube(2) + centre(3) .or.
     +     vertex(1)**2 + vertex(2)**2 .gt. rayon**2 ) then
         dist = 0.
         return
      endif

      call InterCylinder(vertex, cdir, centre, dir, rayon, pos1, ierr)
      if (ierr .eq. 0 .or. ierr .eq. 2) then

c        No further treatment if:
c         - particle travels along the cylinder axis (ierr = 0)
c         - particle starts outside of the cylinder and travels 
c           away from the cylinder (ierr = 0)
c         - particle starts outside of the cylinder and travels
c           towards the cylinder and would have two intersections (ierr = 2)
c           [ should not happen in the case of a true event ]

         dist = 0.              ! get out
         return
      endif

c no correction if z-intercept is too far back
      if (pos1(3) .lt. extremtube(2)+centre(3)) then
         dist = 0.
         return
      endif

c--- cylinder or cone?
      if (pos1(3) .gt. extremtube(1)+centre(3) .or. ierr .eq. -1) then

c--- track goes through endcap
         centre(3) = centre(3) + extremtube(1)
         call IntersectionPlan(vertex, point, centre, dir, pos1, ierr)
         if(sqrt(pos1(1)**2+pos1(2)**2) .lt. rayon_win) iwin=1
         intersec(1) = pos1(1)
         intersec(2) = pos1(2)
         intersec(3) = pos1(3)
         centre(3) = centre(3)+ thickness
         call IntersectionPlan(vertex, point, centre, dir, pos1, ierr)
         if(iwin.eq.1) then      ! particle is going through chamber exit window
            intersec(1) = pos1(1)
            intersec(2) = pos1(2)
            intersec(3) = pos1(3)
            centre(3) = centre(3)+ thickness_win
            call IntersectionPlan(vertex, point, centre, dir, pos1, ierr)
         endif

      else
c--- track goes through the cylinder
         intersec(1) = pos1(1)
         intersec(2) = pos1(2)
         intersec(3) = pos1(3)
         r = rayon + thickness

         call InterCylinder(vertex, cdir, centre, dir, r, pos1, ierr)

      endif

      dist = 0.
      do i = 1,3
         dist = dist + (pos1(i) - intersec(i)) * (pos1(i) - intersec(i))
      enddo

      dist = sqrt(dist)	

      RETURN
      END



