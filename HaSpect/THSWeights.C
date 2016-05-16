#include "THSWeights.h"
#include <TTreeIndex.h>
#include <TSystem.h>
#include <TFile.h>
#include <TMath.h>
ClassImp(THSWeights);


THSWeights::THSWeights(TString name) :TNamed(name,name),fWTree(0),fWeightList(0),fFile(0),fIDi(0),fIDv(0){
  fWTree=new TTree(name+"_W","Tree weights for each species");
  fWTree->SetDirectory(0);
  fWTree->SetCacheSize(1E9);
  fIDTree=new TTree(name+"_ID","event ids for each entry");
  fIDTree->Branch("WID",&fID,"WID/L");
  fCurrEntry=0;
  fIsSorted=kFALSE;
  fN=0;
}

THSWeights::~THSWeights(){
  if(fWeightList) delete fWeightList;
  //if(fWTree) delete fWTree;
  //if(fIDTree) delete fIDTree;
  if(fFile) {delete fFile;}
}
void THSWeights::SetSpecies(TString name){
  UInt_t NSpecies=fSpecies.size(); 
  fSpecies.insert(pair<TString,Int_t>(name,NSpecies)); //save name in map
  fWVals.ResizeTo(NSpecies+1); //create entry in value array
  //make branch with name and connect to new fWVals entry
  fWVals[NSpecies]=0;
  cout<<"Insert species "<<name <<" "<<NSpecies<<" "<<fWVals[NSpecies]<<endl;
  fWTree->Branch(name,&fWVals[NSpecies],name+TString("/D")); 
}
Bool_t THSWeights::GetEntryFast(Long64_t id){
  //move down the tree until find event with correct ID
  //this requires the trees are sorted in ID
  //and fWTree is a subset of whatever tree requires the weight
  //Fastest if both trees have same events
  if(!fIDv) BuildIndex();
  if(id!=fIDv[fCurrEntry]){//no weight for this id
    return fGotEntry=kFALSE;}
  fWTree->GetEntry(fCurrEntry++); //get entry now we know it exists 
  return fGotEntry=kTRUE;
}
Bool_t THSWeights::GetEntrySlow(Long64_t id){
  //for unsorted tree
  //move down the tree until find event with correct ID
  //this is faster if  trees are sorted in ID and ReStart not required
  //in which case use GetEntryFast
  //and fWTree is a subset of whatever tree requires the weight
  //Fastest if both trees have same events
  if(!fIDv) BuildIndex();
  Bool_t ReStart=0;
  if(id!=fIDv[fCurrEntry++]){//no weight for this id
    //    fIDTree->GetEntry(fCurrEntry++);
    if(fCurrEntry==fWTree->GetEntries()&&ReStart==0){fCurrEntry=0; ReStart=1;}
    else if(fCurrEntry==fWTree->GetEntries()){
      cout<<"THSWeights::GetEntry entry not found "<<id<<endl; 
      return fGotEntry=kFALSE;}
  }
  fCurrEntry--;
  fWTree->GetEntry(fCurrEntry++); //get entry now we know it exists 
  return fGotEntry=kTRUE;
}
Bool_t THSWeights::GetEntryBinarySearch(Long64_t id){
  //use a binary search to find the entry for an unsorted tree
  if(!fIDv) BuildIndex();
  Long64_t entry=TMath::BinarySearch(fN,fIDv,id);
  if(fIDv[entry]!=id) return fGotEntry=kFALSE;
  fWTree->GetEntry(fIDi[entry]);//entry should be OK if these arrays are be ordered...
  return fGotEntry=kTRUE;
}
Long64_t THSWeights::Merge(TString tempName,TString outName,TString wmName){
  //THSWeights* WM=new THSWeights();
  //WM->Merge("dirname/prefix","outfile","WeightMap")
  if(outName!=TString("")) {
    //TFile* outFile=new TFile(outName,"recreate");
    SetFile(outName);
  }
  TString dirName=gSystem->DirName(tempName);
  TString prefix=gSystem->BaseName(tempName); //anything after directory in tempname
  if(prefix==TString("")) prefix="Weights";
  void *dir=gSystem->OpenDirectory(dirName);
  if(!dir) cout<<"THSWeights::Merge No directory found : "<<dirName<<endl;
  else cout<<"THSWeights::Merge Merging "<<prefix <<"* in directory "<<dirName<<endl;
  TString fileName;
  while( fileName=(gSystem->GetDirEntry(dir))){
    if(fileName==TString("")) break;
    if(fileName==TString("."))continue;
    if(fileName==TString(".."))continue;
    if(!fileName.Contains(prefix))continue;
    if(!fileName.Contains(".root"))continue;
    cout<<"THSWeights::Merge Adding file "<<fileName<<endl;
    TFile* wfile=new TFile(dirName+"/"+fileName);
    THSWeights* wm=(THSWeights*)wfile->Get(wmName);
    Add(wm);
    delete wm;
    wfile->Close();
    delete wfile;
  }
  gSystem->FreeDirectory(dir);
  
  SortWeights();//needs sorted for binary search to work
  PrintWeight();
  cout<<"THSWeights::Merge done "<<endl;
  // if(outName!=TString("")) {
  //   TFile* outFile=new TFile(outName,"recreate");
  //   Write();
  //   outFile->Close();
  //   delete outFile;
  // }
  return Size();
  
}
TString THSWeights::GetSpeciesName(UInt_t isp){
  if(isp>=fSpecies.size()) return TString("");
  //find the name for a given index
  for(StrIntMap_t::iterator it1 = fSpecies.begin(); it1 != fSpecies.end(); ++it1)
    if(it1->second==(Int_t)isp) return it1->first;

}
void THSWeights::Add(THSWeights* Wts){
  StrIntMap_t *sp0=GetSpeciesp();
  StrIntMap_t *sp1=Wts->GetSpeciesp();
  UInt_t Ns0=sp0->size();
  UInt_t Ns1=sp1->size();
  UInt_t NnewSp=0;
  StrIntMap_t New_sp; //map for additional species from Wts with original index
  cout<<"THSWeights Add "<<Wts->Size()<<" "<<Wts->GetSpecies().size()<<" "<<Wts->GetTree()->GetNbranches()<<" "<<GetTree()->GetNbranches()<<" "<<endl;
  if(fFile) cout<<"FILE "<<fFile->GetName()<<endl;
  //Check for new species
  for(StrIntMap_t::iterator it1 = sp1->begin(); it1 != sp1->end(); ++it1){
    cout<<"THSWeights Add new species :"<<it1->first<<endl;
    if(!(sp0->count(it1->first))){//this species is not in map already
      SetSpecies(it1->first);//add new species branch
      for(Long64_t ie=0;ie<fWTree->GetEntries();ie++)//fill previous entries with 0
	fWTree->GetBranch(it1->first)->Fill();
    }
  }
  //Add the tree
  //zero weight values in case of missing species in new weights
  //these branches will then just be filled with zero weight for
  //the new entries
  for(UInt_t isp=0;isp<Ns0;isp++)
    fWVals[isp]=0;
  //if(New_sp==0){//no new species, just merge trees
  TList* tlist=new TList();
  tlist->Add(Wts->GetTree());
  fWTree->Merge(tlist);
  delete tlist;
  TList* wlist=new TList();
  wlist->Add(Wts->GetIDTree());
  fIDTree->Merge(wlist);
  delete wlist;
  //make a list of weights added, this can be used to select contributing entrylists
  if(!fWeightList) {fWeightList=new TList();fWeightList->SetOwner();}
  fWeightList->Add(new TNamed(Wts->GetTitle(),""));//include name of this bin
  //fileName.ReplaceAll(".root","") ; //remove 
  // fileName.ReplaceAll("Weights","");//remove
  //fWeightList->Add(new TNamed(fileName,TString("")));//include name of this bin //remove

}
void THSWeights::PrintWeight(){
  cout<<"Map "<<GetName()<<" contains "<<Size() <<" events associated file is ";
  if(fFile) cout<<fFile->GetName()<< " "<<fFile->GetTitle()<<endl;
  else cout<<endl;
  cout<<"Species are : "<<endl;
  for(map<TString,Int_t>::iterator it=fSpecies.begin();it!=fSpecies.end();++it)
    cout<<it->first<<endl;
  Int_t Nit=0;
  cout<<"The first ten entries are :"<<endl;
  Int_t Nw=10;
  if(Size()<10) Nw=Size();
  for(Int_t i=0;i<Nw;i++){
  // for(Int_t i=3900;i<4100;i++){
    GetEntry(i);
    cout<<fID<<" "<<fWVals[0]<<" ";
    for(UInt_t iss=1;iss<fSpecies.size();iss++)
      cout<<fWVals[iss]<< " ";
    cout<<endl;
  }
  if(fWeightList){
    cout<<"These weights are combined from :"<<endl;
    for(Int_t i=0;i<fWeightList->GetEntries();i++)
      cout<<fWeightList->At(i)->GetName()<<endl;
  }
}

