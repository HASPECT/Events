#include "THSBins.h"
#include "TROOT.h"
#include "TVectorT.h"

ClassImp(THSBins);

THSBins::THSBins(const THSBins& other, const char* name): TNamed(name,name){
  fBinNames=other.fBinNames;
  fNbins=other.fNbins;
  fNaxis=other.fNaxis;
  fVarAxis=other.fVarAxis;
  fEntryLists=(TObjArray*)other.fEntryLists->Clone();
  //currently not ccopying treees TObjArray *fTrees;
  fTrees=0;
  fFile=0;

}
void THSBins::AddAxis(TString name,Int_t nbins,Double_t min,Double_t max){
  //Add a new axis for a given variable, name should be tree name
  TAxis axis(nbins,min,max);
  axis.SetName(name);
  fVarAxis.push_back(axis);
  fNaxis++;
}
void THSBins::InitialiseTrees(TString name,TString filename){
  if(fEntryLists) {cout<<"THSBins::InitialiseTrees already initialised entry lists"<<endl;return;}
  if(filename!=TString(""))fFile=new TFile(filename,"recreate");
  //create the trees for each unique bin
  fTrees=new TObjArray(fNaxis);
  fTrees->SetName(name);
  fTrees->SetOwner(kTRUE);
 
  //make all the bin names
  TString binName;
  fNbins=0;
  IterateAxis(0,binName);

  //now iterate over fBinsNames constructing entry lists
  // for(Int_t i=0;i<fNbins;i++){
  //   TTree* tr=new TTree(fBinNames[i],fBinNames[i]);
  //   fTrees->Add(tr);
  //   tr->SetDirectory(fFile);
  // }

  
  gROOT->cd(); //trees are in file directory, go back to ROOT

}
void THSBins::InitialiseLists(TString name,TString filename){
  //if(fTrees) {cout<<"THSBins::InitialiseTrees already initialised trees"<<endl;return;}
  if(filename!=TString(""))fFile=new TFile(filename,"recreate");
  //create the entry lists for each unique bin
  fEntryLists=new TObjArray(fNaxis);
  fEntryLists->SetName(name);
  fEntryLists->SetOwner(kTRUE);

  //make all the bin names
  TString binName;
  fNbins=0;
  IterateAxis(0,binName);

  //now iterate over fBinsNames constructing entry lists
  for(Int_t i=0;i<fNbins;i++){
    TEntryList* el=new TEntryList(fBinNames[i],fBinNames[i]);
    fEntryLists->Add(el);
    el->SetDirectory(fFile);
  }
  gROOT->cd(); //lists are in file directory, go back to ROOT


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

void THSBins::RunEntryList(TTree* tree){
  //create entry lists for tree
  TVectorD vVal(fNaxis);//values of variables for given entry
  tree->SetBranchStatus("*","0");//faster if turnoff unused branches
  for(Int_t j=0;j<fNaxis;j++){
    tree->SetBranchStatus(fVarAxis[j].GetName(),"1");//STATUS must be called before ADRESS!! see Important remarkse in TChain SetBranchStatus!
    tree->SetBranchAddress(fVarAxis[j].GetName(),&vVal[j]);
  }
  vector<Int_t> vBin(fNaxis); //store for the bin number of each axis
  for(Long64_t i=0;i<tree->GetEntries();i++){//loop over events
    tree->GetEntry(i);
    Bool_t InLimits=kTRUE;
    for(Int_t iA=0;iA<fNaxis;iA++)//first check var is within variable ranges
      if(vVal[iA]<fVarAxis[iA].GetXmin()||vVal[iA]>fVarAxis[iA].GetXmax()) InLimits=kFALSE;
    if(!InLimits) continue;
    //now find bin for each axis
    for(Int_t iA=0;iA<fNaxis;iA++){//loop over vars/axis
      //cout<<iA<<" val "<<vVal[iA]<<" "<<fVarAxis[iA].GetXmax()<<" "<<(vVal[iA]<fVarAxis[iA].GetXmin()||vVal[iA]>fVarAxis[iA].GetXmax())<<endl;
      for (int iB = 1; iB <= fVarAxis[iA].GetNbins(); iB++)//loop over bins
	if(vVal[iA]<fVarAxis[iA].GetBinUpEdge(iB)){vBin[iA]=iB;break;}//found it, next axis
    }
    //now have the bin for each axis, find the correct entry list
    Int_t theBin=-1;
    for(Int_t iA1=0;iA1<fNaxis-1;iA1++){
      //cout<<vBin[iA1]<<" ";
      Int_t tbin=vBin[iA1]-1;//-1 as bin indexing starts at 1 with 0 underflow
      for(Int_t iA2=iA1+1;iA2<fNaxis;iA2++)
	tbin*=fVarAxis[iA2].GetNbins();
      theBin+=tbin;
    }
    // cout<<vBin[fNaxis-1]<<" ";
    theBin+=vBin[fNaxis-1];
    //cout<<" Bin "<<theBin<<" "<<fEntryLists->GetEntries()<<endl;
    (dynamic_cast<TEntryList*>(fEntryLists->At(theBin)))->Enter(i,tree);
  }
  tree->SetBranchStatus("*",1);
  tree->ResetBranchAddresses();

}
void THSBins::RunTree(TTree* tree){
  //create entry lists for tree
  TVectorD vVal(fNaxis);//values of variables for given entry
  //tree->SetBranchStatus("*","0");//faster if turnoff unused branches, don't use here as fit variables are not included in VarAxis
  for(Int_t j=0;j<fNaxis;j++){
    //    tree->SetBranchStatus(fVarAxis[j].GetName(),"1");//STATUS must be called before ADRESS!! see Important remarkse in TChain SetBranchStatus!
    tree->SetBranchAddress(fVarAxis[j].GetName(),&vVal[j]);
  }
  //Copy the tree branches to the list of trees
  for(Int_t i=0;i<fNbins;i++){
    // fFile->cd();//clone tree is not working all the time when file is set??
    TTree *newTree=tree->CloneTree(0);
    //  newTree->SetDirectory(fFile);
    newTree->SetName(fBinNames[i]);
    fTrees->Add(newTree);
  }

  vector<Int_t> vBin(fNaxis); //store for the bin number of each axis
  for(Long64_t i=0;i<tree->GetEntries();i++){//loop over events
    tree->GetEntry(i);
    Bool_t InLimits=kTRUE;
    for(Int_t iA=0;iA<fNaxis;iA++)//first check var is within variable ranges
      if(vVal[iA]<fVarAxis[iA].GetXmin()||vVal[iA]>fVarAxis[iA].GetXmax()) InLimits=kFALSE;
    if(!InLimits) continue;
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
    (dynamic_cast<TTree*>(fTrees->At(theBin)))->Fill();
  }
  tree->SetBranchStatus("*",1);
  tree->ResetBranchAddresses();
  for(Int_t i=0;i<fTrees->GetEntries();i++)
    (dynamic_cast<TTree*>(fTrees->At(i)))->ResetBranchAddresses();
  gROOT->cd();
}
void THSBins::SaveLists(){
  if(!fFile) return;
  //cout<<"void THSBins::Save() "<<fFile<<endl;
  fFile->cd();
  // if(fTrees)fTrees->Write(fTrees->GetName(),TObject::kSingleKey);
  if(fEntryLists)fEntryLists->Write(fEntryLists->GetName(),TObject::kSingleKey);
  //cout<<"void THSBins::Save() "<<fFile<<" "<<fTrees<<" "<<fEntryLists<<endl;

  fFile->Close();
  //if(fTrees) delete fTrees;fTrees=0;
  //cout<<"void THSBins::Save() "<<fFile<<" "<<fTrees<<" "<<fEntryLists<<endl;
  //  if(fEntryLists)delete fEntryLists; fEntryLists=0;
  //cout<<"void THSBins::Save() "<<fFile<<" "<<fTrees<<" "<<fEntryLists<<endl;
  if(fFile) delete fFile;
  // if(fTrees) delete fTrees;
  // if(fEntryLists) delete fEntryLists;
  // cout<<"void THSBins::Save() "<<fFile<<" "<<fTrees<<" "<<fEntryLists<<endl;
}
void THSBins::SaveTrees(){
  if(!fFile) return;
  fFile->cd();
  if(fTrees)fTrees->Write(fTrees->GetName(),TObject::kSingleKey);
  //if(fTrees)fTrees->Write(fTrees->GetName());
  fFile->Close();
  if(fFile) delete fFile;
  if(fTrees) delete fTrees;

}
