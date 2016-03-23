#define CLAStoHS_cxx
// The class definition in CLAStoHS.h has been generated automatically
// by the ROOT utility TTree::MakeSelector(). This class is derived
// from the ROOT class TSelector. For more information on the TSelector
// framework see $ROOTSYS/README/README.SELECTOR or the ROOT User Manual.

// The following methods are defined in this file:
//    Begin():        called every time a loop on the tree starts,
//                    a convenient place to create your histograms.
//    SlaveBegin():   called after Begin(), when on PROOF called only on the
//                    slave servers.
//    Process():      called for each event, in this function you decide what
//                    to read and fill your histograms.
//    SlaveTerminate: called at the end of the loop on the tree, when on PROOF
//                    called only on the slave servers.
//    Terminate():    called at the end of the loop on the tree,
//                    a convenient place to draw/fit your histograms.
//
// To use this file, try the following session on your Tree T:
//
// Root > T->Process("CLAStoHS.C")
// Root > T->Process("CLAStoHS.C","some options")
// Root > T->Process("CLAStoHS.C+")
//

#include "CLAStoHS.h"
#include <TH2.h>
#include <TStyle.h>
#include <iostream>
using namespace std;

TLorentzVector gTarget(0,0,0,9.38271999999999995e-01);
/*
Note, I can choose to either ignore or count ghost tracks when considering events in the final state
By default it ignores them by calling the function IsGoodEventGhosts();
To count them you should comment this out and use IsGoodEvent() instead in Process()
//if(!IsGoodEvent()) return kTRUE; //not the event we want so exit
if(!IsGoodEventGhosts()) return kTRUE; //not the event we want so exit
*/
CLAStoHS::~CLAStoHS(){

  SafeDelete(fHSgamma);
  SafeDelete(fMissing);
}
void CLAStoHS::Begin(TTree * /*tree*/)
{

  TString option = GetOption();
  THSOutput::HSBegin(fInput,fOutput);
}

void CLAStoHS::SlaveBegin(TTree * /*tree*/)
{
  TString option = GetOption();
  fStrParticles=option;
  //fStrParticles="pi+:pi+:pi-";
  THSOutput::HSSlaveBegin(fInput,fOutput);
  
   //Output Tree is initialised can now add further branches if needed
   //note you are creating these yourself and so are responsible for deleting them
   //You will also have to declare the objects in the .h file
   Int_t buff=32000;
   Int_t split=0;//note split is important in the TSelector framework, if increased branches in subsequent selectors will be data members of the THSParticle object rather than the whole object (this can lead to name conflicts)
   fHSgamma=new THSParticle(22); //beam
   fMissing=new TLorentzVector(); //missing
   fOutTree->Branch("beam",&fHSgamma,buff,split);
   fOutTree->Branch("miss",&fMissing,buff,split);
  
}

Bool_t CLAStoHS::Process(Long64_t entry)
{
  THSOutput::HSProcessStart(entry);
  
  //Check if this event has the correct final state, see fFinalState
  /// if(!IsGoodEvent()) return kTRUE; //not the event we want so exit
   if(!IsGoodEventGhosts()) return kTRUE; //not the event we want so exit
   //check for a good photon
   GetEventPartBranches(entry); //get just the branches required
   //look for photons in 2.0ns window, note 2ns looks a bit too large a window
   //first arguement is centre position, 2nd is window (half size)
   if(!MakeBeam(0,2.0)) return kTRUE;
   //now arrange the detected particles into THSParticles
   MakeDetected();

   //Finished THSOutput stuff

   //can now make further reconstructed particles and make cuts etc.
   *fMissing=gTarget+fHSgamma->P4();
   for(UInt_t ip=0;ip<fNdet;ip++)*fMissing-=fDetParticle[ip]->P4();

   //note if I wanted to make a K0, I can look in the output message for which
   //fDetParticle[?] indice corresponds to the pi- and pi+
   //Then fK0=fDetParticle[x]->P4()+fDetParticle[y]->P4();

   //example cut, missing mass must be within 50MeV of target mass
   // if(TMath::Abs(fMissing.M()-gTarget.M())>0.05) return kTRUE; //failed the cut, don't fill
   //all particles reconstructed, all cuts past, fill the output tree
 
   THSOutput::HSProcessFill();
   return kTRUE;
}

