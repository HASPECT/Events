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

typedef vector<TString> VecString_t;
typedef vector<TAxis> VecAxis_t;

class THSBins : public TNamed{
 protected:
  
  TTree* fBinTree;
  VecString_t fBinNames;//names of individual bins
  VecAxis_t fVarAxis;//bin limits for variables
  TFile* fFile;//! file for writing lists to disc in case they get large
  Int_t fNbins;//number of bins
  Int_t fNaxis;//number of axis
  Int_t fBin;//number of axis

 public:
 THSBins() : fBinTree(0),fFile(0),fNaxis(0),fNbins(0) {};
  THSBins(TString name,TString filename);
  THSBins(const THSBins& other, const char* name=0);

  THSBins(TString name);
  ~THSBins();

  virtual TObject* Clone(const char* newname="") const { return new THSBins(*this,newname); }

  void AddAxis(TString name,Int_t nbins,Double_t min,Double_t max);
  void AddAxis(TString name,Int_t nbins,Double_t* xbins);
  void IterateAxis(Int_t iA,TString binName);
  VecString_t GetBinNames(){return fBinNames;}
  VecAxis_t GetVarAxis(){return fVarAxis;}
  TString GetBinName(Int_t i){if(i<fNbins) return fBinNames[i];else return "";};
  void InitialiseBinTree(TString name,TString filename);
  void Save();
  void RunBinTree(TTree* tree);
  TTree* GetBinTree(){return fBinTree;}
  TTree* GetBinnedTree(TTree* tree,Int_t bin);
  Int_t GetN(){return fNbins;}
  Int_t GetNAxis(){return fNaxis;}
  void PrintAxis();
  ClassDef(THSBins, 1)  // Writeable bins class
};

#endif //ifdef THSBINS

