      SUBROUTINE MOMCOR( pout, pmass, vertex, iflag, icell, pin )
c
c_begin_doc
c  RCS ID string
c  $Id: momcor.F,v 1.17 2010/08/11 12:22:22 btmorr Exp $
c
c  Documentation for subroutine MOMCOR
c
c  Purpose: calculates charged particle momentum at vertex.  
c  --------
c
c!!!!!!!!!!! configuration for photon runs only !!!!!!!!!!!!!!!!!!!!!!!
c
c           Accounts for energy losses in cryo target, its wall,
c           carbon cylinder and start counter.
c
c  Input Parameters:  (Name - Type - Meaning)
c  ----------------
c  vertex(3) (real) vertex coordinates
c  pmass     (real) particle mass
c  pout(3)   (real) reconstructed particle momentum (vector) behind 
c                   the start counter
c
c  iflag     (integer) iflag = 0 - empty target 
c                      iflag = 1 - LH2 target
c                      iflag = 2 - LD2 target
c                      iflag = 3 = L3He target
c                      iflag = 4 = L4He target
c 
c  icell     (integer) target cell type:
c                      icell = 0 - no target
c                      icell = 1 - G1A/G1B/G6 running period
c                      icell = 2 - G2A running period
c                      icell = 3 - G1C running period
c                      icell = 4 - G3  running period
c                      icell = 5 - G6C/G8 running period
c                      icell = 6 - G10a running period
c                      icell = 7 - G11a running period
c                      icell = 8 - EG3a running period
c		       icell = 9 - G9a running period
c
c  Output Parameters:  (Name - Type - Meaning)
c  -----------------
c  pin(3)    (real) particle momentum at vertex
c
c  Other routines:
c  ---------------
c  stcounter, ccylinder, LH2targ, loss
c
c  Notes:
c  ------
c
c  Author:   Eugene Pasyuk      Created:  Tue Apr  6 16:39:57 EDT 1999
c  -------
c
c  Major revisions:
c  ----------------
c  G9a added 8/21/2009, Brian Morrison
c
c_end_doc
c
      IMPLICIT NONE
      SAVE
c
c_begin_var
c  input/output variables:
c  -----------------------
      INTEGER iflag
      INTEGER icell
      REAL pout(3)
      REAL pmass
      REAL vertex(3)
      REAL pin(3)
      
c
c  Local pre-defined variables:
c  ---------------------------
c  RCS information: 
      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
      PARAMETER (CFILE = '$RCSfile: momcor.F,v $')
      PARAMETER (CREVIS = '$Revision: 1.17 $')
      PARAMETER (CSTATE = '$State: Exp $')
      PARAMETER (CDATE = '$Date: 2010/08/11 12:22:22 $')
      PARAMETER (CAUTHO = '$Author: btmorr $')
      DATA CRCSID/   
     1'$Id: momcor.F,v 1.17 2010/08/11 12:22:22 btmorr Exp $'   
     2/   
c  Module information:
      CHARACTER*(*)  CRNAME, CRAUTH
      CHARACTER*100  CRMESS
      PARAMETER (CRNAME='MOMCOR')
      PARAMETER (CRAUTH='Eugene Pasyuk')
c
c  Local User defined variables:
c  -----------------------------
      INTEGER NWRITE
      INTEGER i
      INTEGER iwin
      INTEGER ierr
      INTEGER mate
      INTEGER loss
      REAL p(3)
      REAL po(3)
      REAL pi(3)
      REAL L(17)
      REAL targ(3)
      REAL ups(7)
      REAL dist,zLow,zHigh,scaleFactor
      REAL dist1
      REAL dens_foam
      REAL dens_scint
      REAL dens_sins
      REAL cdir(3)
      REAL pmom
      REAL gap
      REAL st_point(3)
      REAL sc_point(3)
      REAL r1point(3)
      REAL cell_point(3)
      INTEGER flag
      REAL le
      data L/0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0./
      data targ/0.,0.,0./
      data ups/0.,0.,0.,0.,0.,0.,0./
#include "eloss_geom.PAR"
      data target_offset, st_offset/0.0, 0.0, 0.0, 0.0/
      data nwrite/0/
c_end_var
c
c  executable code for routine MOMCOR:
c----6----------------------------------------------------------------72
c
      flag = 0
      le = 0
      IF (NWRITE .LT. 1) THEN
        NWRITE = NWRITE + 1
        CRMESS='First call to MOMCOR'
        CALL RECMES(CRNAME,'I',CRMESS)
      ENDIF