void CLAStoHS::SlaveTerminate()
{
  HSSlaveTerminate();
}

void CLAStoHS::Terminate()
{
  HSTerminate();  
}


//Functions used in Process
Bool_t CLAStoHS::IsGoodEvent(){
  //this algorithm uses std::vec to compare the particle ids of the event
  //and those defined in fFinalState, event is good if they match

  //only get branches used
  b_gpart->GetEntry(fEntry);
  if(gpart!=(Int_t)fNdet) return kFALSE;//only analyse event with correct final state
  //Check the tracking status is OK for all tracks (Must need to apply an efficiency correction for this perhaps MC already does)
  b_stat->GetEntry(fEntry);
  for(UInt_t istat=0;istat<fNdet;istat++) if(stat[istat]<1) return kFALSE;

  b_id->GetEntry(fEntry); //particle id branch
  //store the particle IDs in vector to compare with fFinalState
  vector<Int_t>EventState;
  for(UInt_t ifs=0;ifs<fNdet;ifs++) EventState.push_back(id[ifs]);
  //put in order for comparison with requested fFinalState
  std::sort(EventState.begin(),EventState.begin()+gpart);
  //now compare with the particles defined in fFinalState 
  if(!(std::equal(EventState.begin(),EventState.end(),fFinalState.begin()))) return kFALSE; // return if IDs are not those requested
  else return kTRUE; //it is a good event
}
Bool_t CLAStoHS::IsGoodEventGhosts(){
  //this algorithm uses std::vec to compare the particle ids of the event
  //and those defined in fFinalState, event is good if they match

  //only get branches used
  b_gpart->GetEntry(fEntry);
  //  if(gpart!=(Int_t)fNdet) return kFALSE;//only analyse event with correct final state
  //Check the tracking status is OK for all tracks (Must need to apply an efficiency correction for this perhaps MC already does)
  b_stat->GetEntry(fEntry);
  // for(UInt_t istat=0;istat<fNdet;istat++) if(stat[istat]<1) return kFALSE;

  b_id->GetEntry(fEntry); //particle id branch
  //store the particle IDs in vector to compare with fFinalState
  vector<Int_t>EventState;
  Int_t nreal=0;
  for(UInt_t ifs=0;ifs<gpart;ifs++) {
       if(stat[ifs]>0) {fID[ifs]=ParticleID(ifs);EventState.push_back(fID[ifs]);nreal++;}
       else {fID[ifs]=0;}
  }
  if(nreal!=(Int_t)fNdet) return kFALSE;//only analyse event with correct final state
    //put in order for comparison with requested fFinalState
  std::sort(EventState.begin(),EventState.begin()+nreal);
  //now compare with the particles defined in fFinalState 
  if(!(std::equal(EventState.begin(),EventState.end(),fFinalState.begin()))) return kFALSE; // return if IDs are not those requested
  else return kTRUE; //it is a good event
}


void CLAStoHS::MakeDetected(){
 //this function controls the interfaciing of the reconstructed data to THSParticles
  Int_t iIDall[gpart]; //array containing order of particle IDs in id array for all tracks (incl ghosts) 
  Int_t iID[fNdet]; //array containing order of particle IDs in id array for "real" tracks
  TMath::Sort((Int_t)gpart,fID,iIDall,kFALSE); //order the array in asscending order(kFALSE), e.g. -211,211,211
  //write only the indexes of the real tracks to the ID array
  UInt_t Ndet=0;
   for(Int_t ireal=0;ireal<gpart;ireal++) if(stat[iIDall[ireal]]>0)iID[Ndet++]=iIDall[ireal];
 //the ordering in iID[] should now match fFinalState and fEventSate
  //loop over different particle types
  Ndet=0;
  for(UInt_t itype=0;itype<fNtype.size();itype++){ 
      if(fNtype[itype]==1)MakeParticle(fDetParticle[Ndet],iID[Ndet]); 
     else{//order particles of same type fastest first
      //get an array with the momentum of particles of this type
      Double_t typemom[fNtype[itype]];
      Int_t imom[fNtype[itype]];
      for(UInt_t intype=0;intype<fNtype[itype];intype++) typemom[intype]=p[iID[Ndet+intype]];
      TMath::Sort((Int_t)fNtype[itype],typemom,imom,kTRUE); //order the array in decreasing order(kTRUE)
      for(UInt_t intype=0;intype<fNtype[itype];intype++) MakeParticle(fDetParticle[Ndet+intype],iID[Ndet+imom[intype]]);  //function which maps the original variables into THSParticle 
     }
     Ndet+=fNtype[itype];//move on to the next particle type
  }//end itype loop
}

