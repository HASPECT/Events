#ifndef THSBINS_h
#define THSBINS_h
#include <TAxis.h>
#include <TObjArray.h>
#include <TObject.h>
#include <TEntryList.h>
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include <vector>
#include <iostream>
using namespace std;

//typedef vector<Double_t> VecDouble_t;
typedef vector<TString> VecString_t;
typedef vector<TAxis> VecAxis_t;
#pragma link C++ typedef vector<TAxis> VecAxis_t+;
#pragma link C++ typedef vector<TString> VecString_t+;

class THSBins : public TNamed{
 protected:
  
  VecString_t fBinNames; //names of individual bins
  Int_t fNbins;
  Int_t fNaxis;
  VecAxis_t fVarAxis;//bin limits for variables
  TObjArray *fEntryLists;
  TObjArray *fTrees;
  TFile* fFile;

 public:
 THSBins() : fEntryLists(0),fTrees(0),fFile(0) {fNaxis=0;fNbins=0;};
  THSBins(const THSBins& other, const char* name=0);

 THSBins(TString name) :TNamed(name,name),fEntryLists(0),fTrees(0),fFile(0) {fNaxis=0;fNbins=0;};
  ~THSBins(){if(fFile){delete fFile;}delete fEntryLists;delete fTrees;fVarAxis.clear();};

  virtual TObject* Clone(const char* newname="") const { return new THSBins(*this,newname); }

  void InitialiseLists(TString name="HSBinsEntryList",TString filename="");
  void InitialiseTrees(TString name="HSBinsTree",TString filename="");
  void AddAxis(TString name,Int_t nbins,Double_t min,Double_t max);
  void IterateAxis(Int_t iA,TString binName);
  TString GetBinName(Int_t i){if(i<fNbins) return fBinNames[i];else return "";};
  TEntryList* GetEntryList(Int_t i){return dynamic_cast<TEntryList*>(fEntryLists->At(i));}
  TObjArray* GetEntryLists(){return fEntryLists;}
  void SaveLists();
  void SaveTrees();
  void RunEntryList(TTree* tree);
  void RunTree(TTree* tree);
  Int_t GetN(){return fNbins;}
  ClassDef(THSBins, 0)  // Writeable bins class
};
#endif //ifdef THSBINS

