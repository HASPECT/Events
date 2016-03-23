      SUBROUTINE upstream(vertex, cdir, ups)
      IMPLICIT NONE
      SAVE
c
c_begin_var
c  input/output variables:
c  -----------------------
c
      REAL vertex(3)
      REAL cdir(3)
      REAL ups(7)
      REAL dir(3)

c     Local pre-defined variables:
c     ---------------------------
c

      REAL RC(16)                ! radius of cylinders
      REAL ZPOS(14)             ! z-positions of cylinders


c--   GEOMETRY TAKEN FROM http://www.jlab.org/~zarecky/Frozen%20Spin/66840-04674-04.pdf
c--   AND FROM http://www.jlab.org/~zarecky/Frozen%20Spin/Scattering%20Chamber/66850-04635.pdf
c--
c--   IMPORTANT NOTE: THIS CODE ONLY LOOKS AT THE GEOMETRY THAT BEGINS AT THE BUTANOL TSH (z = -2.64cm)
c--
c--   ALL DISTANCES ARE IN CENTIMETERS
c--
c--   RC(1):  Kel-F (sample cup) inner radius = 0.670*2.54/2 = 0.8509
c--   RC(2):  Kel-F (sample cup) outer radius = 0.750*2.54/2 = 0.9525
c--   RC(3):  Smaller Al portion inner radius = 0.590*2.54/2 = 0.7493
c--   RC(4):  Smaller Al portion outer radius = 0.655*2.54/2 = 0.83185
c--   RC(5):  Larger Al portion inner radius = 0.627*2.54/2 = 0.79629
c--   RC(6):  Larger Al portion outer radius = 0.750*2.54/2 = 0.9525
c--   RC(7):  Steel portion inner radius = 0.555*2.54/2 = 0.70485
c--   RC(8):  Steel portion outer radius = 0.625*2.54/2 = 0.79375
c--
c--   FOR FOLLOWING, ONLY USING GEOMETRY WITH Z > -5.7589 cm AND Z < -2.64 cm
c--   RC(9):  1K inner radius = 2.35585                                                                              
c--   RC(10): 1K outer radius = 2.44475                                                                              
c--   RC(11): Super conducting wire inner radius2 (4-layer)  = 2.44475                                         
c--   RC(12): Super conducting wire outer radius2 = 1.969*2.54/2 = 2.50063              
c--   RC(13): 20K inner radius = (2.517-0.040)*2.54/2 - 10*0.000631957 = 3.1395.(SI layers=0.000631957)               
c--   RC(14): 20K outer radius = 2.517*2.54/2 = 3.19659                                   
c--   RC(15): Scattering chamber inner radius = 3.510*2.54/2 = 4.4577                                                 
c--   RC(16): Scattering chamber outer radius = 4.375*2.54/2 = 5.55625   
c--


      DATA RC/0.8509, 0.9525, 0.7493, 0.83185, 0.79629, 0.9525, 0.70485,
     &     0.79375, 2.35585, 2.44475, 2.44475, 2.50063, 3.1395, 3.19659,
     &     4.4577, 5.55625/


c--   ZPOS(1):  Kel-F starting point = -2.64
c--   ZPOS(2):  Kel-F ending point = -2.64 - 1.0*2.54 = -5.18
c--   ZPOS(3):  Al end cap starting point = -2.64
c--   ZPOS(4):  Al end cap ending point = -2.64 - .005*2.54 = -2.65
c--   ZPOS(5):  Smaller Al starting point = -2.64
c--   ZPOS(6):  Smaller Al ending point = -2.64 - 1.0*2.54 = -5.18
c--   ZPOS(7):  Middle Al piece starting = -5.18
c--   ZPOS(8):  Middle Al piece ending = -5.18 - .125*2.54 = -5.50
c--   ZPOS(9):  Larger Al starting point = -5.50
c--   ZPOS(10): Larger Al ending point = -2.64 - 2.0*2.54 = -7.72
c--   ZPOS(11): Steel starting point = -2.64 - (1.1 + .14)*2.54 = -5.7896
c--   ZPOS(12): Steel ending point = -5.7896 - 10.375*2.54 = -32.1421
c--   ZPOS(13):  Scattering Chamber start = -2.64
c--   ZPOS(14):  Scattering Chamber finish = -5.7589


      DATA ZPOS/-2.64, -5.18, -2.64, -2.65, -2.64, -5.18, -5.18, -5.5,
     &     -5.5, -7.72, -5.7896, -32.1421, -2.64, -5.7589 / 

      DATA dir /0., 0., 1./     !target axis direction cosines