void CLAStoHS::MakeParticle(THSParticle* hsp,Int_t ip){
  //set the vertex
  hsp->SetVertex(vx[ip],vy[ip],vz[ip]);
  //set the intitial Lorentz Vector
  hsp->SetXYZM(p[ip]*cx[ip],p[ip]*cy[ip],p[ip]*cz[ip],hsp->PDGMass());
  //set the measured mass
  hsp->SetMeasMass(sqrt(m[ip]));
  //hsp->SetMeasMass((m[ip]));
  //calculate the vertex time
  hsp->SetTime(sc_t[sc[ip]-1]-sc_r[sc[ip]-1]/hsp->P4().Beta()/29.9792458-tr_time);

}
Bool_t CLAStoHS::MakeBeam(Float_t Tmid,Float_t Tcut){
  //Find a photon within a Tcut window of Tmid  
  //and throw away events with more than one photon
  Bool_t gotGamma=kFALSE;
  for(Int_t im=0;im<taggoodhit;im++) if(TMath::Abs(dt_st_tag[im])+Tmid<Tcut){
      if(gotGamma==kTRUE)return kFALSE; //ambigous photon event throw away for now
      fHSgamma->SetXYZT(0,0,tag_energy[im],tag_energy[im]);
      fHSgamma->SetTime(vertex_time[im]);//=-tr_time
      gotGamma=kTRUE;
    }
  if(!gotGamma) return kFALSE; //didn't get a photon event no good
  return kTRUE; //got one good photon
}

void CLAStoHS::GetEventPartBranches(Int_t evi){
  b_cx->GetEvent(evi);
  b_cy->GetEvent(evi);
  b_cz->GetEvent(evi);
  b_vx->GetEvent(evi);
  b_vy->GetEvent(evi);
  b_vz->GetEvent(evi);
  b_p->GetEvent(evi);
  b_m->GetEvent(evi);
  b_q->GetEvent(evi);
  //timing
  b_sc->GetEvent(evi);
  b_sc_t->GetEvent(evi);
  b_tr_time->GetEvent(evi);

  //tagger
  b_taggoodhit->GetEvent(evi);
  b_tag_energy->GetEvent(evi);
  b_dt_st_tag->GetEvent(evi);
  b_vertex_time->GetEvent(evi);
}

Int_t  CLAStoHS::ParticleID(Int_t itrk){
  //Choose a particle ID method
  return h10ID(itrk); // Use the h10 ntuple EVNT bank ID
  // return MassID(itrk); // Use the h10 ntuple EVNT bank ID

}
Int_t  CLAStoHS::h10ID(Int_t itrk){
 // Use the h10 ntuple EVNT bank ID
  return id[itrk]; 
}
Int_t  CLAStoHS::MassID(Int_t itrk){
  b_m->GetEntry(fEntry);//get the mass for this event
 // Use the h10 ntuple EVNT mass
  double mass2_limits[]={0,0.15,0.4,1.44,1E10}; //define mass^2 ranges for particles
  int mass_pdg[]={11,211,321,2212,0};    //PDG id corresponding to mass2_limit
  Int_t ip=0;
  while(m[itrk]>=mass2_limits[ip]) ip++;

  Int_t tid=mass_pdg[ip]*TMath::Sign(1,id[itrk]);  //sign of id= charge
  // cout<<itrk<<" "<<tid<<" "<<mass_pdg[ip]<<" "<<m[itrk]<<" "<<endl;
  return tid;
}
