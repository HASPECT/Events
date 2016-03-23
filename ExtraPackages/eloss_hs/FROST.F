      SUBROUTINE FROST(vertex, cdir, dist)
      IMPLICIT NONE
      SAVE
c
c_begin_var
c  input/output variables:
c  -----------------------
c
      REAL vertex(3)
      REAL cdir(3)
      REAL dist(12)
      REAL dir(3)

c     Local pre-defined variables:
c     ---------------------------
c

      REAL RC(20)               ! radius of cylinders
      REAL ZPOS(22)             ! z-positions of cylinders


c--   GEOMETRY TAKEN FROM http://www.jlab.org/~zarecky/Frozen%20Spin/66840-04674-04.pdf
c--   AND FROM http://www.jlab.org/~zarecky/Frozen%20Spin/Scattering%20Chamber/66850-04635.pdf
c--
c--   IMPORTANT NOTE: THIS CODE ONLY LOOKS AT THE GEOMETRY THAT BEGINS AT THE BUTANOL TSH (z = -2.64cm)
c--
c--   ALL DISTANCES ARE IN CENTIMETERS
c--
c--   RC(1):  Target Sample Holder (TSH) inner radius = (0.650-2*0.025)*2.54/2 = 0.762
c--   RC(2):  TSH outer radius = 0.650*2.54/2 = 0.8255        
c--   RC(3):  Mixing Chamber (MC) inner radius = (0.9-2*0.025)*2.54/2 = 1.0795
c--   RC(4):  Mixing Chamber (MC) outer radius = 0.9*2.54/2 = 1.143 
c--   RC(5):  1K inner radius1 = 1.855*2.54/2 = 2.35585
c--   RC(6):  1K outer radius1 = (1.969-2*0.017)*2.54/2 = 2.45745
c--   RC(7):  1K inner radius2 = 2.35585 
c--   RC(8):  1K outer radius2 = 2.44475
c--   RC(9)   1K inner radius3 = 2.35585
c--   RC(10)  1K outer radius3 = 2.50063
c--   RC(11)  1K dummy var1 = 2.35585
c--   RC(12)  1K dummy var2 = 2.50063
c--   RC(13): Super conducting wire inner radius1 (3-layer) = RC(6) = 2.45745         
c--   RC(14): Super conducting wire outer radius1 = 1.969*2.54/2 = 2.50063        
c--   RC(15): Super conducting wire inner radius2 (4-layer) = RC(8) = 2.44475         
c--   RC(16): Super conducting wire outer radius2 = 1.969*2.54/2 = 2.50063        
c--   RC(17): 20K inner radius = (2.517-0.040)*2.54/2 - 10*0.000631957 = 3.1395.(SI layers=0.000631957)
c--   RC(18): 20K outer radius = 2.517*2.54/2 = 3.19659         
c--   RC(19): Scattering chamber inner radius = 3.510*2.54/2 = 4.4577        
c--   RC(20): Scattering chamber outer radius = 4.375*2.54/2 = 5.55625      
c--
      DATA RC/0.762, 0.8255, 1.0795, 1.143, 2.35585, 2.45745, 2.35585,
     &     2.44475, 2.35585, 2.50063, 2.35585, 2.50063, 2.45745, 2.50063, 
     &     2.44475, 2.50063, 3.1395, 3.19659, 4.4577, 5.55625 /


