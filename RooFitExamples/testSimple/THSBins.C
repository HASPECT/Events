#include "Riostream.h"
#include "THSBins.h"
#include "TROOT.h"
#include "TDirectory.h"
#include "TVectorT.h"


ClassImp(THSBins)	

THSBins::THSBins(TString name) :TNamed(name,name),fBinTree(0),fFile(0),fNaxis(0),fNbins(0){
}

THSBins::THSBins(TString name,TString filename):TNamed(name,name),fBinTree(0),fFile(0),fNaxis(0),fNbins(0){
  TDirectory *saveDir=gDirectory;
  fFile=new TFile(filename);
  if(!fFile->IsOpen()) {cout<<"THSBins::THSBins(TString name,TString filename) File does not exist "<< filename<<endl;}
  THSBins* filebins=(THSBins*)fFile->Get(name);
  fBinNames=filebins->GetBinNames();
  fNbins=filebins->GetN();
  fNaxis=filebins->GetNAxis();
  fVarAxis=filebins->GetVarAxis();
  //tree is not written to file as data member
  fBinTree=(TTree*)fFile->Get("HSBinTree");
  if(!fBinTree) cout<<"THSBins::THSBins(TString name,TString filename)  Tree does not exist "<<"HSBinTree"<<endl;
  saveDir->cd();
  cout<<"DONE THSBins::THSBin"<<endl;
}
THSBins::THSBins(const THSBins& other, const char* name): TNamed(name,name),fBinTree(0),fFile(0),fNaxis(0),fNbins(0){
  fBinNames=other.fBinNames;
  fNbins=other.fNbins;
  fNaxis=other.fNaxis;
  fVarAxis=other.fVarAxis;
  if(other.fBinTree) fBinTree=(TTree*)other.fBinTree->Clone();
  fFile=0;

}
THSBins::~THSBins(){
  if(fBinTree) delete fBinTree;
  if(fFile){fFile->Close(); delete fFile;}
}
void THSBins::AddAxis(TString name,Int_t nbins,Double_t min,Double_t max){
  //Add a new axis for a given variable, name should be tree name
  TAxis axis(nbins,min,max);
  axis.SetName(name);
  fVarAxis.push_back(axis);
  fNaxis++;
}
void THSBins::AddAxis(TString name,Int_t nbins,Double_t* xbins){
  //Add a new axis for a given variable, name should be tree name
  TAxis axis(nbins,xbins);
  axis.SetName(name);
  fVarAxis.push_back(axis);
  fNaxis++;
}

void THSBins::InitialiseBinTree(TString name,TString filename){
  SetName(name);
  //if(fTrees) {cout<<"THSBins::InitialiseTrees already initialised trees"<<endl;return;}
  TDirectory *saveDir=gDirectory;
  if(filename!=TString(""))fFile=new TFile(filename,"recreate");
  fBinTree=new TTree("HSBinTree","Contains bin number for each event");
  fBinTree->Branch("Bin",&fBin,"Bin/I");
  TString binName;
  fNbins=0;
  fBinNames.clear();
  IterateAxis(0,binName);
  saveDir->cd(); //lists are in file directory, go back to ROOT


}
void THSBins::IterateAxis(Int_t iA,TString binName) {
  //iterate through all bins possible with given axis and constuct
  //unique names for them
  if (iA >= fNaxis){ //stop clause
    fBinNames.push_back(binName);
    fNbins++;
    cout<<binName<<endl;
    return;
  }
  for (int iB = 1; iB <= fVarAxis[iA].GetNbins(); iB++) { 
    fVarAxis[iA].SetBinLabel(iB,Form("%1.2f_",fVarAxis[iA].GetBinCenter(iB)));
    IterateAxis(iA+1,binName+fVarAxis[iA].GetName()+fVarAxis[iA].GetBinLabel(iB));
   }
}

