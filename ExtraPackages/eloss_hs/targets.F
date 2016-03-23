      SUBROUTINE targets(vertex, cdir, targ)
      IMPLICIT NONE
      SAVE
c
c_begin_var
c  input/output variables:
c  -----------------------
c
      REAL vertex(3),vertexTmp(3)
      REAL cdir(3)
      REAL targ(3)      !length in butanol, length in carbon, length in ch2

c     Local pre-defined variables:
c     ---------------------------
c
c     RCS information: 

      CHARACTER*132  CFILE, CREVIS, CSTATE, CDATE, CAUTHO, CRCSID
      PARAMETER (CFILE = '$RCSfile: targets.F,v $')
      PARAMETER (CREVIS = '$Revision: 1.8 $')
      PARAMETER (CSTATE = '$State: Exp $')
      PARAMETER (CDATE = '$Date: 2010/10/22 14:08:18 $')
      PARAMETER (CAUTHO = '$Author: clasg9 $')
      DATA CRCSID/   
     1     '$Id: targets.F,v 1.8 2010/10/22 14:08:18 clasg9 Exp $'/
      
c     Module information:

      CHARACTER*(*)  CRNAME, CRAUTH
      CHARACTER*100  CRMESS
      PARAMETER (CRNAME='targets')
      PARAMETER (CRAUTH='Brian Morrison')
c     
c     Local User defined variables:
c     -----------------------------
c

      REAL TPOS(6)
      REAL RC
      REAL zLow,zHigh,pathLength
      REAL carlen,chlen,czAbs,carZcenter,chZcenter
      INTEGER i,ierr,ierrTmp


c     - Physical butanol: from -2.64 to +2.64
c     These values will change based on what I read from the vertex plot:
c     - centroid and sigma carbon: 6.143 cent and .6758 sigma -> Range: 4.7914 to 8
c     - centroid and sigma ch2: 15.88 cent and .6578 sigma - > Range: 13.9066 to 17.8534
      DATA TPOS/-2.64, 2.64, 4.7914, 8, 13.9066, 17.8534/
      carlen = 0.14986
      chlen = 0.34544

      carZcenter = 6.17 + carlen/2
      chZcenter = 15.97 + chlen/2

      ierr = 0
      pathLength = 0
      do i=1,3
         targ(i) = 0
      enddo

c--> BUTANOL
      zLow = TPOS(1)
      zHigh = TPOS(2)
      RC = 0.762
      call cylPath(vertex,cdir,rC,zLow,zHigh,pathLength,ierr)
      if (ierr.ne.0) then
         targ(1) = pathLength
      endif

c--> SPECIAL CASES (CARBON AND CH2 TARGETS)
      RC = 0.75

c--   FIRST DO CARBON
      zLow = TPOS(3)
      zHigh = TPOS(4)
      call cylPath(vertex,cdir,rC,zLow,zHigh,pathLength,ierr)
      do i=1,3
         vertexTmp(i) = vertex(i)
      enddo
      if (ierr.eq.1) then
         vertexTmp(3) = carZcenter
      endif
      zLow = carZcenter - carlen/2
      zHigh = carZcenter + carlen/2
      call cylPath(vertexTmp,cdir,rC,zLow,zHigh,pathLength,ierr)

      targ(2) = pathLength
      
      if (ierr.eq.0) then
         targ(2) = 0.0  ! MISSES TARGET
      endif

c--   NOW DO CH2
      zLow = TPOS(5)
      zHigh = TPOS(6)
      call cylPath(vertex,cdir,rC,zLow,zHigh,pathLength,ierr)
      do i=1,3
         vertexTmp(i) = vertex(i)
      enddo
      if (ierr.eq.1) then
         vertexTmp(3) = chZcenter
      endif
      zLow = chZcenter - chlen/2
      zHigh = chZcenter + chlen/2
      call cylPath(vertexTmp,cdir,rC,zLow,zHigh,pathLength,ierr)

      targ(3) = pathLength

      if (ierr.eq.0) then
         targ(3) = 0.0  ! MISSES TARGET
      endif


      RETURN
      end

c-----------------------------
      subroutine diskIntercept(vertex,cdir,rDisk,zPos,intercept,ierr)
      IMPLICIT NONE
      REAL vertex(3), cdir(3), intercept(3),rVec(3)
      REAL rDisk,zPos,rAbs,interceptXY
      INTEGER i,ierr

c--   INTITIALIZATION OF "rVec", "intercept", and "ierr"
      do i=1, 3
         rVec(i) = 0.0
         intercept(i) = 0.0
      enddo
      intercept(3) = zPos
      ierr = 0
      
c--   THE VECTOR "rVec" IS DEFINED THROUGH THE RELATION "vertex + rVec = intercept"      
c--   THE Z-ELEMENT OF "rVec" IS KNOWN FROM Z-POSITION OF DISK ("zPos") AND "vertex"
      rVec(3) = zPos - vertex(3)