c---------------------------------------------------------------------
      pmom = sqrt(pout(1)**2 + pout(2)**2 + pout(3)**2)

      do i = 1,3
         po(i) = pout(i)
         cdir(i) = pout(i)/pmom
         pi(i) = po(i)
      enddo

      call air_gap(vertex, cdir, r1point) !air gap

      call stcounter(icell, vertex, cdir, dist, st_point) ! start counter

      gap = sqrt((r1point(1)-st_point(1))**2 + 
     +           (r1point(2)-st_point(2))**2 +
     +           (r1point(3)-st_point(3))**2)

      mate = 8                          ! air
      ierr = loss(mate, gap, pmass, po, p)
      do i = 1,3 
         pi(i) = p(i)
      enddo

      mate = 2                           ! Start Counter scintillator
      ierr = loss(mate, dist, pmass, pi, p)

      do i = 1,3
         pi(i) = p(i)
      enddo

      
      if( icell .LT. 7 ) then                    ! no carbon cyl. for g11 and after

         call ccylinder( vertex, cdir, dist)     ! carbon cylinder

         if ( dist .ne. 0.) then 
            mate = 3                             ! carbon cylinder (Carbon)
            ierr = loss(mate, dist, pmass, pi, p)
            do i = 1,3
               pi(i) = p(i)
            enddo
         endif

      endif

      if( icell .eq. 7  .or. icell .eq. 8) then 
c      g11 and  above 

         call scat_chamber( icell, vertex, cdir, dist, iwin, sc_point )

         if(dist.ne.0) then

            gap = sqrt((st_point(1) - sc_point(1))**2 + 
     +                 (st_point(2) - sc_point(2))**2 +
     +                 (st_point(3) - sc_point(3))**2)
            mate = 8            ! air
            ierr = loss(mate, gap, pmass, pi, p)
            do i = 1,3 
               pi(i) = p(i)
            enddo

            if(iwin.eq.0)then
               mate = 2         ! sciltill.in place of foam+scint (dencity scaled)
               dens_foam  = 0.110                     ! g/cm^2
               dens_scint = 1.032                     ! g/cm^2
               dist = dist * (dens_foam/dens_scint)
            else
               mate = 6         ! exit window (Al)
            endif
            ierr = loss(mate, dist, pmass, pi, p)
            do i = 1,3
               pi(i) = p(i)
            enddo         

         endif

      endif

      if (icell .ne. 9) then
	call targcell(icell, vertex, cdir, dist, dist1) ! target cell
      endif

      if ( dist .ne. 0. .and. icell .ne. 9) then

c For simplicity we will use plastic scintillator (mate=2) 
c as a target cell wall material.
c
c Even for G3 cell, which has aluminum foil besides plastic.
c The thickness of Al is 0.001" is small compared to total thickness (0.0075") 


         mate = 2               
         ierr = loss(mate, dist1, pmass, pi, p)
         do i = 1,3
            pi(i) = p(i)
         enddo

         if(iflag .ne. 0) then   !check if the target is empty or full
            mate = 0

            if(iflag .eq. 1) mate = 1 ! LH2
            if(iflag .eq. 2) mate = 4 ! LD2
            if(iflag .eq. 3) mate = 5 ! L3He
            if(iflag .eq. 4) mate = 7 ! L4He

            ierr = loss(mate, dist, pmass, pi, p)

            do i = 1,3
               pi(i) = p(i)
            enddo
         endif
      endif

      do i = 1,3
         pin(i) = pi(i)
      enddo




c--FROST Target--c
      if (icell .eq. 9) then

         do i=1,3
            po(i) = pi(i)
         enddo
         
         do i=1,12
            L(i) = 0
         enddo
         
         do i=1,3
            targ(i) = 0
         enddo

         do i=1,7
            ups(i) = 0
         enddo
         
         call FROST(vertex, cdir, L) ! target cell
         call targets(vertex, cdir, targ) !targets
         call upstream(vertex, cdir, ups) !upstream


         flag = 2 ! scintilator
         scaleFactor = 1.0/10.0
         le = L(12)*scaleFactor
         ierr = loss(flag, le, pmass, po, pi) ! Nose end cap
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6 ! Al
         scaleFactor = 0.0025/1.27 
         le = L(12)*scaleFactor ! foil thickness = 0.0025 cm, SC wall thickness = 1.27 cm 
         ierr = loss(flag, le, pmass, po, pi) ! Nose end cap aluminum foil
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6 ! Al
         le = L(11)
         ierr = loss(flag, le, pmass, po, pi) ! Exit window
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 2 ! scintilator
         scaleFactor = 1.0/10.0
         le = L(10)*scaleFactor
         ierr = loss(flag, le, pmass, po, pi) ! Scattering Chamber cylinder walls
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6 ! Al
         scaleFactor = 0.0025/2.1971
         le = L(10)*scaleFactor ! foil thickness = 0.0025 cm, SC wall thickness = 2.1971 cm 
         ierr = loss(flag, le, pmass, po, pi) ! Scattering Chamber cylinder wall aluminum foil
         do i=1,3
            po(i) = pi(i)
         enddo

