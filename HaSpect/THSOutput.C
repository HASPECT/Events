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

#include <TProofOutputFile.h>
#include <TDatabasePDG.h>
#include <TSystem.h>
#include <TEntryList.h>
#include <TObjArray.h>
#include <TROOT.h>
#include <TH3.h>
#include <TKey.h>
#include <TProof.h>
#include <TTreeIndex.h>
#include <iostream>
#include <algorithm>
#include "THSOutput.h"

using namespace std;

THSOutput::~THSOutput(){
  if(fOutTree) SafeDelete(fOutTree);
  if(fFile){
    SafeDelete(fFile);
    //SafeDelete(fProofFile);
  }
  if(fParentFile) {fParentFile->Close(); SafeDelete(fParentFile);fParentTree=0;}
}
void THSOutput::HSBegin(TList* input,TList* output){
  fOutName = gSystem->Getenv("HSOUT"); //outfile name must be set as an enviroment variable
  PrepareOutDir();  //Make directory for output root files
  fSelInput=input; //connect to the selector input list
  fSelOutput=output; //connect to the selector output list
  //Copy all the source code macros
  fCodeList=new TList();
  fCodeList->SetOwner();
  fCodeList->SetName("CodeList");
  ImportSysDirtoList(gSystem->pwd(),fCodeList);//the directory the user analysis code is in
  ImportSysDirtoList(gSystem->Getenv("HSANA"),fCodeList); //the directory the HasPEct core code is
  if(gProof)fSelInput->Add(fCodeList);//add to input so can give to Slaves, if fInput doesn't exit then not running on PROOF and this fCodeList is OK
  
}
void  THSOutput::HSSlaveBegin(TList* input,TList* output){
  //need to reconnect with selector in case running on proof
   fOutName = gSystem->Getenv("HSOUT"); //outfile name must be set as an enviroment variable
 if(!fSelOutput){
    fSelInput=input; //connect to the selector input list
    fSelOutput=output; //connect to the selector output list
  }
  //Get the source code for writing to output file
  if(!fCodeList){ //running on PROOF
    fCodeList=dynamic_cast<TList*>(fSelInput->FindObject("CodeList"));
    if(!fCodeList)Error(" THSOutput::HSSlaveBegin"," no code list found cannot save source to file");
  }
  //The HS output requires the definition of particles in the final state
  //These should be entered into fStrParticles in the selector Begin()
  if(fStrParticles.Length()>0) InitOutput();
  
  //Make the entry list, this records the events used from the input chain
  fEntryList=new TEntryList("HSelist","Entry List");
  fSelOutput->Add(fEntryList);

  //Start the ID from the offset if
  fgID=fgIDoff;	
};
void THSOutput::HSNotify(TTree* tree){
  //Function that looks after the output file
  //Needs to be called in TSelector::Notify()
  fCurTree=tree;
  if(fEntryList)fEntryList->SetTree(fCurTree);//This seems to be required in the case of running tree->Process(TSelector) but not tree->Process(Filename), it is not clear why this should be different but this line does not seem to cause problems for the latter case...
  //Check if gID exists in current tree, if not will start saving it now
  if(!fCurTree->GetBranch("fgID")) fSaveID=kTRUE;
  if(fOutName.EndsWith(".root")){
    //Only want to do some things once when writing 1 file only
     if(fStepName.Length()==0){
       InitOutFile(fCurTree); //initialise output file
    //   CopyCode(fFile,fCurTree->GetCurrentFile()); //copy surce code from input tree to output
       return;
     }
    else return; //only saving one combined file
   }
 //Case  making many output files
  FinishOutput(); //close the last file
  InitOutFile(fCurTree);   //start the new file
  return;

}
void THSOutput::HSProcessStart(Long64_t entry){
  fEntry=entry;
}
void THSOutput::HSProcessFill(){
  //if(fSaveID) fgID=fEntry;
  if(fSaveID) fgID++;
  else if(fOutTree) fCurTree->GetBranch("fgID")->GetEntry(fEntry); //make sure get ID branch to write to new file
  if(fEntryList)fEntryList->Enter(fEntry);
  if(fOutTree) fOutTree->Fill();
}
void THSOutput::HSSlaveTerminate(){
  FinishOutput();
}
void THSOutput::HSTerminate(){
  //This function is a bit messy, mainly due to allowing different configurations
  //of analysis to use the same code, e.g. proof or not, many files or one
  if(fOutName.EndsWith(".root")){//Just one file
    //Copy the source code  and event list once
    // //to stop PROOF merge making multiple copies of source code. 
    TEntryList* elist=dynamic_cast<TEntryList*>(fSelOutput->FindObject("HSelist")); 	//must use the eventlist merged in output list
    TEntryList* eltemp=0;
    TString InDirName;
    TString InFileName;
    if(elist->GetLists()){
      //just going to save code from first file in list
      InDirName=gSystem->DirName(((TEntryList*)elist->GetLists()->At(0))->GetFileName(	));//assume all files in same directory as first
      InFileName=((TEntryList*)elist->GetLists()->At(0))->GetFileName();
    }	
    else{//only one file in input chain
      InFileName=elist->GetFileName();
    }
    TDirectory* savedir=gDirectory;
    TFile* elfile=TFile::Open(fOutName,"update"); //reopen output file
    
    //Copy original input code to output file
    TFile* infile=new TFile(InFileName);
    if(!fStepDir){
      infile=new TFile(InFileName);
      CopyCode(elfile,infile);
    }
    elfile->cd();
    WriteListtoFile(fStepDir);
    infile->Close();
    delete infile;
    cout<<"Written code to "<<fStepName<<endl;//fStepName set in CopyCode
    //Write the entrylist to the step directory
    elfile->cd(fStepName); //Write in directory with source code
    elist->Write(0,TObject::kOverwrite);
     //If kinematic bins to be saved save them
    //note this is currently only implemented for one .root ouput file
    //the entry lists themselves know which tree from the chain to use
    //Note again the entry list must be saved after the PROOF merge
    //This makes sure all treenames etc. are correct
    if(elfile) elfile->cd();
    TDirectory *curDir = gDirectory->mkdir("HSKinBinEntries");
    curDir->cd();
    TIter next(fSelOutput);
    TKey* key=0;
    Long64_t Nkbevs=0;
    while ((key = (TKey*)next()))if(TString(key->GetName()).Contains("HSBin")){//find the HSBin  list
	fSelOutput->FindObject(fStepName=key->GetName())->Write();
	Nkbevs+=((TEntryList*)fSelOutput->FindObject(fStepName=key->GetName()))->GetN();
      }
    cout<<"Saved "<< Nkbevs<<" to kin bin entry lists"<<endl;
    //saved kinematic bin entries
    if(elfile) {
      elfile->Close();
      savedir->cd();
      delete elfile;	
    }
  }
  else{
    //Writing the event lists in each output file
    // as well as the overall one on ParentEventList.root
    TIter next(fSelOutput);
    TProofOutputFile* elpofile=0;
    TObject* outo=0;
    TEntryList* elist=dynamic_cast<TEntryList*>(fSelOutput->FindObject("HSelist")); //must use the eventlist merged in output list
    //now iterate over output file and write a copy of their event list
    //the event list for each tree is retireved from el
    TFile* infile=0; //pointer to input file
    while((outo=dynamic_cast<TObject*>(next()))){
      if((elpofile=dynamic_cast<TProofOutputFile*>(outo))){
	TFile* elfile = elpofile->OpenFile("UPDATE");
	cout<<elfile->GetName()<<endl;
	//First sort tree to regain original ordering
	if(gProof){
	  TIter fnext(elfile->GetListOfKeys());
	  TKey* fkey=0;
	  //Look for a trees in saved file
	  while ((fkey = (TKey*)fnext())){
	    if(TString(fkey->GetClassName())==TString("TTree")){//find the HSStep list
	      SortTree(dynamic_cast<TTree*>(elfile->Get(fkey->GetName())));
	    }
	  }
	}//done reordering if proof
	//now look for source code directory 
	//now just get the sub event list corresponding to this file
	//note proof file contains just the base name without directory
	//we need the directory and name of the input trees
	TEntryList* eltemp=0;
	TString InDirName;
	TString InFileName;
	if(elist->GetLists()){

	  InDirName=gSystem->DirName(((TEntryList*)elist->GetLists()->At(0))->GetFileName());//assume all files in same directory as first
	  InFileName=InDirName+"/"+elpofile->GetName();
	  eltemp=elist->GetEntryList(elist->GetTreeName(),InFileName);
	}	
	else{//only one file in input chain
	  eltemp=elist; 
	  InFileName=elist->GetFileName();
	}
	//Copy original input code(and previous entry lists) to output file
	infile=new TFile(InFileName);
	CopyCode(elfile,infile);
	elfile->cd();
	WriteListtoFile(fStepDir);
	cout<<"Written code to "<<fStepName<<endl;
	//Write the new entrylist to the step directory
	eltemp->SetName(elist->GetName());
	elfile->cd(fStepName); //Write in directory with source code
	eltemp->Write(0,TObject::kOverwrite);
	elfile->Close();
	delete elfile;
      }
    }
    cout<<"Infile "<<infile<<endl;
    if(infile) {
      infile->Close();
      delete infile;
    }
    //Save the overall event list in a new file in output directory
    TFile* allel=new TFile(fOutName+"/ParentEventList.root","recreate");
    elist->Write();
    allel->Close();
    delete allel;
  }
}
void THSOutput::InitOutput(){
  //used the predefined string in fStrParticles to setup output tree branches
 //Make output Tree   
   if(!fOutTree) fOutTree=new TTree("HSParticles","A tree containing reconstructed particles");

  Int_t buff=64000;
  Int_t split=0;//note split is important in the TSelector framework, if increased branches in subsequent selectors will be data members of the THSParticle object rather than the whole object (this can lead to name conflicts)
   
   //now automatically create requested detected particles
   //Get the final state defined in fStrParticles
   fNdet=CountChar(fStrParticles,':')+1; //number of particles is just "number of :" +1
   // fDetParticle=new THSParticle*[fNdet];//array to hold the detected particles
   Int_t pos=0;//marker for position in particle list string
   for(UInt_t ifs=0;ifs<fNdet;ifs++){//loop over fStrParticles string and seperate particles
     TString pName;
     if(fStrParticles.Index(":",pos+1)>0)pName=fStrParticles(pos,fStrParticles.Index(":",pos+1)-pos);//split the fStrParticles string
     else pName=fStrParticles(pos,fStrParticles.Length()-pos);//no more ':' so last particle
     pos=fStrParticles.Index(":",pos+1)+1;//move pos on to the next particle
     fDetID.push_back(TDatabasePDG::Instance()->GetParticle(pName)->PdgCode());//get PDG code for particle
     fFinalState.push_back(fDetID[ifs]); //save ID in fFinalState to compare each event with
   }
   //order the final state vector, allows easy comparison with event vector, this is ascending order
   //!!!!!!!!!!!!!!!!!!!!!!very important step for event algorithm!!!!!!!!!!
   std::sort(fFinalState.begin(),fFinalState.end());
   //sort the number of different particle types into fNtype
   Int_t vipart=0;//counter to move to next particle type in fFinalState
   Int_t nvipart=0;//counter of number of particle types
   while((nvipart=std::count(fFinalState.begin(),fFinalState.end(),fFinalState[vipart]))){fNtype.push_back(nvipart);vipart+=nvipart;}
  //now make the branches and particles
   UInt_t Nbranches=0;
   vipart=0;
   for(UInt_t itype=0;itype<fNtype.size();itype++){//loop over types of particles
     fIDtype.push_back(fFinalState[Nbranches]);//get the pdg id of this type
     for(UInt_t intype=0;intype<fNtype[itype];intype++){//loop over particles of same type
       //create particles, note the vector will look after deleting these when it goes out of scope
       fDetParticle.push_back(new THSParticle(fFinalState[Nbranches]));
       TString pName=TDatabasePDG::Instance()->GetParticle(fFinalState[Nbranches])->GetName();
       pName.ReplaceAll("+","p");  //Get rid of + and - in names so not to confuse tree->Draw
       pName.ReplaceAll("-","m");
       pName+=TString("_")+=intype;//append the number of instance of this particle to PDG name
       fOutTree->Branch(pName,fDetParticle[Nbranches],buff,split); //make the branch
       cout<<"Made branch "<<pName<<" the particle can be accessed in the code with fDetParticle["<<Nbranches<<"]"<<endl;
       Nbranches++;
     }//end intype loop
   }//end intype loop
}
void THSOutput::FinishOutput(){
   // Write the ntuple to the file
  if (fFile) {
    Bool_t cleanup = kFALSE;
    TDirectory *savedir = gDirectory;
    fFile->cd();
    if (!fOutTree) {//no output tree 
      fSelOutput->Add(fProofFile); //give this proof file to output for merging in case there are histograms
      fProofFile->Print();
     }
    else  if (fOutTree->GetEntries() > 0) {//if tree has entries save it
      fOutTree->Write(0, TObject::kOverwrite);  //write ouput tree
      fOutTree->Reset();  //remove saved entries
      fSelOutput->Add(fProofFile); //give this proof file to output for merging
      fProofFile->Print();
      fOutTree->SetDirectory(0);
    } 
    else {//an empty tree so don't save it
      cleanup = kTRUE;
      fOutTree->SetDirectory(0);
    }
   //Loop over output and look for histograms to save
    TIter next(fSelOutput);
    TObject* ho=0;
    TH1* hf=0;
    while((ho=next())){
      if((hf=dynamic_cast<TH1*>(ho))){
	hf->Write();
	hf->Reset(""); //remove saved histogram entries for next file
      }
   
    }
    gDirectory = savedir;
    fFile->Close(); //Close this file
    // Cleanup, if needed
    if (cleanup) {
      TUrl uf(*(fFile->GetEndpointUrl()));
      SafeDelete(fFile);
      gSystem->Unlink(uf.GetFile());
      SafeDelete(fProofFile);
      //	SafeDelete(fOutTree);
    }
  }
}
void THSOutput::InitParent(TTree* ctree,TString step){
  //Function to link a parent selector to this one.
  //It uses the event lists which are written a HSStep directory
  //of the file being analysed to get the parent tree.
  //The parent selector should be made a data memeber of this selector
  //and the fParent.Init(fParentTree) should be called after this 
  //function in the current selector Init()
  TDirectory* savedir=gDirectory;
  cout<<step<<endl;
  // exit(1);
  //get the entry list of the parent from te file of the current tree
  TFile* infile=ctree->GetCurrentFile();
  if(fParEntryLists)cout<<infile->GetName()<<" "<<fParEntryLists->GetEntries()<<endl;
  //Get the chain of entry lists leading to the selected parent list
  //This allows determination of the parent entry number if several
  //sequences of filtering have been performed
  TString tempstep=step; //"e.g. Step_2/Step_1/Step_0
  TEntryList* oldel=0;
  tempstep+="/temp";
  if(!fParEntryLists) {fParEntryLists=new TList();}
  else fParEntryLists->Clear();
  // if(!fParEntryLists) fParEntryLists=new TList();
  // else {SafeDelete( fParEntryLists); fParEntryLists=new TList();}
  while ((tempstep=gSystem->DirName(tempstep))){//loop through step directories
   if(tempstep==".") break;
   cout<<tempstep<<endl;
   TEntryList* el=(TEntryList*)infile->Get(tempstep+"/HSelist");
   cout<<el->GetN()<<endl;
   if(oldel) if(oldel->GetN()==el->GetN()) continue;//no need to add as contains the same entries
   fParEntryLists->Add(el);//Add to list of entry lists
   cout<<"Added list "<<fParEntryLists->GetEntries()<<endl;
   oldel=el;
 }

   // if(!el){ Error("THSOuput::InitParent","No valid event list found");return;}
  if(fParEntryLists->GetEntries()==0){ Error("THSOuput::InitParent","No valid event list found");return;}
  //delete previous tree file
  if(fParentFile) {fParentFile->Close(); SafeDelete(fParentFile);fParentTree=0;}
  //get the new parent file and tree from the entrylist
  //Get the actual parent event list
  TEntryList* el=(TEntryList*)infile->Get(step+"/HSelist");
  cout<<"PArent file name "<<el->GetFileName()<<" "<<el->GetN()<<" "<<el->GetLists()<<endl;
  fParentFile=new TFile(el->GetFileName());
  fParentTree=(TTree*)fParentFile->Get(el->GetTreeName());
  // fParentTree->SetEntryList(el); //not required as get entry in this code
  savedir->cd();
}
Long64_t THSOutput::GetParentEntry(Long64_t parentry){
  //Moves through the list of parent entry lists to find the correct 
  //parent entry number for this "entry" of the child 
  TEntryList* iel=0;     
  //Long64_t parentry;      
  for(Int_t i=fParEntryLists->GetEntries()-1;i>-1;i--){
   iel=(TEntryList*)fParEntryLists->At(i);
   parentry=iel->GetEntry(parentry);
  }
  return parentry;
}
void THSOutput::SortTree(TTree* tree){
  //reorder events in the tree so the order is preserved
  //typically PROOF will reorder events, but order preservation is
  //useful for connecting with parent trees
  //Make sure the file you are writing to is the current directory
  
  //order the events based on the global ID variable, which came from the original tree
  cout<<"THSOutput::SortTree Sorting tree You may want to disable this?"<<endl;
  if(!tree) return;
  if(!tree->GetBranch("fgID")) return;
  cout<<" Make index "<< tree->BuildIndex("fgID")<<endl;
  TTreeIndex *index = (TTreeIndex*)tree->GetTreeIndex();
  TTree* cltree=tree->CloneTree(0); //create empty tree with branch adresses set
  cltree->SetAutoSave();
  for( int i =  0; i < index->GetN() ; i++ ) {
    Long64_t local = tree->LoadTree( index->GetIndex()[i] );
    tree->GetEntry(local);
    cltree->Fill(); //fill as ordered by the build index
  }
  cltree->Write("",TObject::kOverwrite); //replace current tree
}
void THSOutput::PrepareOutDir(){
  //Make the outpur directory if requested
  //If it exists exit so we do not copy over any files
  if(!fOutName.EndsWith(".root")&&gSystem->MakeDirectory(fOutName)!=0){// onlymake directory first time and only if it doesn't exist
      Error("THSOutput::PrepareOutDir()", "Directory given  is not approriate, it either exists or its parent doesn't, I would rather not overwrite it in case it contains files you need. It is advised t set fOutput=\"outfile or outdir\" in your selector constructor, this way it can be seen by all the worker servers.");
      cout<<"Directory Name = "<<fOutName<<endl;
      gSystem->Exit(1);
    }
}
void THSOutput::InitOutFile(TTree* chain){
   //check fOutName to see if making 1 or many files
  if(fFile&&fOutName.EndsWith(".root"))return;//only saving one file

  //create a new output file to save tree to
  //can be called as part of notify to switch filename to match input file
   TString ofname;
  if(fOutName.EndsWith(".root")) ofname=fOutName;
  else  {
    ofname =gSystem->BaseName((fCurTree->GetCurrentFile()->GetName()));
    ofname.Prepend(fOutName+"/");
  }
  cout<<"InitOut "<<ofname<<endl;
  TDirectory* savedir=gDirectory;
  Info("Notify", "processing file: %s", ofname.Data());
  if(fFile)	SafeDelete(fFile);
   //if(fProofFile)	SafeDelete(fProofFile);
  cout<<"Making new proof file "<<ofname<<endl;
  fProofFile = 0;
  fFile = 0;
  fProofFile = new TProofOutputFile(ofname,"M");
  fFile = fProofFile->OpenFile("RECREATE");
  if (fFile && fFile->IsZombie()) SafeDelete(fFile);
  // Cannot continue
   if (!fFile) {
     Info("SlaveBegin", "could not create '%s': instance is invalid!", fProofFile->GetName());
     return;
   }
   //if it exists give the tree to the file
   
   gDirectory = savedir;

}
void THSOutput::InitOutTree(){
  //Function to set tree file and make sure fgID branch exists
  //used to be part of InitOutTree but split because needs to be called after fOutTree create
   if(fOutTree&&fFile){
     fOutTree->SetDirectory(fFile);
     fOutTree->AutoSave();
     //cout<<fOutTree->GetBranch("fgID")<<fSaveID<<endl;
     if(!fOutTree->GetBranch("fgID"))fOutTree->Branch("fgID", &fgID, "fgID/D");
     if(!fSaveID)//copy existing global ID
       fOutTree->SetBranchAddress("fgID",fCurTree->GetBranch("fgID")->GetAddress());
   }
}
void THSOutput::CopyCode(TDirectory* curDir,TDirectory* prevDir){

  //Function to copy all source codes to the output file (curDir)or outputlist
  //If any code exists in the input file (prevDir) this is copied as a sublist
  //in the new list.
  //The code is saved as TMacros ans are stored in a TList (easier to use than TDirectory)
  TDirectory* savedir=gDirectory;
  TDirectory* prevStep=0;
  //Check to see if code already exists in parent file
  TIter next(prevDir->GetListOfKeys());
  TKey* key=0;
  //Look for a directory in the input root file which includes HSStep
    //it will be copied to the new output root file
  while ((key = (TKey*)next())) if(TString(key->GetName()).Contains("HSStep")){
      prevStep=dynamic_cast<TDirectory*>(key->ReadObj());
      break;
    }
  if(prevStep){
    //We have the saved last step, we will save this in the current source step
    //so as to contain the full analysis chain
    //the current step will have the HSStep_number incremented by 1
    fStepName= prevStep->GetName();
    TString prevstepi=TString(fStepName(fStepName.Index("_")+1,fStepName.Length()-fStepName.Index("_"))); //get the number as a string
    Int_t prevStepN=prevstepi.Atoi(); //convert it to an int
    TString stepi;
    stepi=stepi.Itoa(prevStepN+1,10); //add 1 and convert back to string
    fStepName.ReplaceAll(prevstepi,stepi); //now have the new step name
  }
  else fStepName="HSStep_0";

  //create list of current source, prepare to add previous code form in file
  if(fStepDir) delete fStepDir; //cleanup previous step directory
  fStepDir=(TList*)fCodeList->Clone();
  fStepDir->SetOwner();
  fStepDir->SetName(fStepName);
  
  //If there was a previous step copy its source to the new step list
  if(prevStep) fStepDir->Add(CopyDirtoList(prevStep));
  //Write the source code to the output file curDir
  // curDir->cd();
  // WriteListtoFile(fStepDir);
  savedir->cd();

}
void THSOutput::ImportSysDirtoList(const char *dirname,TList* list) {
  //based on $ROOTSYS/tutorials/io/importCode.C       
  //  char *slash = (char*)strrchr(dirname,'/');
  TDirectory *savdir = gDirectory;
  void *dirp = gSystem->OpenDirectory(dirname); //open directory on system
  if (!dirp) return;
  char *direntry;
  Long_t id, size,flags,modtime;
  //loop on all entries of this directory
  while ((direntry=(char*)gSystem->GetDirEntry(dirp))) {
    TString afile = Form("%s/%s",dirname,direntry);
    gSystem->GetPathInfo(afile,&id,&size,&flags,&modtime);
    if (direntry[0] == '.')             continue; //forget the "." and ".." special cases
    if (!strcmp(direntry,"CVS"))        continue; //forget some special directories
    if (!strcmp(direntry,"htmldoc"))    continue;
    if (strstr(dirname,"root/include")) continue;
    if (strstr(direntry,"G__"))         continue;
    if (strstr(direntry,".root"))         continue;
    if (strstr(direntry,"~"))         continue;
    //Copy all of these types of files
    if (strstr(direntry,".c")    ||
	strstr(direntry,".h")    ||
	strstr(direntry,".dat")  ||
	strstr(direntry,".py")   ||
	strstr(direntry,".txt")   ||
	strstr(direntry,".C")) {
      TMacro *m = new TMacro(afile);
      m->SetName(gSystem->BaseName(afile));
      list->Add(m);
  
    } else {
      if (flags != 3)                     continue; //must be a directory
 	//*************remove copying of lower level directories
       //we have found a valid sub-directory. Process it
      // TList* list1=new TList(); //create new sublist
      // list1->SetName(afile);
      // list1->SetOwner();
      // ImportSysDirtoList(afile,list1); //write the sub directory to the sublist
      // list->Add(list1); //add sub list
    }
  }
  gSystem->FreeDirectory(dirp);
  savdir->cd();
}