c--   THE DIRECTION COSINE OF "rVec" IS GIVEN BY "cdir". THIS MEANS WE CAN FIND rAbs = |rVec|
      if (cdir(3).ne.0) then ! POSSIBLE INTERCEPT
         rAbs = rVec(3)/cdir(3)
         if (rAbs.gt.0) then ! POSSIBLE INTERCEPT
c--   NOW WE CAN FIND THE OTHER ELEMENTS OF "rVec"
            rVec(1) = rAbs*cdir(1)
            rVec(2) = rAbs*cdir(2)
c--   FIND THE THE OTHER ELEMENTS OF "intercept"
            intercept(1) = vertex(1) + rVec(1)
            intercept(2) = vertex(2) + rVec(2)
            interceptXY = sqrt(intercept(1)**2 + intercept(2)**2)
            if (interceptXY.le.rDisk) then ! VALID INTERCEPT
               ierr = 1
            endif
         endif
      endif
      if (ierr.eq.0) then ! NO INTERCEPT -> SET "intercept = (0,0,0)"
         do i=1, 3
            intercept(i) = 0.0
         enddo
      endif
      RETURN
      end
      
c-----------------------------
      subroutine cylPath(vertex,cdir,rCyl,zLow,zHigh,pathLength,ierr)
      IMPLICIT NONE
      REAL vertex(3),cdir(3),a1(3),a2(3),dir(3),center(3)
      REAL diskInt1(3),diskInt2(3),hitPos(2,3)
      REAL rCyl,zLow,zHigh,pathLength,dSq,d,vXY
      INTEGER i,ierr,ierrCyl,ierrDisk1,ierrDisk2
      INTEGER nHits,nHitsCyl
      DATA dir /0., 0., 1./
      DATA center/0.,0.,0./
      DATA a1/0.,0.,0./
      DATA a2/0.,0.,0./

c--   INITIALIZE "dSq", "ierr", "nHits" AND "pathLength"
      dSq = 0.0
      ierr = 0
      nHits = 0
      nHitsCyl = 0
      pathLength = 0.0

      call CylInt(vertex,cdir,center,dir,rCyl,a1,a2,ierrCyl)

c--   IF "ierrCyl" = 0: NO INTERCEPT-> NO PATH LENGTH
      if (ierrCyl.ne.0) then  

         call diskIntercept(vertex,cdir,rCyl,zLow,diskInt1,ierrDisk1)
         call diskIntercept(vertex,cdir,rCyl,zHigh,diskInt2,ierrDisk2)

c--   COUNT VALID HITS AND SET "hitPos" ARRAY
c--   VERTEX INSIDE CAPPED CYLINDER IS TAKEN AS A VALID HIT FOR "nHits"
c--   VARIABLE "nHitsCyl" COUNTS ONLY HITS ON CYLINDER AND END CAPS
         vXY = sqrt(vertex(1)**2 + vertex(2)**2)
         if (vXY.le.rCyl.and.vertex(3).ge.zLow.and.vertex(3).le.zHigh) then
            nHits = 1
            do i=1, 3
               hitPos(nHits,i) = vertex(i)
            enddo
         endif
         if (ierrDisk1.eq.1) then
            nHits = nHits + 1
            nHitsCyl = nHitsCyl + 1
            do i=1, 3
               hitPos(nHits,i) = diskInt1(i)
            enddo
         endif
         if (ierrDisk2.eq.1) then
            nHits = nHits + 1
            nHitsCyl = nHitsCyl + 1
            do i=1, 3
               hitPos(nHits,i) = diskInt2(i)
            enddo
         endif
         if (ierrCyl.eq.1) then
            if (a1(3).le.zHigh.and.a1(3).ge.zLow) then
               nHits = nHits + 1
               nHitsCyl = nHitsCyl + 1
               do i=1, 3
                  hitPos(nHits,i) = a1(i)
               enddo
            endif
         endif
         if (ierrCyl.eq.2) then
            if (a1(3).le.zHigh.and.a1(3).ge.zLow) then
               nHits = nHits + 1
               nHitsCyl = nHitsCyl + 1
               do i=1, 3
                  hitPos(nHits,i) = a1(i)
               enddo
            endif
            if (a2(3).le.zHigh.and.a2(3).ge.zLow) then
               nHits = nHits + 1
               nHitsCyl = nHitsCyl + 1
               do i=1, 3
                  hitPos(nHits,i) = a2(i)
               enddo
            endif
         endif
         
c         if (nHits.eq.1) then
c            write(*,*) 'WARNING: IMPOSSIBLE CYLINDER TRACK (type1)!!!'
c         endif
c         if (nHits.gt.2) then
c            write(*,*) 'WARNING: IMPOSSIBLE CYLINDER TRACK (type3+)!!!'
c         endif

         if (nHits.eq.2) then
            ierr = nHitsCyl
            do i=1,3
               dSq  = dSq + (hitPos(1,i) - hitPos(2,i))**2 
            enddo
            pathLength = sqrt(dSq)
         endif
      endif

      RETURN
      end