c     RCS information: 

c      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
c      PARAMETER (CFILE = '$RCSfile: upstream.F,v $')
c      PARAMETER (CREVIS = '$Revision: 1.4 $')
c      PARAMETER (CSTATE = '$State: Exp $')
c      PARAMETER (CDATE = '$Date: 2010/08/11 12:22:22 $')
c      PARAMETER (CAUTHO = '$Author: btmorr $')
c      DATA CRCSID/   
c     1     '$Id: upstream.F,v 1.4 2010/08/11 12:22:22 btmorr Exp $'/
      
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
      INTEGER ierrD1,ierrD2,ierrOuter,ierrInner,i
      REAL  rInner, rOuter, rDisk, zLow, zHigh,z0,z0Tmp

      z0 = -2.64
      do i=1,3
         ups(i) = 0.0
      enddo
c     _end_var

      
c--   Kel-F (Target Sample Holder upstream portion)     

      rInner = RC(1)
      rOuter = RC(2)
      zLow = ZPOS(2)
      zHigh = ZPOS(1)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(1) = pathLength


c--   Al end cap

      rInner = RC(4)
      zLow = ZPOS(4)
      zHigh = ZPOS(3)

      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrOuter)
      if (ierrOuter.ne.0) then
         pathLength = pathLength1
      else
         pathLength = 0.0
      endif

      ups(2) = pathLength


c--   Al smaller portion

      rInner = RC(3)
      rOuter = RC(4)
      zLow = ZPOS(6)
      zHigh = ZPOS(5)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(2) = ups(2) + pathLength


c--   Al middle portion

      rInner = RC(3)
      rOuter = RC(6)
      zLow = ZPOS(8)
      zHigh = ZPOS(7)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(2) = ups(2) + pathLength


c--   Al larger portion

      rInner = RC(5)
      rOuter = RC(6)
      zLow = ZPOS(10)
      zHigh = ZPOS(9)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(2) = ups(2) + pathLength


c--   Steel portion

      rInner = RC(7)
      rOuter = RC(8)
      zLow = ZPOS(12)
      zHigh = ZPOS(11)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(3) = pathLength

c--   1K
      rInner = RC(9)
      rOuter = RC(10)
      zLow = ZPOS(14)
      zHigh = ZPOS(13)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(4) = pathLength

c--   Super conductor
      rInner = RC(11)
      rOuter = RC(12)
      zLow = ZPOS(14)
      zHigh = ZPOS(13)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(5) = pathLength

c--   20K
      rInner = RC(13)
      rOuter = RC(14)
      zLow = ZPOS(14)
      zHigh = ZPOS(13)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(6) = pathLength


c--   Scattering Chamber
      rInner = RC(15)
      rOuter = RC(16)
      zLow = ZPOS(14)
      zHigh = ZPOS(13)

      call cylPath(vertex,cdir,rOuter,zLow,zHigh,pathLength2,ierrOuter)
      call cylPath(vertex,cdir,rInner,zLow,zHigh,pathLength1,ierrInner)
      if (ierrOuter.ne.0) then
         pathLength = pathLength2 - pathLength1
      else
         pathLength = 0.0
      endif

      ups(7) = pathLength


      RETURN
      end