c--   ZPOS(1):  TSH end cap start = 2.64
c--   ZPOS(2):  TSH end cap finish = 2.70
c--   ZPOS(3):  Mixing Chamber (MC) end cap start = 3.2
c--   ZPOS(4):  Mixing Chamber (MC) end cap finish = 3.2 + 0.064 = 3.264
c--   ZPOS(5):  1k start1 = -2.64  
c--   ZPOS(6):  1k finish1 = 3.02707
c--   ZPOS(7):  1k start2 = 3.02707  
c--   ZPOS(8):  1k finish2 = 5.29529
c--   ZPOS(9):  1k start3 = 5.29529  
c--   ZPOS(10): 1k finish3 = 6.15 
c--   ZPOS(11): 1K end cap start = 6.15
c--   ZPOS(12): 1K end cap finish = 6.15 + 0.02 = 6.17
c--   ZPOS(13): 3-layer wire cylinder start = -2.64  
c--   ZPOS(14): 3-layer wire cylinder finish = 3.02707
c--   ZPOS(15): 4-layer wire cylinder start = 3.02707
c--   ZPOS(16): 4-layer wire cylinder finish = 5.29529
c--   ZPOS(17): 20K end cap start = 15.95
c--   ZPOS(18): 20K end cap finish = 15.95 + 0.02 = 15.97
c--   ZPOS(19): Scattering chamber cylinder start = -2.64
c--   ZPOS(20): Scattering chamber cylinder finish = 18.9934
c--   ZPOS(21): exit window start = 21.61
c--   ZPOS(22): exit window finish = 21.61 + 0.025 = 21.635
c--
      DATA ZPOS/2.64, 2.7, 3.2, 3.264, -2.64, 3.02707, 3.02707, 5.29529,
     &     5.29529, 6.15, 6.15, 6.17, -2.64, 3.02707, 3.02707, 5.29529,
     &     15.95, 15.97, -2.64, 18.9934, 21.61, 21.635  / 

      DATA dir /0., 0., 1./     !target axis direction cosines

c     RCS information: 

c      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
c      PARAMETER (CFILE = '$RCSfile: FROST.F,v $')
c      PARAMETER (CREVIS = '$Revision: 1.8 $')
c      PARAMETER (CSTATE = '$State: Exp $')
c      PARAMETER (CDATE = '$Date: 2010/08/11 12:22:22 $')
c      PARAMETER (CAUTHO = '$Author: btmorr $')
c      DATA CRCSID/   
c     1     '$Id: FROST.F,v 1.8 2010/08/11 12:22:22 btmorr Exp $'/
      
c     Module information:

c      CHARACTER*(*)  CRNAME, CRAUTH
c      CHARACTER*100  CRMESS
c      PARAMETER (CRNAME='FROST')
c      PARAMETER (CRAUTH='Brian Morrison')
c     
c     Local User defined variables:
c     -----------------------------
c


      REAL pathLength1, pathLength2, pathLength,intercept(3)
      INTEGER ierrD1,ierrD2,ierrOuter,ierrInner,exitWindowR
      INTEGER i,hit1,hit2,ierr
      REAL  rInner, rOuter, rDisk, zLow, zHigh,z0,z0Tmp
      REAL  vc1(3),vc2(3),pos1(3),pos2(3),pos1w(3),pos2w(3)
      REAL a1(3),a2(3),center(3),d,dSq,czAbs
      DATA center/0.,0.,0./
      DATA pos1/0.,0.,0./
      DATA pos2/0.,0.,0./
      DATA pos1w/0.,0.,0./
      DATA pos2w/0.,0.,0./
      DATA vc1/0.,0.,21.427754295/
      DATA vc2/0.,0.,22.062002433/


      z0 = -2.64
      exitWindowR = 2.54/2
      do i=1,17
         dist(i) = 0.0
      enddo
