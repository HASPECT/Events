#define MCCLAStoHS_cxx
// The class definition in MCCLAStoHS.h has been generated automatically
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
// Root > T->Process("MCCLAStoHS.C")
// Root > T->Process("MCCLAStoHS.C","some options")
// Root > T->Process("MCCLAStoHS.C+")
//

#include "MCCLAStoHS.h"
#include <TH2.h>
#include <TH3.h>
#include <TStyle.h>
#include <TProofOutputFile.h>
#include <iostream>
#include <utility>
#include "eLoss.h"              //For energy loss correc
using namespace std;

TLorentzVector gTarget(0,0,0,9.38271999999999995e-01);
/*
Note, I can choose to either ignore or count ghost tracks when considering events in the final state
By default it ignores them by calling the function IsGoodEventGhosts();
To count them you should comment this out and use IsGoodEvent() instead in Process()
//if(!IsGoodEvent()) return kTRUE; //not the event we want so exit
if(!IsGoodEventGhosts()) return kTRUE; //not the event we want so exit
*/
MCCLAStoHS::~MCCLAStoHS(){

  SafeDelete(fHSgamma);
  SafeDelete(fMissing);
  SafeDelete(fATE);
  SafeDelete(fATPx);
  SafeDelete(fATPy);
  SafeDelete(fATPz);
  SafeDelete(fATPdg);
}
void MCCLAStoHS::Begin(TTree * /*tree*/)
{

  TString option = GetOption();
  THSOutput::HSBegin(fInput,fOutput);
}

void MCCLAStoHS::SlaveBegin(TTree * /*tree*/)
{
  TString option = GetOption();
  fStrParticles=option;
  //fStrParticles="pi+:pi+:pi-";
  //Remove missing particle from string leaving detected
  fMissPDG=0;
  if(fStrParticles.Contains("AT")){
    fStrMiss=fStrParticles;
    fStrParticles.Remove(fStrParticles.First("AT")-1,fStrParticles.Sizeof()); //remove the extra AmpTools particle
    fStrMiss.Remove(0,fStrMiss.First("AT")+2); //remove the detected particles
    fMissPDG=TDatabasePDG::Instance()->GetParticle(fStrMiss)->PdgCode();
  }
  fNAT=0;
  cout<<"Detecting "<<fStrParticles<<" reconstructing "<<fStrMiss<<endl;
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
   fOutTree->Branch("MCTruth",&fMCParticle,buff);

   //Add branches for reading in AmpTools
   AddATBranches(kFALSE);//true means wipe all other branches
   AddATtrBranches();

   fMCOK=-1;
  //if you want kinematic bins you must define fHisbins here
   //fHisbins=new TH2F("MCcorBins","MCcorBins",20,0,2.5,40,0,4);
  // fHisbins=new TH3F("MCcorBins","MCcorBins",30,0,3,40,0,4,20,-20,20);
   //if(fHisbins) fOutput->Add(fHisbins);
   //fHisbins->SetXTitle("Th");//theta
   //fHisbins->SetYTitle("P"); //momentum
   // fHisbins->SetZTitle("PZ"); //z-vertex
   
   THSHisto::ChangeNames();
   for(UInt_t i=0;i<fNtype.size();i++){//create a correction for each type of detected particle
      TString pName=TDatabasePDG::Instance()->GetParticle(fIDtype[i])->GetName();
      pName.ReplaceAll("+","p");  //Get rid of + and - in names so not to confuse tree->Draw
      pName.ReplaceAll("-","m");  //Get rid of + and - in names so not to confuse tree->Draw
      THSHisto::LoadCut(pName);
      fTypeNames.push_back(std::make_pair(fIDtype[i],pName));
      // fTypeIndex.push_back(std::make_pair(fIDtype[i],i));//use PID to get index
   }
   THSHisto::LoadHistograms();

   //Load a previosly determined mc correction
   TDirectory* savedir=gDirectory;
   TFile* hisCorr=0;
   // hisCorr=new TFile("/home/dglazier/Dropbox/g11sim/g11_pipi/g11_pipi3/");
   // savedir->cd();
   // if(hisCorr->IsOpen())
   //   for(Int_t i=0;i<fNtype.size();i++){
   //     fMCCorr[fTypeNames[i].first]=(TH2F*)hisCorr->Get(fTypeNames[i].second+"Cor")->Clone();
   //   }
   // else cout<<"MCCLAStoHS::SlaveBegin MC Warning Corrections histograms not found"<<endl;

   //init eloss centre x,y,z,st counter
     initELoss(0.,0.,0.,0); 	

}