c--   ONLY CORRECT FOR 20K IF SURE THAT VERTEX DOES NOT COME FROM THE CH2 TARGET AND IS FORWARD TRACK
         zLow = 13.9066
         zHigh = 17.8534
         if (vertex(3).lt.zLow.or.vertex(3).gt.zHigh.or.cdir(3).lt.0) then                  
            flag = 6 ! Al
            le = L(9)
            ierr = loss(flag, le, pmass, po, pi) ! 20k
            do i=1,3
               po(i) = pi(i)
            enddo
         endif

         flag = 12 ! superconduction wire
         le = L(8)
         ierr = loss(flag, le, pmass, po, pi) ! Holding Coil 4-layer
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 12 ! superconduction wire
         le = L(7)
         ierr = loss(flag, le, pmass, po, pi) ! Holding Coil 3-layer
         do i=1,3
            po(i) = pi(i)
         enddo

c--   ONLY CORRECT FOR 1K IF SURE THAT VERTEX DOES NOT COME FROM THE CARBON TARGET AND IS FORWARD TRACK
         zLow = 4.7914
         zHigh = 8.0
         if (vertex(3).lt.zLow.or.vertex(3).gt.zHigh.or.cdir(3).lt.0) then         
            flag = 6 ! Al
            le = L(6)
            ierr = loss(flag, le, pmass, po, pi) ! 1K stage 3
            do i=1,3
               po(i) = pi(i)
            enddo
         endif

         flag = 6 ! Al
         le = L(5)
         ierr = loss(flag, le, pmass, po, pi) ! 1K stage 2
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6 ! Al
         le = L(4)
         ierr = loss(flag, le, pmass, po, pi) ! 1K stage 1
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 13 ! PCTFE        
         le = L(3)
         ierr = loss(flag, le, pmass, po, pi) ! Mixing Chamber cylinder and endcap
         do i=1,3
            po(i) = pi(i)
         enddo
         
         flag = 7 ! L4He
         le = L(2)
         ierr = loss(flag, le, pmass, po, pi) ! Mixing Chamber cup interior
         do i=1,3
            po(i) = pi(i)
         enddo
         
         flag = 13 ! PCTFE
         le = L(1)
         ierr = loss(flag, le, pmass, po, pi) ! target cup cylinder and end cap
         do i=1,3
            po(i) = pi(i)
         enddo

         
c- targets

c- CH2
         flag = 10
         le = targ(3)
         ierr = loss(flag, le, pmass, po, pi) ! just after 20K end cap
         do i=1,3
            po(i) = pi(i)
         enddo
         
c- Carbon

         flag = 11
         le = targ(2)
         ierr = loss(flag, le, pmass, po, pi) ! just after 1K end cap
         do i=1,3
            po(i) = pi(i)
         enddo
         
c- Butanol

         flag = 9
         le = targ(1)
         ierr = loss(flag, le, pmass, po, pi) ! target cup interior
         do i=1,3
            po(i) = pi(i)
         enddo
         
c- Upstream stuff
         
         flag = 2 ! scintilator
         scaleFactor = 1.0/10.0
         le = ups(7)*scaleFactor
         ierr = loss(flag, le, pmass, po, pi) ! Scattering Chamber cylinder walls
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6 ! Al
         scaleFactor = 0.0025/2.1971
         le = ups(7)*scaleFactor ! foil thickness = 0.0025 cm, SC wall thickness = 2.1971 cm 
         ierr = loss(flag, le, pmass, po, pi) ! Scattering Chamber cylinder wall aluminum foil
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6  ! Al
         le = ups(6)
         ierr = loss(flag, le, pmass, po, pi) ! 20k
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 12 ! superconducting wire
         le = ups(5)
         ierr = loss(flag, le, pmass, po, pi) ! Holding Coil 3-layer
         do i=1,3
            po(i) = pi(i)
         enddo

         flag = 6 ! Al
         le = ups(4)
         ierr = loss(flag, le, pmass, po, pi) ! 1K 
         do i=1,3
            po(i) = pi(i)
         enddo
c- Steel
         flag = 14
         le = ups(3)
         ierr = loss(flag, le, pmass, po, pi)
         do i=1,3
            po(i) = pi(i)
         enddo
c- Aluminum
         flag = 6
         le = ups(2)
         ierr = loss(flag, le, pmass, po, pi)
         do i=1,3
            po(i) = pi(i)
         enddo

c- Kel-F
         flag = 13
         le = ups(1)
         ierr = loss(flag, le, pmass, po, pi)
         do i=1,3
            po(i) = pi(i)
         enddo
         

         
      endif
      
      do i=1,3
         pin(i) = pi(i)
      enddo
      
      RETURN
      END
c
c------------------------------------------------------------------------------





