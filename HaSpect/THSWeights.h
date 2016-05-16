#ifndef THSWEIGHTS_h
#define THSWEIGHTS_h

#include <TTree.h>
#include <TNamed.h>
#include <TList.h>
#include <TVectorD.h>
#include <map>
#include <iostream>
using namespace std;

typedef map<TString, Int_t > StrIntMap_t;

class THSWeights : public TNamed{
 private:
  TTree *fWTree;  //tree of weights, branchname = species
  TTree *fIDTree;  //tree of ids, branchname = species
  StrIntMap_t fSpecies;//names of species with index in map
  TList* fWeightList; //list of weight bins which have been merged to make this
  TFile* fFile;
  TVectorD fWVals;
  Long64_t fID;
  Long64_t fCurrEntry;
  Bool_t fGotEntry;
  Bool_t fIsSorted;
  Long64_t fN;
  Long64_t *fIDi;//!
  Long64_t *fIDv;//!
 public:
 THSWeights() : fWTree(0),fIDTree(0),fWeightList(0),fFile(0),fIDi(0),fIDv(0){};
   THSWeights(TString name);
  ~THSWeights();
  
  TTree* GetIDTree(){return fIDTree;};
  void SetIDTree(TTree* tree){fIDTree=tree;}
  TTree* GetTree(){return fWTree;};
  void SetTree(TTree* tree){fWTree=tree;}
  void FillWeights(Long64_t ev,TVectorD wgt){ fID=ev; fWVals=wgt; fWTree->Fill();fIDTree->Fill();fN++;}
  void GetEntry(Long64_t ent){fWTree->GetEntry(ent);fIDTree->GetEntry(ent);}; 
  Bool_t GetEntryFast(Long64_t id); //use id branch with sorted tree
  Bool_t GetEntrySlow(Long64_t id); //use id branch
  Bool_t GetEntryBinarySearch(Long64_t id); //use binary search to give faster entrys when used by unsorted trees
  Double_t GetWeight(TString spe){if(fSpecies.count(spe))return GetWeight(fSpecies[spe]);return 0;}
  Double_t GetWeight(Int_t ispe){
    if(fGotEntry)
      return fWVals[ispe];
    else return 0; //entry for id not found
  }
  Long64_t* GetIDi(){return fIDi;};
  void SetCurrEntry(Long64_t ent){fCurrEntry=ent;}
  Long64_t GetCurrEntry(){return fCurrEntry;}
  Long64_t Size(){return fWTree->GetEntries();}
  void Add(THSWeights* wm);
  // void Multiply(THSWeights* other,TString species);
  void SetSpecies(TString name);
  Int_t GetNSpecies(){return fSpecies.size();}
  StrIntMap_t GetSpecies(){return fSpecies;}
  StrIntMap_t* GetSpeciesp(){return &fSpecies;}
  TString GetSpeciesName(UInt_t isp);
  void SetSpecies(StrIntMap_t species){fSpecies=species;};
  Int_t GetSpeciesID(TString name){if(fSpecies.count(name))return fSpecies[name]; else {cout<<"THSWeights:: GetSpeciesID species not found "<<endl;return -1;}}
  TList* GetWeightList(){return fWeightList;}
  void PrintWeight();
  Long64_t Merge(TString tempName,TString outName="",TString wmName="WeightMap");
  void SortWeights();
  void BuildIndex();
  void SetFile(TString filename);
  void Save();
  void Mem();
  void Disk();
  void LoadSaved(TString fname,TString wname);

  ClassDef(THSWeights, 1)  // Writeble Weight map  class
};

#endif //ifdef THSWeights
