{
  //LOAD precompiled eloss library
  // setenv ELOSS /home/dglazier/Dropbox/HaSpect/Analysis/eloss_hs
   gSystem->Load("libPhysics");
   gSystem->Load("$ELOSS/libeloss");
   gInterpreter->AddIncludePath("$ELOSS");
  //ELOSS loaded
   
  //Set up the input chain to be analysed
   TChain* tree=new TChain("h10","datachain");
   tree->Add("/run/media/dglazier/hd_dglazier/Data/g11_pipi3/g11_pipi32001*.bos.evt.recsis.root");
 
   //Set the ouput directory or file (if full file name is given just 1 file is created, while if a directory it will be created and filled with files of the same name as the input 
   gSystem->Setenv("HSOUT",
		   "/run/media/dglazier/hd_dglazier/Data/HSANAtest/");
 
   //Configure the analysis (i.e PROOF, source compilation...)
   TString HSANA=gSystem->Getenv("HSANA");
   gROOT->LoadMacro(HSANA+"/HSControl.C");
   TProof* prf=HSControl(kFALSE,kFALSE); //where the proof stuff is set, kTRUE means use PROOF
   if(prf){
     //prf->Load(TString(gSystem->Getenv("ROOTSYS"))+"/lib/libPhysics.so");
     TString ELOSS=gSystem->Getenv("ELOSS");
     prf->Load(ELOSS+"/libeloss.so");
     prf->AddIncludePath(ELOSS);
   }	

   //Use an event list from a previous analysis 
   //If only 1 file was output the event list will be in its top directory, so just give the file name
   //If many files were ouput the event list will be in the same directory with the name ParentEventList.root
   //HSEntryList("/PATH TO PERVIOUS EVENTLIST/ParentEventList.root");
   
   //particle final state, if you expect a missing particle preceed its name with AT and it will be found in the mc truth values
   tree->Process("MCCLAStoHS.C++","pi-:pi+:ATproton");

   //Not actually sure if this is needed or not...
   if(prf)prf->ClearData(TProof::kUnregistered | TProof::kForceClear);
 }
