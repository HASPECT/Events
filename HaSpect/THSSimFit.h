#ifndef THSSIMFIT_h
#define THSSIMFIT_h

#include "THSRooFit.h"

using namespace RooFit;
using namespace std;

class THSSimFit : public THSRooFit {
 protected:
 Double_t  fAccFrac;
 public:
 THSSimFit() ;
 THSSimFit(TString name):THSRooFit(name) { }
 // THSSimFit(THSSimFit* rf) ;
 virtual ~THSSimFit(){};

 void SetModelEventsTree(TString name,TTree* tree);
 virtual void RunWithBins(Int_t Nbins=1);
 virtual void PrepareForFarm();
 virtual Bool_t  InitialiseFit();
 void SetAccFrac(Double_t af) {fAccFrac=af;}
 ClassDef(THSSimFit, 0)  // RooFit mc simulation fit class

};


#endif //ifdef THSSIMFIT
