//--Author      DI Glazier 25/01/2015
//--Rev
//--Update
//--Description
//HASPECT Event Reconstruction
//THSLongPS
//Class to perform general Longitudinal Phasespace Analysis

#ifndef THSLongPS_h
#define THSLongPS_h
#include <TLorentzVector.h>
#include <TString.h>
#include <vector>
#include <iostream>
#include <algorithm>

class THSLongPS {
 protected :
  Int_t fNpart;  //Number of particles in final state
  vector<TLorentzVector> fP4s; //vector of CM 4 vectors
  
  vector< vector<Bool_t> > fIsForward;      //Which particles are going forward
  Int_t fSector; //which longitudnal region is this event in
  Int_t fNSector; //number of  longitudnal regions
  vector<TString> fSectName; //vector of names corresponding to fSector
  vector< vector<Int_t> > fITop; //indices of particles in top branch for each sector
  vector< vector<Int_t> > fIBot; //indices of particles in top branch
  TLorentzVector fCM; //CM 4 vector = sum of fP4s; 
  Float_t fMTop; //MAss of particle on top branch
  Float_t fMBot; //MAss pf particles on bottom branch
  Float_t fCosTh; //Cos centre of mass angle
  Int_t fSize;//number of 4 vectors loaded for this event
 public :
  THSLongPS(){};
  THSLongPS(Int_t Np);
  virtual ~THSLongPS(){};
  
  //void Initialise(); //Set fNpart, reserve space in vectors
  void AddParticle(TLorentzVector p4);//add 4 vectos should be done for each particle for each event
  void Reset(){fSize=0;fCM.SetXYZT(0,0,0,0);}
  void Analyse(); // do the kinematics and find the current sector
//void RemoveSector();//Remove a sector if it is not very interesting

  Float_t GetTopMass(){return fMTop;};
  Float_t GetBotMass(){return fMBot;};
  Float_t GetCosTh(){return fCosTh;};
  Int_t GetSector(){return fSector;};
  Int_t GetNSector(){return fNSector;};
  TString GetSectName(Int_t in){return fSectName[in];}
  void MakeIndices(Int_t Np,Int_t Nt);
  void PrintVector(vector<Int_t> );
  void PrintVector(vector<Bool_t> );
};

#endif
