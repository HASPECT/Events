#ifndef THSEVENTSFIT_h
#define THSEVENTSFIT_h

#include "THSsPlot.h"

using namespace RooFit;
using namespace std;

class THSEventsFit : public THSsPlot {
 protected:
  Double_t  fAccFrac; //only keep pdfs with entries >fAccFrac
  Bool_t fWithConstraints; // Use contraints from EventsPDF class
public:
 THSEventsFit() ;
 THSEventsFit(TString name):THSsPlot(name) {fWithConstraints=kFALSE;fAccFrac=0.0005; }
 // THSEventsFit(THSEventsFit* rf) ;
 virtual ~THSEventsFit(){};

 void SetModelEventsTree(TString name,TTree* tree);
 void SetWithConstraints(Bool_t withc=kTRUE){fWithConstraints=withc;}
 virtual void RunWeights(Int_t Nbins=1);
 virtual void PrepareForFarm();
 virtual Bool_t  InitialiseFit();
 void SetAccFrac(Double_t af) {fAccFrac=af;}
 ClassDef(THSEventsFit, 0)  // RooFit mc simulation fit class

};


#endif //ifdef THSSIMFIT
