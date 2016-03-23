#include "TLorentzVector.h"

#define ELOSS_TARG_EMPTY        0 // empty target. For IFLAG=0 no energy losses in target.
#define ELOSS_TARG_LH2          1 // LH2 (dencity 0.0708 g/cc)
#define ELOSS_TARG_SCINT        2 // plastic scintillator
#define ELOSS_TARG_C12_17       3 // carbon (density 1.7 g/cc)
#define ELOSS_TARG_LD2          4 // LD2 (dencity 0.1623 g/cc)
#define ELOSS_TARG_LH3          5 // L3He (dencity 0.072 g/cc)
#define ELOSS_TARG_AL           6 // Al
#define ELOSS_TARG_LHE4         7 // L4He (dencity 0.14 g/cc)
#define ELOSS_TARG_AIR          8 // air (gas, dencity 0.0129*10^-3 g/cc)
#define ELOSS_TARG_BUT_G9A      9 // butanol g9a (density 0.538 g/cc)      
#define ELOSS_TARG_CH2         10 // polyethylene (CH2) g9a (density 1.41 g/cc) 
#define ELOSS_TARG_C12_G9A     11 // carbon g9a (density 1.926 g/cc)
#define ELOSS_TARG_SCON        12 // superconduction wire
#define ELOSS_TARG_PTFE        13 // PCTFE


#define ELOSS_CELL_NONE 0       // - no target
#define ELOSS_CELL_g1a 1        // - g1a/g1b/g6a/g6b cell
#define ELOSS_CELL_g1b 1        //- g1a/g1b/g6a/g6b cell
#define ELOSS_CELL_g6a 1        //- g1a/g1b/g6a/g6b cell
#define ELOSS_CELL_g6b 1        //- g1a/g1b/g6a/g6b cell
#define ELOSS_CELL_g2a 2        //- g2a cell
#define ELOSS_CELL_g1c 3        //- g1c cell
#define ELOSS_CELL_g3 4         // - g3 cell
#define ELOSS_CELL_g8a 5        // - g8a/g6c cell
#define ELOSS_CELL_g6c 5        //- g8a/g6c cell
#define ELOSS_CELL_g10a 6       //- g10a cell
#define ELOSS_CELL_g11a 7       //- g11a cell
#define ELOSS_CELL_g8b 7        //- g11a cell
#define ELOSS_CELL_g13 7        //- g11a cell
#define ELOSS_CELL_g13a 7       //- g11a cell
#define ELOSS_CELL_g13b 7       //- g11a cell
#define ELOSS_CELL_eg3a 8       //- eg3a cell
#define ELOSS_CELL_g9a  9       //- g9a cell

TLorentzVector *eLoss(TLorentzVector*, float, TVector3, int, int);
void initELoss(float, float, float, float);

typedef struct {
  float target_offset[3];
  float st_offset;
} eloss_geom_t;