Bool_t MCCLAStoHS::Process(Long64_t entry)
{
  THSOutput::HSProcessStart(entry);
  //Initialise and check MC truth branches
  if(fMCOK==-1) CheckMCTruth();
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
   //Get the mc truth particles
   MakeDetectedMC();
   //MAke histograms with difference between true and reconstructed
   MCCorrections();

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
    
   doAT(); //Fill the AmpTools branches before tree fill
   doATtr(); //Fill the AmpTools truth branches before tree fill
   THSOutput::HSProcessFill();
   return kTRUE;
}

void MCCLAStoHS::SlaveTerminate()
{
  HSSlaveTerminate();
}

void MCCLAStoHS::Terminate()
{
  HSTerminate();  
   TIter next(fSelOutput);
   TProofOutputFile* elpofile=0;
   TObject* outo=0;
   TFile* infile=0; //pointer to input file
   //loop through the output list and find the ouput files
   //we want to get the histograms and make a correction histogram
   //with same binning as fHisbins
   while((outo=dynamic_cast<TObject*>(next()))){
     if((elpofile=dynamic_cast<TProofOutputFile*>(outo))){
       TFile* hfile = elpofile->OpenFile("UPDATE");
       cout<<"Gong to make corrections for "<<hfile->GetName()<<endl;
       //Loop over Cuts (particles) these are kept in THSHisto
       for(UInt_t ic=0;ic<fVecCuts.size();ic++){
	 cout<<"doing "<<fVecCuts[ic]<<endl;
	 if(!fHisbins) continue;//only do it for histograms in different bins
	 TH2F* hisCor=(TH2F*)fHisbins->Clone();
	 hisCor->SetNameTitle(fVecCuts[ic]+"Cor",fVecCuts[ic]); 
	 //Loop over bins in fHisbins
	 for(Int_t ib=0;ib<(fHisbins->GetNbinsX()+2)*(fHisbins->GetNbinsY()+2);ib++){ //+2 for over and underflow
	   cout<<ib<<endl;
	   TString sLabel;
	   sLabel=fVecCuts[ic]+fVecBinNames[ib]; //make histogram name form cut and bin
	   TH1F* hisDiff=(TH1F*)hfile->Get("Pdiff"+sLabel); //get histogram
	   if(!hisDiff) continue;
	   hisCor->SetBinContent(ib,hisDiff->GetMean());
	   hisCor->SetBinError(ib,hisDiff->GetRMS());
	 }//done for this particle
	 hisCor->Write();
       }//done looping on particles
       hfile->Close();
       delete hfile;hfile=0;
     }//done for all files 
   }
}

