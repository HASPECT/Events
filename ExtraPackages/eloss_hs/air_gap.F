      SUBROUTINE AIR_GAP(vertex, cdir, xpoint)
c
c_begin_doc
c  RCS ID string
c  $Id: air_gap.F,v 1.5 2009/10/02 19:01:26 btmorr Exp $
c
c  Documentation for subroutine AIR_GAP
c
c  Purpose: Finds a point wher track hits Region 1 DC
c  --------
c
c  Input Parameters:  (Name - Type - Meaning)
c  ----------------
c  vertex(3) (real) vertex coordinates
c  cdir(3)   (real) track direction cosines  
c
c  Output Parameters:  (Name - Type - Meaning)
c  -----------------
c  xpoint(3) - intercept of a track with Region 1 DC
c
c  Other routines:
c  ---------------
c  IntesectionPlan 
c  InterCylinder
c
c  Notes:
c  ------
c
c  Author:   Eugene Pasyuk      Created:  Tue Feb  17 15:38:45 EST 2004
c  -------
c
c  Major revisions:
c  ----------------
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
      REAL vertex(3)
      REAL cdir(3)
      REAL xpoint(3)
c
c  Local pre-defined variables:
c  ---------------------------

      REAL d_flat               !distance from beam axis to the 
                                !flat part of Region 1 DC
      REAL center_offset        !Offset of the center of the 
                                !cylindrical part from the beam axis
      REAL cyl_radius           !radius of cylidrical part

      REAL ncp(3)               !normal vector to central plane

      REAL pi

      DATA d_flat/57./
      DATA center_offset/-53./
      DATA cyl_radius/110./

      DATA pi/3.14159265359/
      DATA ncp/0.,0.,1./

#ifdef Linux
c for Linux these functions are not implemeted in standard library

      REAL sind, cosd, angle
      sind(angle) = sin( angle*pi/180. )
      cosd(angle) = cos( angle*pi/180. )

#endif



c  RCS information: 
      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
      PARAMETER (CFILE = '$RCSfile: air_gap.F,v $')
      PARAMETER (CREVIS = '$Revision: 1.5 $')
      PARAMETER (CSTATE = '$State: Exp $')
      PARAMETER (CDATE = '$Date: 2009/10/02 19:01:26 $')
      PARAMETER (CAUTHO = '$Author: btmorr $')
      DATA CRCSID/   
     1'$Id: air_gap.F,v 1.5 2009/10/02 19:01:26 btmorr Exp $'   
     2/   
c  Module information:
      CHARACTER*(*)  CRNAME, CRAUTH
      CHARACTER*100  CRMESS
      PARAMETER (CRNAME='AIR_GAP')
      PARAMETER (CRAUTH='Eugene Pasyuk')
c
c  Local User defined variables:
c  -----------------------------
      INTEGER NWRITE
      integer i
      integer ierr
      REAL alpha
      REAL w
      REAL phi 
      REAL point(3)
      REAL np(3)
      REAL start(3)
      REAL theta
      REAL dir(3)
      REAL aupoint(3)
      REAL norme
      REAL csinus
      REAL center(3)
      REAL cpi(3)


      DATA NWRITE/0/


c_end_var
c
c  executable code for routine AIR_GAP:
c----6----------------------------------------------------------------72
c
      IF (NWRITE .LT. 1) THEN
        NWRITE = NWRITE + 1
        CRMESS='This is a AIR_GAP routine, this message written once'
        write(CRMESS,*) 
        CALL RECMES(CRNAME,'I',CRMESS)
      ENDIF

c
c-------------Intersection with Region 1 DC
c

      alpha = atan2(cdir(2),cdir(1))
      alpha = 3.*alpha/pi
      w = nint(alpha)
      phi = w*60.

c -- vector normal to plane --
      np(1) = cosd(phi)
      np(2) = sind(phi)
      np(3) = 0.

      do i = 1,3
         point(i) = vertex(i) + 1000.*cdir(i)
         start(i) = d_flat*np(i)
      enddo

c     --  First let's find out if track hits flat part of Region 1

      call IntersectionPlan(vertex, point, start, np, aupoint, ierr)

      if ( aupoint(3) .le. 0. ) then !plane or cylinder?
c     -- plane --
C         write(*,*) 'plane'

         do i = 1,3
            xpoint(i) = aupoint(i)
         enddo
c     -- We are done with plane
         RETURN
      else
c     -- cylinder --
C     --define cylinder
C         write(*,*) 'cylinder'
         center(3) = 0.
         center(1) = center_offset*np(1)
         center(2) = center_offset*np(2)
c     -- axis direction
         dir(1) = np(2)
         dir(2) = -np(1)
         dir(3) = 0 
         

         if(vertex(3) .lt. 0.) then 
c     -- following trick is to avoid double intercept if vertex is upstream
c     -- of CLAS center
c     -- replace vertex with an intercept of a track with 
c     -- CLAS central plane 
            do i = 1, 3 
               start(i) = 0.
            enddo
            call IntersectionPlan(vertex,point,start,ncp,aupoint,ierr)
            call InterCylinder(aupoint, cdir, center, dir,
     +           cyl_radius, xpoint, ierr)
         else
c     -- vertex is downstrem of CLAS center, we don't need to do that
            call InterCylinder(vertex, cdir, center, dir,
     +           cyl_radius, xpoint, ierr)
         endif
         
      endif
C     -- Done with cylinder
      
      RETURN
      END
c
c------------------------------------------------------------------------------




