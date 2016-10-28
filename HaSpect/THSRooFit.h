#ifndef THSROOFIT_h
#define THSROOFIT_h
#include <RooWorkspace.h>
#include <RooAddPdf.h>
#include <RooDataSet.h>
#include <RooDataHist.h>
#include <RooFitResult.h>
#include <RooLinkedList.h>
#include <RooArgList.h>
#include <RooGaussian.h>
#include <RooCmdArg.h>
#include <RooRealVar.h>
#include <RooAbsArg.h>
#include <RooAbsData.h>
#include <RooPlot.h>
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
  RooArgList fConstraints;//constraints on  parameters
  RooRealVar *fID;
  RooAbsPdf* fModel; //model to be fitted to data
  RooAbsData* fData; //dataset to be fitted
  TList* fCanvases;  //canvases for plotting fitted variables
  TList* fHists;  //histograms for plotting weighted variables
  TList* fRooFits;  //collection of sub fits
  TTree* fTree; //!     // tree to be imported, don't save as part of class!
  TString fIDBranchName;
  TString fYld;     //yield variable prepend
  TString fSingleSp;
  TString fOutDir;
  TString fWeightName; //Input Weight species for this dataset 
  RooFitResult* fResult;   //RooFit result
  THSWeights* fInWeights; //! //input weights for dataset to be fitted
  THSBins* fDataBins; //! 
  Bool_t fGotID;
  Bool_t fBinnedFit;
  Bool_t ftoWS;
  Double_t fChi2;
  Int_t fFiti;
  RooLinkedList fFitOptions;
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
  void DefineSets();                  // define sets in the workspace
  void LoadDataSet(TTree* tree,Bool_t toWS=kFALSE);    //Import a tree into a RooDataSet
  void LoadDataSet(RooAbsData* data,Bool_t toWS=kFALSE);    //Import a tree into a RooDataSet
  void LoadWeights(TString wfile,TString wname);
  void SetDataWeight(); //Add a weight to RooFit dataset
  void LoadWorkSpace(RooWorkspace* ws); //load a workspace without data
  void LoadWorkSpaceData(RooWorkspace* ws); //load a workspace with data
  void TotalPDF(); //sum different PDFs if different species
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
  RooArgList GetConstraints(){return fConstraints;}
  RooAbsPdf* GetModel(){return fModel;}
  RooAbsData* GetData(){return fData;}
  RooDataSet* GetDataSet(){return dynamic_cast<RooDataSet*>(fData);}
  RooDataHist* GetDataHist(){return dynamic_cast<RooDataHist*>(fData);}
  TList* GetFits(){return fRooFits;}
  void AddVariables(RooArgList list){fVariables=list;}
  void AddAuxVars(RooArgList list){fAuxVars=list;}
  void AddParameters(RooArgList list){fParameters=list;}
  void AddYields(RooArgList list){fYields=list;}
  void AddGausConstraint(RooGaussian *pdf){if(!pdf) return; fWS->import(*((RooAbsPdf*)pdf->Clone()));fConstraints.add(*(fWS->pdf(pdf->GetName())));};
  void SetSingleSpecies(TString ssp){fSingleSp=ssp;};
  void SetIDBranchName(TString str){//Probably just needed for sPlot but keep here
    fIDBranchName=str;
    fID=dynamic_cast<RooRealVar*>((fWS->factory(str+"[0,9.99999999999999e14]")));
    fWS->defineSet("ID",RooArgSet(*fID));
   }
   void RemoveDataSet();
   void RemoveConstraints(){fConstraints.removeAll();};
  TTree* GetTree(){return fTree;}
  RooWorkspace* GetWorkSpace(){return fWS;}
  void SetTree(TTree* tree){fTree=tree;}
  void PlotDataModel();
  void PlotModel(TString varName,TString pdfName);
  void Factory(TString opt){fWS->factory(opt);}
  void Fit(Bool_t randPar=kFALSE);
  virtual Bool_t  InitialiseFit();
  THSRooFit*  CreateSubFit(TNamed cut); //allow individual cuts
  THSRooFit*  CreateSubFitBins(TNamed cut);//from predefined entrylists
  virtual THSRooFit*  CreateSubFitBins(TTree* ctree,Bool_t CopyTree=kTRUE);//from alredy selected tree
  void SavePlots(TString filename);
  virtual void FitMany(Int_t Nfits=1);
  void RandomisePars();
  void MakeBins();
  virtual void FitWithBins(Int_t Nfits=1);
  virtual void PrepareForFarm();
  THSBins* GetBins(){return fDataBins;}
  void SetOutDir(TString name){fOutDir=name;fOutDir+="/";gSystem->MakeDirectory(fOutDir);}
  TString GetOutDir(){return fOutDir;}
  void WriteToFile(TString fname);
  void SetBinnedFit(Bool_t bf=kTRUE){fBinnedFit=bf;}
  void SetInWeights(THSWeights* wts){fInWeights=wts;}
  void SetWeightName(TString WName);
  void DefaultFitOptions();
  void AddFitOption(RooCmdArg cmd){fFitOptions.Add((RooCmdArg*)cmd.Clone());}
  RooLinkedList GetFitOptions(){return fFitOptions;}
  ClassDef(THSRooFit, 0)  // RooFit interface fit class, 

};


#endif //ifdef THSROOFIT
