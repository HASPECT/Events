
#ifndef THSEVENTSPDF
#define THSEVENTSPDF

#include "RooAbsPdf.h"
#include "RooRealProxy.h"
#include "RooCategoryProxy.h"
#include "RooAbsReal.h"
#include "RooRealVar.h"
#include "RooAbsCategory.h"
#include "RooDataHist.h"
#include "RooHistPdf.h"
 
#include "TTree.h"
#include "TEntryList.h"
#include "TH2.h" 

class THSEventsPDF : public RooAbsPdf {
 public:
 THSEventsPDF() : fx_off(0),fx(0),falpha(0),fHist(0),fHistPdf(0), fRHist(0),fWeightHist(0), fTree(0) {cout<<"THSEventsPDF()"<<endl;} ; 
  THSEventsPDF(const char *name, const char *title,
	       RooAbsReal& _x,
	       RooAbsReal& _alpha,
	       RooAbsReal& _off,
	       RooAbsReal& _scale,
	       Int_t NBins=50,
	       Int_t NAlphBins=20); //number of bins to make of smeared distribution
  THSEventsPDF(const THSEventsPDF& other, const char* name=0) ;
  virtual TObject* clone(const char* newname) const { return new THSEventsPDF(*this,newname); }
  virtual ~THSEventsPDF();

  
  Int_t getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars,const char* rangeName=0) const;
  Double_t analyticalIntegral(Int_t code,const char* rangeName=0) const;
  
  void SetTree(TTree* tree){fTree=tree;}
  TTree* GetTree(){return fTree;}
  void SetEntryList(TEntryList* el) {fTree->SetEntryList(el);}
  Long64_t AddSmearedModel(TTree* tree=0,RooArgList vars=RooArgList());
  TH2* GetModelHist(){return fRHist;}
  RooHistPdf* GetHistPdf(){return fHistPdf;}  
  void SetModelHist(TH2F* his){ //set a user created morphing model (not smeared)
    fRHist=his;
    fHist = new RooDataHist(fRHist->GetName(),fRHist->GetName(),RooArgSet(*fx_off,*falpha),RooFit::Import(*fRHist));
    fHistPdf = new RooHistPdf(TString("PDF")+fRHist->GetName(),TString("PDF")+fRHist->GetName(),RooArgSet(*fx_off,*falpha),*fHist,1); ;
  }
  void SetWeightHist(TH1* hw,TString sx,TString sy=TString(""),TString sz=TString(""));
  Double_t GetDistWeight(Double_t wx,Double_t wy=0,Double_t wz=0);
 protected:
  void adjustBinning(Int_t* offset1=0) const;
  
  RooRealProxy x ;
  RooRealProxy offset ; //systematic offset
  RooRealProxy scale ; //systematic axis scaling
  RooRealProxy alpha ; //morphing paramter for additiona width
  
  RooRealVar* fx_off; //variables for pdf fHist
  RooRealVar* fx; //variables for fHist
  RooRealVar* falpha;
  
  Double_t evaluate() const ;
  
  RooDataHist* fHist;
  RooHistPdf* fHistPdf;
  TH2F* fRHist;
  TH1* fWeightHist;//allow tree distribution to be weighted by branch corresponing to axis in this histogram
  Double_t fMean;
  Double_t fOldScale;
  TTree* fTree;
  Int_t fNWdim;
 private:
  
  ClassDef(THSEventsPDF,1) // RooFit PDF class constructed as histogram from tree of events
 };
 
#endif