void THSBins::RunBinTree(TTree* tree){
  cout<<"THSBins::RunBinTree "<<tree->GetEntries()<<endl;
  //create entry lists for tree
  TDirectory *saveDir=gDirectory;
  fFile->cd();
  TVectorD vVal(fNaxis);//values of variables for given entry
  tree->SetBranchStatus("*","0");//faster if turnoff unused branches
  for(Int_t j=0;j<fNaxis;j++){
    tree->SetBranchStatus(fVarAxis[j].GetName(),"1");//STATUS must be called before ADDRESS!! see Important remarkse in TChain SetBranchStatus!
    tree->SetBranchAddress(fVarAxis[j].GetName(),&vVal[j]);
  }
  vector<Int_t> vBin(fNaxis); //store for the bin number of each axis
  for(Long64_t i=0;i<tree->GetEntries();i++){//loop over events
    tree->GetEntry(i);
    Bool_t InLimits=kTRUE;
    for(Int_t iA=0;iA<fNaxis;iA++)//first check var is within variable ranges
      if(vVal[iA]<fVarAxis[iA].GetXmin()||vVal[iA]>fVarAxis[iA].GetXmax()) InLimits=kFALSE;
    if(!InLimits) {fBin=fNbins;fBinTree->Fill();continue;}
    //now find bin for each axis
    for(Int_t iA=0;iA<fNaxis;iA++){//loop over vars/axis
      for (int iB = 1; iB <= fVarAxis[iA].GetNbins(); iB++)//loop over bins
	if(vVal[iA]<fVarAxis[iA].GetBinUpEdge(iB)){vBin[iA]=iB;break;}//found it, next axis
    }
    //now have the bin for each axis, find the correct entry list
    Int_t theBin=-1;
    for(Int_t iA1=0;iA1<fNaxis-1;iA1++){
      Int_t tbin=vBin[iA1]-1;//-1 as bin indexing starts at 1 with 0 underflow
      for(Int_t iA2=iA1+1;iA2<fNaxis;iA2++)
	tbin*=fVarAxis[iA2].GetNbins();
      theBin+=tbin;
    }
    theBin+=vBin[fNaxis-1];
    fBin=theBin;
    fBinTree->Fill();
  }
  cout<<"Done bin tree "<<endl;
  tree->SetBranchStatus("*",1);
  tree->ResetBranchAddresses();
  saveDir->cd();

}

TTree* THSBins::GetBinnedTree(TTree* tree,Int_t bin){
  //This class will not delete the tree
  // gROOT->cd();
  cout<<"Start THSBins::GetBinned"<<endl;
  TDirectory *saveDir=gDirectory;
  tree->AddFriend(fBinTree,"bt");
  tree->SetBranchStatus("bt.Bin",1);
  tree->GetDirectory()->cd();//or else complains memeiry resident...
  TTree* tree_copy=tree->CopyTree(Form("bt.Bin==%d",bin));
  tree_copy->SetName(fBinNames[bin]);
  //tree_copy->SetTitle(fBinNames[bin]);
  tree->RemoveFriend(fBinTree);
  saveDir->cd();
  return tree_copy;
 }
void THSBins::Save(){
  cout<<" THSBins::Save() Saving "<<GetName()<<endl;
  if(!fFile) return;
  fFile->cd();
  if(fBinTree){ fBinTree->Write();delete fBinTree;fBinTree=0;}
  Write();
  fFile->Close();
  if(fFile) delete fFile;
  
}
void THSBins::PrintAxis(){

  for(Int_t iA=0;iA<fNaxis;iA++)
    cout<<fVarAxis[iA].GetName()<<" "<<fVarAxis[iA].GetNbins()<<" "<<fVarAxis[iA].GetXmin()<<" "<<fVarAxis[iA].GetXmax()<<" "<<endl;
}

// TEntryList* THSBins::MergedLists(TList* list){

//   TList* mlist=new TList(); 
//   TEntryList* mel=0;
//   for(Int_t i=0;i<list->GetEntries();i++){
//     if(mel=dynamic_cast<TEntryList*>(fEntryLists->FindObject(list->At(i)->GetName())))
//       mlist->Add(mel);

//   }
//   TEntryList *subList=new TEntryList();
//   subList->Merge(mlist);
//   delete mlist;
//   return subList; 

// }