void THSWeights::BuildIndex(){
  cout<<"THSWeights::BuildIndex "<<fIDTree->BuildIndex("(Long64_t)WID")<<endl;
  TTreeIndex *index = (TTreeIndex*)fIDTree->GetTreeIndex();
  fIDi=index->GetIndex();//entry numbers
  fIDv=index->GetIndexValues();//id values
  fN=fWTree->GetEntries();
}
void THSWeights::SortWeights(){
  //GetEntryFast only works properly on trees where the ID is in order
  //This is not garunteed particualrly if weights are merged from different bins
  //reorder here
  BuildIndex();

  TTree* Mwtree=fWTree->CloneTree(); //create clone tree in memory
  Mwtree->SetDirectory(0);
  TTree* wtree=Mwtree->CloneTree(0); //create empty tree with branch adresses set
  TTree* idtree=fIDTree->CloneTree(0); //create empty tree with branch adresses set

  cout<<"THSWeights::SortWeights() reordering trees "<<fWTree->GetDirectory()<<endl; 
  for( Long64_t i =  0; i < fN ; i++ ) {
     fID=fIDv[i];
    idtree->Fill(); //fill as ordered by the build index
    //wtree is synched with id tree
    Mwtree->GetEntry(fIDi[i]);
    wtree->Fill(); //fill as ordered by the build index
  }
  cout<<"THSWeights::SortWeights() reordering trees"<<endl;
  //swap sorted trees to datamembers
  delete fIDTree;fIDTree=0;
  delete fWTree;fWTree=0;
  delete Mwtree;
  fIDTree=idtree;
  fWTree=wtree;
  fIDTree->SetDirectory(fFile);  
  fWTree->SetDirectory(fFile);  
  //resetbranch addresses
  fIDTree->SetBranchAddress("WID",&fID);
  for(UInt_t iss=0;iss<fSpecies.size();iss++)
  for(StrIntMap_t::iterator it1 = fSpecies.begin(); it1 != fSpecies.end(); ++it1)
    fWTree->SetBranchAddress(it1->first,&fWVals[it1->second]);
  
  cout<<"THSWeights::SortWeights Print new ordering"<<endl;
  PrintWeight();
  BuildIndex();//build for new tree!
  fIsSorted=kTRUE;
}
void THSWeights::SetFile(TString filename){
  TDirectory *saveDir=gDirectory;
  cout<<"THSWeights::SetFile "<<filename<<endl;
  fFile=new TFile(filename,"recreate");
  if(fIDTree)fIDTree->SetDirectory(fFile);
  if(fWTree)fWTree->SetDirectory(fFile);
  saveDir->cd();
}
void THSWeights::Save(){
  cout<<"void THSWeights::Save() "<<fFile<<endl;
  cout<<fIDTree<<" "<<fWTree<<endl;
  if(!fFile) {cout<<"THSWeights::Save() no file associated with "<<GetName()<<" so not saved"<<endl;return;}

  cout<<" THSWeights::Save() "<<fFile->GetName()<<" "<<fFile->GetTitle()<<endl;
  fWTree->SetDirectory(fFile);
  fFile->cd();
  Write();
  fFile->Close();
  delete fFile;fFile=0;
  cout<<"THSWeights::Save() Saved weights to file"<<endl;

}
void THSWeights::Mem(){ //Load tree into memory
  fWTree->SetDirectory(0);//take tree into memory
  fIDTree->SetDirectory(0);
  fCurrEntry=0;
}
void THSWeights::Disk(){ //Put trees back onto hard disk
  if(fFile){
    fIDTree->SetDirectory(fFile);
    fWTree->SetDirectory(fFile);
  }
  else cout<<"THSWeights::Disk() No file set to keep trees"<<endl;
}
void THSWeights::LoadSaved(TString fname,TString wname){
  TDirectory* savedir=gDirectory;
  TFile* wfile=new TFile(fname);
  THSWeights* file_wts=(THSWeights*)wfile->Get(wname);//read into memory
  savedir->cd();
  fWTree=(TTree*) file_wts->GetTree()->Clone();
  fWTree->SetDirectory(0);
  fSpecies=file_wts->GetSpecies();
  fWVals.ResizeTo(fSpecies.size());
  for(UInt_t i=0;i<fSpecies.size();i++)
    fWTree->SetBranchAddress(GetSpeciesName(i),&fWVals[i]); 

  fIDTree=(TTree*)file_wts->GetIDTree()->Clone();
  fIDTree->SetDirectory(0);
  fIDTree->SetBranchAddress("WID",&fID);

  fCurrEntry=0;
  fIsSorted=kFALSE;
  fN=fWTree->GetEntries();
  delete file_wts;  
  wfile->Close();
  delete wfile;
}