//Functions used in Process
Bool_t MCCLAStoHS::IsGoodEvent(){
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
Bool_t MCCLAStoHS::IsGoodEventGhosts(){
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
  for(Int_t ifs=0;ifs<gpart;ifs++) {
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


void MCCLAStoHS::MakeDetected(){
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

void MCCLAStoHS::MakeParticle(THSParticle* hsp,Int_t ip){
  //set the vertex
  hsp->SetVertex(vx[ip],vy[ip],vz[ip]);
  //set the intitial Lorentz Vector
  hsp->SetXYZM(p[ip]*cx[ip],p[ip]*cy[ip],p[ip]*cz[ip],hsp->PDGMass());
  //set the measured mass
  hsp->SetMeasMass(sqrt(m[ip]));
  //hsp->SetMeasMass((m[ip]));
  //calculate the vertex time
  hsp->SetTime(sc_t[sc[ip]-1]-sc_r[sc[ip]-1]/hsp->P4().Beta()/29.9792458-tr_time);
 //do energy loss correction
  hsp->SetP4(eLoss(hsp->P4p(),hsp->PDGMass(),hsp->Vertex(),1,7));
  //If MC correction exists apply it
  if(fMCCorr.size()>0){
    hsp->P4p()->SetRho(Correct_MCTrackMomentum(hsp->PDG(),hsp->P4()));
    hsp->P4p()->SetVectM(hsp->P4p()->Vect(),hsp->PDGMass());
  }
}
Bool_t MCCLAStoHS::MakeBeam(Float_t Tmid,Float_t Tcut){
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

void MCCLAStoHS::GetEventPartBranches(Int_t evi){
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
Int_t  MCCLAStoHS::ParticleID(Int_t itrk){
  //Choose a particle ID method
  return h10ID(itrk); // Use the h10 ntuple EVNT bank ID
  //return MassID(itrk); // Use the h10 ntuple EVNT bank ID

}
Int_t  MCCLAStoHS::h10ID(Int_t itrk){
 // Use the h10 ntuple EVNT bank ID
  return id[itrk]; 
}
Int_t  MCCLAStoHS::MassID(Int_t itrk){
  b_m->GetEntry(fEntry);//get the mass for this event
 // Use the h10 ntuple EVNT mass
  double mass2_limits[]={0,0.15,0.4,1.44,1E10}; //define mass^2 ranges for particles
  int mass_pdg[]={11,211,321,2212,0};    //PDG id corresponding to mass2_limit
  Int_t ip=0;
  while(m[itrk]>=mass2_limits[ip]) ip++;

  Int_t tid=mass_pdg[ip]*id[itrk]/TMath::Abs(id[itrk]);  //sign of id= charge
  // cout<<itrk<<" "<<tid<<" "<<mass_pdg[ip]<<" "<<m[itrk]<<" "<<endl;
  return tid;
}

//MC TRUTH Functions
void MCCLAStoHS::MakeDetectedMC(){
  GetEventMCBranches(fEntry); //get an event to check
 //this function controls the interfaciing to the MC truth particles

  Int_t Ndet=0;
  TVector3 vec3(0,0,1);//for setting 4-vector momentum
  //looping over all generated particles which may be different from erquested particles
  //but we are only going to save requested particles
  Double_t dtr=TMath::DegToRad();
  for(UInt_t idet=0;idet<fNdet;++idet){//loop over detected particles and find corresponding truth
    Double_t pdiff=1E12;
    Int_t imc=-1;
    for(Int_t mctype=0;mctype<mcnentr;mctype++){//find difference with MC truth candidates
      if(mcid[mctype]!=fDetParticle[idet]->PDG()) continue;
      vec3.SetMagThetaPhi(mcp[mctype],mctheta[mctype]*dtr,mcphi[mctype]*dtr);
      Double_t diff=0;
      if((diff=TMath::Abs((vec3-fDetParticle[idet]->P4p()->Vect()).Mag()))<pdiff){ //make diffierence in momentum vectors
	  //keep the id of the smallest diff
	  pdiff=diff;
	  imc=mctype;
	}
    }//end of mc loop
    if(imc>-1){
      vec3.SetMagThetaPhi(mcp[imc],mctheta[imc]*dtr,mcphi[imc]*dtr);
      fMCParticle[idet].SetVectM(vec3,mcm[imc]);
    }
  }//end of det particle loop
}
Bool_t MCCLAStoHS::CheckMCTruth(){

  //check that the final particles requested match those generated
  vector<Int_t>EventState;;
  GetEventMCBranches(fEntry); //get an event to check
  cout<<"Check "<<mcnentr<<" "<<mcid[0]<<endl;
  for(Int_t ifs=0;ifs<mcnentr;ifs++) EventState.push_back(mcid[ifs]);
  //put in order for comparison with requested fFinalState
  std::sort(EventState.begin(),EventState.end());
  //now compare with the particles defined in fFinalState 
  if(!(std::equal(EventState.begin(),EventState.end(),fFinalState.begin()))){

    cout<<"Warning MC trueht particle do not exactly match those requested for final state"<<endl;
    cout<<"Momentum diff histograms may not be correct!!"<<endl;
    for(Int_t ifs=0;ifs<mcnentr;ifs++) cout<<" ID = "<<mcid[ifs]<<endl;
    }
  if(fMCParticle.size()!=(UInt_t)mcnentr){
    for(UInt_t ifs=0;ifs<fNdet;ifs++) {//create a lorentz vector for each detected particle 
      fMCParticle.push_back(TLorentzVector(0,0,1,2));
    }
  }
  fMCOK=1;
  return kTRUE; // return if IDs are not those 
}
void MCCLAStoHS::GetEventMCBranches(Int_t evi){
  b_mcnentr->GetEvent(evi);
  b_mcp->GetEvent(evi);
  b_mcm->GetEvent(evi);
  b_mctheta->GetEvent(evi);
  b_mcphi->GetEvent(evi);
  b_mcid->GetEvent(evi);
}
void MCCLAStoHS::MCCorrections(){
  for(UInt_t i=0;i<fNtype.size();i++){
    fp4rec=fDetParticle[i]->P4p();
    fp4true=&(fMCParticle[i]);
    //Int_t  kinBin=GetKinBin(fp4rec->Theta(),fp4rec->Rho(),fDetParticle[i]->Vertex().Z());
    Int_t  kinBin=GetKinBin(fp4rec->Theta(),fp4rec->Rho());
    FillHistograms(fTypeNames[i].second,kinBin);
  }
}
void MCCLAStoHS::HistogramList(TString sLabel){
  TDirectory::AddDirectory(kFALSE); //do not add to current directory
  //now define all histograms and add to Output
  //label includes kinematic bin and additional cut name
  // e.g fOutput->Add(MapHist(new TH1F("Mp1"+sLabel,"M_{p1}"+sLabel,100,0,2)));
  fOutput->Add(MapHist(new TH1F("Pdiff"+sLabel,"P_{diff}"+sLabel,100,-0.05,0.05)));
  //end of histogram list
  TDirectory::AddDirectory(kTRUE); //back to normal
}
void MCCLAStoHS::FillHistograms(TString sCut,Int_t bin){
  fCurrCut=sCut;
  fCurrBin=bin;
  //Get histogram from list
  //Fill histogram
  FindHist("Pdiff")->Fill(fp4true->Rho()-fp4rec->Rho());
  
}

Double_t  MCCLAStoHS::Correct_MCTrackMomentum(Int_t pdg,TLorentzVector p4){
  Double_t Pmom=p4.Rho();
  return (fMCCorr[pdg]->GetBinContent(fMCCorr[pdg]->FindFixBin(p4.Theta(),Pmom)))+Pmom;
}

Double_t MCCLAStoHS::AddATBranches(Bool_t wipe){

  if(wipe) fOutTree->SetBranchStatus("*",0); //don't fill other branches

  fNAT=fNdet;
  if(fMissPDG)fNAT++; //additional missing particle to be written
  fATE=new Double_t[fNAT];
  fATPx=new Double_t[fNAT];
  fATPz=new Double_t[fNAT];
  fATPy=new Double_t[fNAT];
  fATPdg=new Double_t[fNAT];
  fOutTree->Branch("Ef",fATE,Form("Ef[%d]/D",fNAT));
  fOutTree->SetBranchStatus("Ef",1); 
  fOutTree->Branch("px",fATPx,Form("px[%d]/D",fNAT));
  fOutTree->SetBranchStatus("px",1); 
  fOutTree->Branch("py",fATPy,Form("py[%d]/D",fNAT));
  fOutTree->SetBranchStatus("py",1); 
  fOutTree->Branch("pz",fATPz,Form("pz[%d]/D",fNAT));
  fOutTree->SetBranchStatus("pz",1); 
  fOutTree->Branch("particle_id",fATPdg,Form("particle_id[%d]/D",fNAT));
  fOutTree->SetBranchStatus("particle_id",1); 
  fOutTree->Branch("Ein_beam",&fATEb,"Ein_beam/D");
  fOutTree->SetBranchStatus("Ein_beam",1); 

}
Double_t MCCLAStoHS::AddATtrBranches(){
  //Add MC truth vectors

  fATtrE=new Double_t[fNAT];
  fATtrPx=new Double_t[fNAT];
  fATtrPz=new Double_t[fNAT];
  fATtrPy=new Double_t[fNAT];
  fATtrPdg=new Double_t[fNAT];
  fOutTree->Branch("tr_Ef",fATtrE,Form("Ef[%d]/D",fNAT));
  fOutTree->SetBranchStatus("tr_Ef",1); 
  fOutTree->Branch("tr_px",fATtrPx,Form("px[%d]/D",fNAT));
  fOutTree->SetBranchStatus("tr_px",1); 
  fOutTree->Branch("tr_py",fATtrPy,Form("py[%d]/D",fNAT));
  fOutTree->SetBranchStatus("tr_py",1); 
  fOutTree->Branch("tr_pz",fATtrPz,Form("pz[%d]/D",fNAT));
  fOutTree->SetBranchStatus("tr_pz",1); 
  fOutTree->Branch("tr_particle_id",fATtrPdg,Form("particle_id[%d]/D",fNAT));
  fOutTree->SetBranchStatus("tr_particle_id",1); 
  fOutTree->Branch("tr_Ein_beam",&fATtrEb,"tr_Ein_beam/D");
  fOutTree->SetBranchStatus("tr_Ein_beam",1); 
}
Double_t MCCLAStoHS::doAT(){
  for(Int_t in=0;in<fNAT-1;in++){
    fATE[in]=fDetParticle[in]->P4p()->E();   
    fATPx[in]=fDetParticle[in]->P4p()->Px();   
    fATPy[in]=fDetParticle[in]->P4p()->Py();   
    fATPz[in]=fDetParticle[in]->P4p()->Pz();   
    fATPdg[in]=fDetParticle[in]->PDG();   
  }
  //if missing particle
  if(fMissPDG){
    fATE[fNAT-1]=fMissing->E();   
    fATPx[fNAT-1]=fMissing->Px();   
    fATPy[fNAT-1]=fMissing->Py();   
    fATPz[fNAT-1]=fMissing->Pz();   
    fATPdg[fNAT-1]=fMissPDG;   
  }
  fATEb=fHSgamma->P4p()->E();
}

Double_t MCCLAStoHS::doATtr(){
  TLorentzVector vec4;
  TVector3 vec3;
  Double_t dtr=TMath::DegToRad();
  if(fNAT!=mcnentr&&fEntry<20){cout<<"warning mctruth particles are not equal to the AMpTools output "<<fNAT<<" "<<mcnentr<<endl;}
  for(Int_t in=0;in<mcnentr;in++){
    vec3.SetMagThetaPhi(mcp[in],mctheta[in]*dtr,mcphi[in]*dtr);
    vec4.SetVectM(vec3,mcm[in]);

    fATtrE[in]=vec4.E();   
    fATtrPx[in]=vec4.Px();   
    fATtrPy[in]=vec4.Py();   
    fATtrPz[in]=vec4.Pz();   
    fATtrPdg[in]=mcid[in];   
  }
 
  fATtrEb=tag_energy[0];//I think this is just the correct energy!
  
}
