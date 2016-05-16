#ifndef THSROOFIT_h
#define THSROOFIT_h
#include <RooWorkspace.h>
#include <RooAddPdf.h>
#include <RooDataSet.h>
#include <RooStats/SPlot.h>
#include <RooFitResult.h>
#include <TNamed.h>
#include <TSystem.h>
#include <TChain.h>
#include <TCanvas.h>
#include <TList.h>
#include <TFile.h>
#include <THSBins.h>
#include <THSWeights.h>
#include <vector>
#include <map>
using namespace RooFit;
using namespace std;

class THSRooFit : public TNamed {
 protected:
  RooWorkspace* fWS; //RooFit workspace container
  RooArgList fVariables; //variables in tree, to be fitted
  RooArgList fAuxVars; //variables in tree, not to be fitted
  RooArgList fBinVars; //variables in tree, to be used to make bins for fitting
  RooArgList fYields;//species yields
  RooArgList fPDFs;//species pdfs
  RooArgList fParameters;//model parameters
  RooRealVar *fID;
  RooAbsPdf* fModel; //model to be fitted to data
  RooAbsData* fData; //dataset to be fitted
  TList* fCanvases;  //canvases for plotting fitted variables
  TList* fHists;  //histograms for plotting weighted variables
  TList* fRooFits;  //collection of sub fits
  TTree* fTree; //!     // tree to be imported, don't save as part of class!
  RooStats::SPlot* fSPlot; //!  //sPlot object
  RooFitResult* fResult;   //RooFit result
  THSWeights* fWeights; //! 
  THSBins* fDataBins; //! 
  TString fIDBranchName;
  TString fYld;     //yield variable prepend
  TString fSingleSp;
  TString fOutDir;
  Bool_t fGotID;
  Bool_t fBinnedFit;
  Double_t fChi2;
  Double_t fSRange[2];
  Int_t fFiti;
 public:
  THSRooFit() ;//default constructor, must not allocate memory!!!
  THSRooFit(TString name);
  THSRooFit(THSRooFit* rf) ;
  virtual ~THSRooFit();

  void LoadVariable(TString opt);   //Load the variables to be fitted
  void LoadVariable(RooRealVar* var){fVariables.add(*var);fWS->import(*var);}
  void LoadAuxVars(TString opt);   //Load the variables not to be fitted
  void LoadAuxVars(RooRealVar* var){fAuxVars.add(*var);fWS->import(*var);}
  void LoadBinVars(TString opt,Int_t nbins,Double_t min,Double_t max);
  void LoadBinVars(TString opt,Int_t nbins,Double_t* xbins);
  void LoadSpeciesPDF(TString opt,Int_t Scale0=1); //Load a contributing species
  void TotalPDF();                  // Make the total fit pdf
  void DefineSets();                  // define sets in the workspace
  void LoadDataSet(TTree* tree,Bool_t toWS=kFALSE);    //Import a tree into a RooDataSet
  void LoadDataSet(RooAbsData* data,Bool_t toWS=kFALSE);    //Import a tree into a RooDataSet
  void LoadWorkSpace(RooWorkspace* ws); //load a workspace without data
  void LoadWorkSpaceData(RooWorkspace* ws); //load a workspace with data
  RooArgList GetVariables(){return fVariables;}
  RooRealVar* GetVariable(TString name){return dynamic_cast<RooRealVar*>(fVariables.find(name));}
  RooArgList GetAuxVars(){return fAuxVars;}
  RooArgList GetBinVars(){return fBinVars;}
  RooRealVar* GetAuxVar(TString name){return dynamic_cast<RooRealVar*>(fAuxVars.find(name));}
  RooRealVar* GetBinVar(TString name){return dynamic_cast<RooRealVar*>(fBinVars.find(name));}
  RooRealVar* GetVar(TString name);
  RooArgList GetYields(){return fYields;}
  RooArgList *GetPDFsp(){return &fPDFs;}
  RooArgList *GetYieldsp(){return &fYields;}
  RooArgList GetPDFs(){return fPDFs;}
  RooArgList GetParameters(){return fParameters;}
  RooAbsPdf* GetModel(){return fModel;}
  RooAbsData* GetDataSet(){return fData;}
  TList* GetFits(){return fRooFits;}
  void AddVariables(RooArgList list){fVariables=list;}
  void AddAuxVars(RooArgList list){fAuxVars=list;}
  void AddParameters(RooArgList list){fParameters=list;}
  void AddYields(RooArgList list){fYields=list;}
  void RemoveDataSet();
  TTree* GetTree(){return fTree;}
  THSWeights* GetWeights(){return fWeights;}
  RooWorkspace* GetWorkSpace(){return fWS;}
  void SetTree(TTree* tree){fTree=tree;}
  void PlotDataModel();
  void PlotModel(TString varName,TString pdfName);
  void Factory(TString opt){fWS->factory(opt);}
  void Fit(Bool_t randPar=kFALSE);
  virtual Bool_t  InitialiseFit();
  //sPlot functions
  void sPlot();
  void SetSPlotRange(Double_t min,Double_t max){fSRange[0]=min,fSRange[1]=max;};
  void MergeModelSpecies();
  void SetSingleSpecies(TString ssp){fSingleSp=ssp;};
  void SetIDBranchName(TString str){
    fIDBranchName=str;
    fID=dynamic_cast<RooRealVar*>((fWS->factory(str+"[0,9.99999999999999e14]")));
    fWS->defineSet("ID",RooArgSet(*fID));
   }
  Double_t GetSWeight(Long64_t event,TString species);
  void ExportWeights(TString wmname="WeightMap");
  void ExportTreeWeights(TString wmname="WeightMap");
  void ExportWeightsToFile(TString filename);
  void AddWeightMap(THSWeights* Wts);
  void DrawTreeVar(TString VarName,Int_t nbins,Double_t hmin,Double_t hmax);
  void CorrelationWithVars(TString VarName);
  THSRooFit*  CreateSubFit(TNamed cut); //allow individual cuts
  THSRooFit*  CreateSubFitBins(TNamed cut);//from predefined entrylists
  THSRooFit*  CreateSubFitBins(TTree* ctree,Bool_t CopyTree=kTRUE);//from alredy selected tree
  void AddSubWeights();
  void SaveHists(TString filename);
  void SavePlots(TString filename);
  virtual void RunWeights(Int_t Nfits=1);
  virtual void FitMany(Int_t Nfits=1);
  void RandomisePars();
  void MakeBins();
  virtual void RunWithBins(Int_t Nfits=1);
  virtual void PrepareForFarm();
  THSBins* GetBins(){return fDataBins;}
  void SetOutDir(TString name){fOutDir=name;fOutDir+="/";gSystem->MakeDirectory(fOutDir);}
  void WriteToFile(TString fname);
  void SetBinnedFit(Bool_t bf=kTRUE){fBinnedFit=bf;}
  ClassDef(THSRooFit, 0)  // RooFit interface fit class, 

};


#endif //ifdef THSROOFIT