TList* THSOutput::CopyDirtoList(TDirectory *source) {
  //copy all objects and subdirs of directory source as a tlist
  //This should be easy, but for some reason is not. 
  //This code is based on tutorials/io/copyFiles.C   
  TList* list=new TList();
  list->SetName(source->GetName());
  list->SetOwner();
  TDirectory *savdir = gDirectory;
  //loop on all entries of this directory
  TKey *key=0;
  TIter nextkey(source->GetListOfKeys());
  while ((key = (TKey*)nextkey())) {
      const char *classname = key->GetClassName();
      TClass *cl = gROOT->GetClass(classname);
      if (!cl) continue;
      if (cl->InheritsFrom(TDirectory::Class())) {
       source->cd(key->GetName());
         TDirectory *subdir = gDirectory;
         list->Add(CopyDirtoList(subdir));
      } else if (cl->InheritsFrom(TTree::Class())) {
         TTree *T = (TTree*)source->Get(key->GetName());
	 TTree *newT = T->CloneTree(-1,"fast");
         list->Add(newT);
      } else {
         source->cd();
         TObject *obj = key->ReadObj();
	 list->Add(obj);
      }
  }
  savdir->cd();
  return list;
}
void THSOutput::WriteListtoFile(TList* list0){
  //Contents of list will be written in current file  
  //loop on all entries of this list
  if(!gDirectory->IsWritable()) return;
  //make directory for this list
  TDirectory *saveDir = gDirectory;
  TDirectory *curDir = gDirectory->mkdir(list0->GetName());
  curDir->cd();
  TKey *key;
  TIter nextkey(list0);
  //loop through list and  make new directories
  while ((key = (TKey*)nextkey())) {if(TString(key->GetName()).Contains("HSStep")){//key->GetClassName() causes a crash...
      TList* list1=(TList*)list0->FindObject(key->GetName());
      WriteListtoFile(list1); //write sublist to a subdirectory
    } 
    //write the objects in this list
    else key->Write(0, TObject::kOverwrite);
  }
  // list0->Write(0, TObject::kOverwrite);
  //back to original file directory
  saveDir->cd();
}

Int_t THSOutput::CountChar(TString tests,char testc){
  //based on TString function of same name which for some reason takes an int not a char
  Int_t count = 0;
  Int_t len   = tests.Length();
  const char *data  = tests.Data();
  for (Int_t ccn = 0; ccn < len; ccn++)
    if (data[ccn] == testc) count++;
  
  return count;
}
