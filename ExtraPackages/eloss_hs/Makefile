# The rootbeer makefile

# get root libs
ROOTLIBS	= $(shell root-config --libs)


#if ROOTBEER not defined then make it the current working directory
ifndef ROOTBEER
 OBJDIR		= ./
 BINDIR		= ./
 SODIR		= ./
else
 OBJDIR		= $(ROOTBEER_OBJ)
 BINDIR		= $(ROOTBEER_BIN)
 SODIR		= $(ROOTBEER_SLIB)
endif

#default no debugging
DEBUG		= 0


#name of file with bankdefs
CLASBANKS     	=  $(INCDIR)/clasbanks.ddl

#only require this if building stuff hacked from clas packages
CLASPACK =$(CLAS_PACK)

# Linux with egcs
CXX		= g++
LD		= g++
CC		= gcc
#G77		= g77
G77		= gfortran

# allow debugging with >make DEBUG=1 
ifeq ($(DEBUG),1)
CXXFLAGS      	= -g -Wall -Wno-deprecated -DR__THREAD -fno-exceptions -fno-strict-aliasing \
		-fPIC -I$(ROOTSYS)/include -I./
CFLAGS	      	= -g -Wno-deprecated -D_REENTRANT -fno-strict-aliasing
#FFLAGS	      	= -g-mcpu=pentium -fno-automatic -finit-local-zero -ffixed-line-length-none \
#		-fno-second-underscore -DLinux

FFLAGS += -g
         FFLAGS += -fno-f2c
#        FFLAGS += -m32
         FFLAGS += -fno-automatic 
         FFLAGS += -fPIC
         FFLAGS += -ffixed-line-length-none
         FFLAGS += -fno-second-underscore
         FFLAGS += -funroll-loops
         FFLAGS += -fomit-frame-pointer

LDFLAGS       	= -g

else

CFLAGS		= -O2 -Wno-deprecated -Wno-unused -D_REENTRANT -fno-strict-aliasing
CXXFLAGS      	= -O2 -Wno-deprecated -Wno-unused -Wall -DR__THREAD \
		-fno-exceptions -fno-strict-aliasing -fPIC -I$(ROOTSYS)/include -I./
LDFLAGS       	= -O2 -Wall -Wno-deprecated -Wno-unused
#FFLAGS		=-O2 -mcpu=pentium -fno-automatic -finit-local-zero -ffixed-line-length-none \
#		-fno-second-underscore -DLinux

FFLAGS += -O2
         FFLAGS += -fno-f2c
#        FFLAGS += -m32                                                                                                                             
         FFLAGS += -fno-automatic
         FFLAGS += -fPIC                                                                                                                            
         FFLAGS += -ffixed-line-length-none
         FFLAGS += -fno-second-underscore
         FFLAGS += -funroll-loops
         FFLAGS += -fomit-frame-pointer


endif

SOFLAGS       	= -shared
LIBS          	= $(ROOTLIBS) -lm -ldl -rdynamic
RBLIB		= -L$(SODIR) -lRootBeer -lEG


GAWK 		= gawk

LIBPTHREAD	= -lpthread

#------------------------------ some rules ------------------------------

.SUFFIXES: 	.cxx

#rules for .cxx
$(OBJDIR)/%.o: 	$(SRCDIR)/%.cxx $(INCDIR)/%.h 
		$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: 		%.cxx
		$(CXX) $(CXXFLAGS) -o $@ -c $<


#rule for .C
$(OBJDIR)/%.o: 	$(SRCDIR)/%.C
		$(CXX) $(CXXFLAGS) -o $@ -c $<

#rule for c
$(OBJDIR)/%.o: 	$(SRCDIR)/%.c
		$(CC) $(CFLAGS) -o $@ -c $<

%.o:		%.c
		$(CC) $(CFLAGS) -o $@ -c $<

%.o:		%.F
		$(G77) $(FFLAGS) -DLinux -o $@ -c $<


## incorporation of Eugene's eloss package _____________________________________________________________________________
## Build the library 
ELOSSSO		= $(SODIR)/libeloss.so
ELOSS_RB_DIR	= $(PWD)

eloss: 		$(ELOSSSO)

# build the shared library by using the .F files from the standard eloss distribution, which must exist, or be linked as 
# extra_packages/eloss
# and some extra .F and .cxx and .h in extra_packages eloss_rb
#
$(ELOSSSO):	$(ELOSS_RB_DIR)/elossDict.o $(ELOSS_RB_DIR)/eLoss.h $(ELOSS_RB_DIR)/elossLinkDef.h \
		$(addsuffix .o, $(basename $(filter %.F %.cxx, $(wildcard $(ELOSS_RB_DIR)/*)))) \
		$(addprefix $(ELOSS_RB_DIR)/, $(addsuffix .o, $(notdir $(basename $(filter %.F, $(wildcard $(PWD)/*))))))


		$(LD) $(SOFLAGS) $(LDFLAGS) $(ELOSS_RB_DIR)/*.o  -o $@ -lgfortran


$(ELOSS_RB_DIR)/elossDict.o: 	$(ELOSS_RB_DIR)/eLoss.h $(ELOSS_RB_DIR)/elossLinkDef.h
				@echo "Generating dictionary $(PACK)/eloss/elossDict.cxx"
				$(ROOTSYS)/bin/rootcint -f $(ELOSS_RB_DIR)/elossDict.cxx \
				-c $(ELOSS_RB_DIR)/eLoss.h $(ELOSS_RB_DIR)/elossLinkDef.h
				$(CXX) $(CXXFLAGS) -o $@ -c $(ELOSS_RB_DIR)/elossDict.cxx

eloss_clean: 
		rm -f $(ELOSS_RB_DIR)/*.o $(ELOSS_RB_DIR)/*Dict* $(SODIR)/libeloss.so
#_______________________________________________________________________________________________________________________