c     _end_var

      
c--   TSH (Target Sample Holder)     

      rInner = RC(1)
      rOuter = RC(2)
      zLow = ZPOS(1)
      zHigh = ZPOS(2)

      call cylPath(vertex,cdir,rOuter,z0,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,z0,zLow,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(1) = pathLength

c--   MC (Mixing Chamber interior)

      rInner = RC(2)
      rOuter = RC(3)
      zLow = ZPOS(2)
      zHigh = ZPOS(3)

      call cylPath(vertex,cdir,rOuter,z0,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,z0,zLow,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(2) = pathLength

c--   MC (Mixing Chamber cylinder and end caps)

      rInner = RC(3)
      rOuter = RC(4)
      zLow = ZPOS(3)
      zHigh = ZPOS(4)

      call cylPath(vertex,cdir,rOuter,z0,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,z0,zLow,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(3) = pathLength


c--   1K stage 1 
c--
c--   NOTE: The 1K containment surface also acts as the support structure for the
c--   superconducting holding coils. Stage 1 is defined as the part of the 
c--   1K cylinder that has z>= -2.64 cm and supports the 3-layers of wire. Stage 2
c--   supports the 4-layers of wire. Stage 3 does not provide support for any wires.


      rInner = RC(5)
      rOuter = RC(6)
      zLow = ZPOS(5)
      zHigh = ZPOS(6)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(4) = pathLength

c--   1K stage 2

      rInner = RC(7)
      rOuter = RC(8)
      zLow = ZPOS(7)
      zHigh = ZPOS(8)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(5) = pathLength

c--   1K stage 3

      rInner = RC(9)
      rOuter = RC(10)
      zLow = ZPOS(11)
      zHigh = ZPOS(12)
      z0Tmp = ZPOS(9)

      call cylPath(vertex,cdir,rOuter,z0Tmp,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,z0Tmp,zLow,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(6) = pathLength


c--   Super conducting wire (3 layer)

      rInner = RC(13)
      rOuter = RC(14)
      zLow = ZPOS(13)
      zHigh = ZPOS(14)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(7) = pathLength

c--   Super conducting wire (4 layer)

      rInner = RC(15)
      rOuter = RC(16)
      zLow = ZPOS(15)
      zHigh = ZPOS(16)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(8) = pathLength


c--   20k

      rInner = RC(17)
      rOuter = RC(18)
      zLow = ZPOS(17)
      zHigh = ZPOS(18)

      call cylPath(vertex,cdir,rOuter,z0,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,z0,zLow,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(9) = pathLength


c--   SC (Scattering Chamber cylinder wall)

      rInner = RC(19)
      rOuter = RC(20)
      zHigh = ZPOS(20)

      call cylPath(vertex,cdir,rOuter,z0,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,z0,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      dist(10) = pathLength

c-- Exit window

      rOuter = exitWindowR
      zLow = ZPOS(21)
      zHigh = ZPOS(22)
      call diskIntercept(vertex,cdir,rOuter,zLow,intercept,ierrD1)
      czAbs = sqrt(cdir(3)*cdir(3))
      if (ierrD1.eq.1) then
         dist(11) = (zHigh - zLow)/czAbs
      endif

c--   Nose end cap
c--   GEOMETRY: 
c--   Conic structure with apex1 = (0.0, 0.0, 21.4278), apex2 = (0.0, 0.0, 22.0620)
c--   Angle that conic structure makes with the z-axis is 120 degrees


c--   FIRST CHECK IF TRACK GOES THROUGH NOSE END CAP

      rOuter = RC(19)
      zLow = ZPOS(20)
      call diskIntercept(vertex,cdir,rOuter,zLow,intercept,ierrD2)

c--   MAKE SURE THAT TRACK GOES THROUGH NOSE END CAP BUT NOT EXIT WINDOW

      if (ierrD2.eq.1.and.ierrD1.eq.0) then
c--   Need to get another point on track to send to IntersectionCone subroutine
         rInner = RC(19)
         call CylInt(vertex, cdir, center, dir, rInner, a1, a2, ierr)
         
c--   NOTE: Conic surface is at 60 degrees with respect to z-axis 
         
         call IntersectionCone(vertex, a1, vc1, dir, 60.0,
     &        pos1, pos2, hit1)
         call IntersectionCone(vertex, a1, vc2, dir, 60.0,
     &        pos1w, pos2w, hit2)
         
         
         dSq = 0
         do i=1,3
            dSq = dSq + (pos1w(i) - pos1(i))**2
         enddo
         d = sqrt(dSq)
         dist(12) = d
      endif
      
      RETURN
      end
      

      
