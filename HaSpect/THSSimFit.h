#ifndef THSSIMFIT_h
#define THSSIMFIT_h

#include "THSRooFit.h"

using namespace RooFit;
using namespace std;

class THSSimFit : public THSRooFit {
 protected:
  Bool_t fIsBinTrees;
 public:
 THSSimFit() ;
 THSSimFit(TString name):THSRooFit(name) {};
 // THSSimFit(THSSimFit* rf) ;
 virtual ~THSSimFit(){};

 virtual void RunWeights();
 void SetModelEventsTree(TString name,TTree* tree);
 virtual void RunWithTrees();
 virtual void RunWithBins();
 void SetBinTrees(){fIsBinTrees=kTRUE;}
 
 ClassDef(THSSimFit, 0)  // RooFit mc simulation fit class

};


#endif //ifdef THSSIMFIT
