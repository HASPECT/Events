#ifndef THSROOFIT_h
#define THSROOFIT_h
#include <RooWorkspace.h>
#include <RooAddPdf.h>
#include <RooDataSet.h>
#include "RooStats/SPlot.h"
#include "RooFitResult.h"
#include <TNamed.h>
#include <TChain.h>
#include <TCanvas.h>
#include <TList.h>
#include <TFile.h>
#include <THSBins.h>
#include <vector>
#include <map>
using namespace RooFit;
using namespace std;

typedef vector<Double_t> VecDouble_t;
typedef vector<VecDouble_t> VecVecDouble_t;
typedef map<Long64_t, VecDouble_t > WeightMap_t;
typedef map<TString, Int_t > StrIntMap_t;
#pragma link C++ typedef vector<Double_t> VecDouble_t+;
#pragma link C++ typedef vector<VecDouble_t> VecVecDouble_t+;
#pragma link C++ typedef  map<Long64_t, VecDouble_t > WeightMap_t+;
#pragma link C++ class WeightMap_t+;
#pragma link C++ typedef map<TString, Int_t > StrIntMap_t+;
#pragma link C++ class StrIntMap_t+;

class THSWeightMap : public TNamed{
 private:
  WeightMap_t fWeights;  //map of weights, key =id, value =vector of weights for different species
  StrIntMap_t fSpecies;
 public:
  THSWeightMap() {};
  THSWeightMap(TString name) :TNamed(name,name){};
  ~THSWeightMap(){};
  
  WeightMap_t GetMap(){return fWeights;};
  WeightMap_t* GetMapp(){return &fWeights;};
  void SetWeight(Long64_t ev,VecDouble_t wgt){fWeights.insert(pair<Long64_t,VecDouble_t>(ev,wgt));}
  Double_t GetWeight(Long64_t ev,Int_t sp){return fWeights.at(ev)[sp];}
  Long64_t Size(){return fWeights.size();}
  void SetMap(TString file,TString name){
    TFile* mfile=new TFile(file);
    WeightMap_t *wmap=0;
    mfile->GetObject(name,wmap);
    fWeights.clear();
    fWeights.insert(wmap->begin(),wmap->end());
    StrIntMap_t *spe=0;
    mfile->GetObject(name+TString("Species"),spe);
    SetSpecies(*spe);
    mfile->Close();
    delete mfile;wmap=0;
  }
  void Add(WeightMap_t wmap){fWeights.insert(wmap.begin(),wmap.end());}
  void Multiply(THSWeightMap* other,TString species);
  void SetSpecies(Int_t ii,TString name){fSpecies.insert(pair<TString,Int_t>(name,ii));}
  StrIntMap_t GetSpecies(){return fSpecies;}
  StrIntMap_t* GetSpeciesp(){return &fSpecies;}
  void SetSpecies(StrIntMap_t species){fSpecies=species;};
  Int_t GetSpeciesID(TString name){return fSpecies[name];}
  virtual void PrintWeight();
  ClassDef(THSWeightMap, 0)  // Writeble Weight map  class
};


class THSRooFit : public TNamed {
 protected:
  RooWorkspace* fWS;
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
  TTree* fTree;      // tree to be imported
  RooStats::SPlot* fSPlot; //sPlot object
  RooFitResult* fResult;   //RooFit result
  TString fIDBranchName;
  Bool_t fGotID;
  TString fYld;     //yield variable prepend
  THSWeightMap* fWMap;
  THSBins* fDataBins;
  TString fOutDir;
 public:
  THSRooFit() ;
  THSRooFit(TString name):TNamed(name,name) {};
  THSRooFit(THSRooFit* rf) ;
  virtual ~THSRooFit();

  void LoadVariable(TString opt);   //Load the variables to be fitted
  void LoadVariable(RooRealVar* var){fVariables.add(*var);fWS->import(*var);}
  void LoadAuxVars(TString opt);   //Load the variables not to be fitted
  void LoadAuxVars(RooRealVar* var){fAuxVars.add(*var);fWS->import(*var);}
  void LoadBinVars(TString opt,Int_t nbins,Double_t min,Double_t max);
  void LoadSpeciesPDF(TString opt); //Load a contributing species
  void TotalPDF();                  // Make the total fit pdf
  void LoadDataSet(TTree* tree);    //Import a tree into a RooDataSet
  void LoadDataSet(RooAbsData* data);    //Import a tree into a RooDataSet
  void LoadWorkSpace(RooWorkspace* ws); //load a workspace without data
  RooArgList GetVariables(){return fVariables;}
  RooRealVar* GetVariable(TString name){return dynamic_cast<RooRealVar*>(fVariables.find(name));}
  RooArgList GetAuxVars(){return fAuxVars;}
  RooArgList GetBinVars(){return fBinVars;}
  RooRealVar* GetAuxVar(TString name){return dynamic_cast<RooRealVar*>(fAuxVars.find(name));}
  RooRealVar* GetBinVar(TString name){return dynamic_cast<RooRealVar*>(fBinVars.find(name));}
  RooRealVar* GetVar(TString name);
  RooArgList GetYields(){return fYields;}
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
  THSWeightMap* GetWeights(){return fWMap;}
  RooWorkspace* GetWorkSpace(){return fWS;}
  void SetTree(TTree* tree){fTree=tree;}
  void PlotDataModel();
  void PlotModel(TString varName,TString pdfName);
  void Factory(TString opt){fWS->factory(opt);}
  void Fit();
  //sPlot functions
  void sPlot();
  void SetIDBranchName(TString str){
    fIDBranchName=str;
    fID=dynamic_cast<RooRealVar*>((fWS->factory(str+"[0,9.99999999999999e14]")));
    fWS->defineSet("ID",RooArgSet(*fID));
   }
  Double_t GetSWeight(Long64_t event,TString species);
  //void ExportWeights(TString filename="",TString wmname="WeightMap");
  void ExportTreeWeights(TString wmname="WeightMap");
  void ExportWeightsToFile(TString filename);
  void AddWeightMap(WeightMap_t wm);
  void DrawTreeVar(TString VarName,Int_t nbins,Double_t hmin,Double_t hmax);
  void CorrelationWithVars(TString VarName);
  THSRooFit*  CreateSubFit(TNamed cut); //allow individual cuts
  THSRooFit*  CreateSubFitBins(TNamed cut);//from predefined entrylists
  THSRooFit*  CreateSubFitBins(TTree* ctree);//from alredy selected tree
  void AddSubWeights();
  void SaveHists(TString filename);
  void SavePlots(TString filename);
  virtual void RunWeights();
  void MakeBins();
  void MakeTreeBins();
  virtual void RunWithBins();
  virtual void RunWithTrees();
  THSBins* GetBins(){return fDataBins;}
  void SetOutDir(TString name){fOutDir=name;}
  ClassDef(THSRooFit, 0)  // RooFit interface fit class

};


#endif //ifdef THSROOFIT
