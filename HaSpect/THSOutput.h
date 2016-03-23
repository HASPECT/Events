//--Author      DI Glazier 30/06/2014
//--Rev
//--Update
//--Description
//HASPECT Event Reconstruction
//THSOuput
//Controls the output of the TSelector analysis
//Creates THSParticle branches in ouput tree
//Writes source code to output file
//Writes either one combined file, or one file for each input file 
//fStrParticles should be passed a string of particles to be
//searched for and output in the tree, e.g. "pi+:pi-:pi+:neutron"
//the particles will be ordered in fDetParticle[i] for further analysis
//ordered by type then decreasing momentum, see output message for connecting
//branch names to fDetParticle indices
//After making a skeleton selector class you can add inheritance from this class
//to obtain its functionality

#ifndef THSOutput_h
#define THSOutput_h

#include <TFile.h>
#include <TChain.h>
#include <TList.h>
#include <TString.h>
#include <TDirectory.h>
#include <TMacro.h>
#include <vector>
#include "THSParticle.h"
#include "THSHisto.h"

class TProofOutputFile;
class THSHisto;

class THSOutput : public THSHisto{
 protected :
  TFile            *fFile;      //Output file
  TProofOutputFile *fProofFile; // For optimized merging of the ntuple
  TTree            *fOutTree;  //Output tree
  TTree            *fCurTree;  //Current input tree
  TEntryList* fEntryList; //Entry List prooduced during selector Process
  TString fOutName;   //name of ouput file or directory
  TString fStrParticles; //string containing detected particles name
  TString fStepName; //Name of directory in fFile to write source code to
  TList* fStepDir; //Directory containing source code
  TList* fSelInput;  //pointers to the selector input
  TList* fSelOutput;  //pointer to the selector ouput
  TList* fCodeList;  //pointer to the list of source code
  Long64_t fEntry; //current entry number in Chain
  vector<Int_t> fFinalState; //vector holding the particle ids selected in final state
  vector<UInt_t> fNtype; //vector holding the number of different particles in fFinalState
  vector<UInt_t> fIDtype; //vector holding the PDG_ID of different particles in fFinalState
  UInt_t fNdet; //number of particles detected (the size of fFinalState)
  vector<Int_t> fDetID;
  vector<THSParticle*> fDetParticle;
 
  TFile* fParentFile;//Parent file
  TTree* fParentTree;//Parent tree if you need to connect to parent branches
  TList* fParEntryLists;

  Bool_t fSaveID; //Ony save ID if this is the first THSOutput tree, subsequent trees will aready contain this branch
  Double_t fgID; //global event ID number, should be set in first instance of THSOutput and preserved through further steps. Required to synchornise differnt trees as PROOF does not preserve event ordering
  Long64_t fgIDoff; //starting offset for ID number
 public :
 THSOutput() : fFile(0), fProofFile(0), fOutTree(0),fCurTree(0), fEntryList(0),fStepDir(0),fSelInput(0),fSelOutput(0), fCodeList(0),fParentFile(0),fParentTree(0), fParEntryLists(0) {fSaveID=kFALSE;fgID=0;fgIDoff=0;}   
  virtual ~THSOutput();

  virtual void InitOutput(); //Configure the output file and tree 
  virtual void FinishOutput();
  //virtual void NextFile(TTree* chain);  //call function when switch files in chain
  virtual void InitOutFile(TTree* chain);
  void InitOutTree();
  void PrepareOutDir(); //create output file directory
  Int_t CountChar(TString tests,char testc);
  void CopyCode(TDirectory* curDir=0,TDirectory* prevDir=0);  //Give new file,curDir to copy to, and and previous analysis steps, prevDir
  void ImportSysDirtoList(const char *dirname,TList* list);  //look through the given OS directory and save source code files

  //Utility functions for copying objects from directories to lists 
  //and writing from lists to directories
  //these were created as it proved too much effort to use Tdirectory throughout 
  TList* CopyDirtoList(TDirectory *source); //write source to list
  void WriteListtoFile(TList* list0); //write to current directory

  //Mimic Selector functions
  void HSBegin(TList* input,TList* output);
  void HSSlaveBegin(TList* input,TList* output);
  void HSProcessStart(Long64_t entry);
  void HSProcessFill();
  void HSSlaveTerminate();
  void HSNotify(TTree* tree);
  void HSTerminate();

  //Functions to connect (make friends) with parent selectors
  void InitParent(TTree* ctree,TString step);
  Long64_t GetParentEntry(Long64_t parentry);
  void SortTree(TTree* tree);
  
  TTree* GetOutTree() {return fOutTree;}
  THSParticle* GetHSParticle(Int_t np){return fDetParticle[np];}
  vector<UInt_t> GetNType(){return fNtype;}
  vector<Int_t> GetFinalState(){return fFinalState;}

  void SetIDoff(Long64_t off) {fgIDoff=off;}
  //Setter functions may be needed if using without TSelector
  void SetParticles(TString sp){fStrParticles=sp;}
};
#endif
