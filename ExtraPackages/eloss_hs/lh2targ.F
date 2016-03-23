      SUBROUTINE LH2TARG(  vertex, cdir, dist, dist1 )
c
c_begin_doc
c  RCS ID string
c  $Id: lh2targ.F,v 1.1.1.1 2000/04/17 15:30:33 marki Exp $
c
c  Documentation for subroutine LH2TARG
c
c  Purpose: calculates track length within LH2 target and its mylar walls.
c  --------
c           g1 run target geometry
c
c  Input Parameters:  (Name - Type - Meaning)
c  ----------------
c  vertex(3) (real) vertex coordinates
c  cdir(3)   (real) track direction cosines  
c
c  Output Parameters:  (Name - Type - Meaning)
c  -----------------
c  dist (real) track length within the scintillator
c  dist1 (real) track length in mylar wall
c
c  Other routines:
c  ---------------
c  IntersectionSphere, IntersectionCylindre
c
c  Notes:
c  ------
c
c  Author:   Eugene Pasyuk      Created:  Tue Apr  6 16:37:59 EDT 1999
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
c
      REAL vertex(3)
      REAL cdir(3)
      REAL dist
      REAL dist1 
c  Local pre-defined variables:
c  ---------------------------
      REAL long
      REAL rayon
      REAL wall
      REAL centre(3)
      REAL dir(3)

      DATA long /18.5/          !target length
      DATA rayon /3./           !target radius
      DATA wall /0.017/         !mylar wall thickness

      DATA centre /0., 0., 0./  !coordinates of the center of a target
      DATA dir /0., 0., 1./     !target axis direction


c  RCS information: 
      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
      PARAMETER (CFILE = '$RCSfile: lh2targ.F,v $')
      PARAMETER (CREVIS = '$Revision: 1.1.1.1 $')
      PARAMETER (CSTATE = '$State: Exp $')
      PARAMETER (CDATE = '$Date: 2000/04/17 15:30:33 $')
      PARAMETER (CAUTHO = '$Author: marki $')
      DATA CRCSID/   
     1'$Id: lh2targ.F,v 1.1.1.1 2000/04/17 15:30:33 marki Exp $'   
     2/   
c  Module information:
      CHARACTER*(*)  CRNAME, CRAUTH
      CHARACTER*100  CRMESS
      PARAMETER (CRNAME='LH2TARG')
      PARAMETER (CRAUTH='Eugene Pasyuk')
c
c  Local User defined variables:
c  -----------------------------
      INTEGER NWRITE
      INTEGER ierr
      INTEGER ierr1 
      INTEGER extrm
      INTEGER i

      REAL pos1(3)
      REAL pos2(3)
      REAL pos1w(3)
      REAL pos2w(3)
      REAL point(3)
      REAL r
      REAL x

c_end_var
c
c  executable code for routine LH2TARG:
c----6----------------------------------------------------------------72
c
      IF (NWRITE .LT. 1) THEN
         NWRITE = NWRITE + 1
         CRMESS = 'This is a DUMMY routine, this message written once'
         CALL RECMES(CRNAME,'I',CRMESS)
      ENDIF
c     
c------------------------------------------------------------------------------

      do i = 1,3
         point(i) = vertex(i) + 10.*cdir(i)
      enddo


      call IntersectionCylindre(vertex, point, centre, dir, rayon,
     $     pos1, pos2, ierr)

      if (ierr .eq. 0 .or. ierr .eq. 2 .or.
     $     ((vertex(1) - pos1(1))*(vertex(1) - pos2(1))
     +     +(vertex(2) - pos1(2))*(vertex(2) - pos2(2))) .gt. 0.) then
         dist = 0.
         return                 ! no intersection, get out
      endif

      if( (vertex(3) - pos1(3))*(vertex(3) - pos2(3)) .gt. 0.) then
         dist = 0.
         return                 !no intersection, get out
      endif

c---  cylinder or sphere?
      extrm = 0
      if (pos1(3) .gt. (long/2. - rayon)) extrm = 1
      if (-(long/2. - rayon) .gt. pos1(3)) extrm = -1

c---  sphere
      if (extrm .ne. 0) then 
         centre(3) = 1.*extrm*(long/2. - rayon)
         call IntersectionSphere(vertex, point, centre, rayon,
     $        pos1, pos2, ierr)
         if (ierr .eq. 0) then 
            dist = 0.
            return              !no intersection, get out
         endif
c---  spheric wall
         r = rayon + wall
         call IntersectionSphere(vertex, point, centre, r,
     $        pos1w, pos2w, ierr1)

         if (ierr .eq. 2) then
            pos1(1) = pos2(1)
            pos1(2) = pos2(2)
            pos1(3) = pos2(3)
            pos1w(1) = pos2w(1)
            pos1w(2) = pos2w(2)
            pos1w(3) = pos2w(3)
         endif
c---  cylinder wall
      else 
         r = rayon + wall
         call IntersectionCylindre(vertex, point, centre, dir, r,
     $        pos1w, pos2w, ierr)
      endif

      dist = 0.
      x = 0.

      do i = 1,3
         dist = dist + (pos1(i) - vertex(i))*(pos1(i) - vertex(i))
         x = x + (pos1w(i) - vertex(i))*(pos1w(i) - vertex(i))
      enddo

      dist = sqrt(dist)
      x = sqrt(x)
      dist1 = x - dist

      RETURN
      END


