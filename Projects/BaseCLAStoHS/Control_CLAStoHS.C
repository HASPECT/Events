{
  //Set the input files
   TChain* tree=new TChain("h10","datachain");
   tree->Add("/home/dglazier/Work/data/kkpi/data*.root");
  
   //Set the ouput directory or file (if full file name is given just 1 file is created, while if a directory it will be created and filled with files of the same name as the input 
   gSystem->Setenv("HSOUT",
		   "/home/dglazier/Work/data/kkpi/hs3/");
   
   //Configure the analysis (i.e PROOF, source compilation...)
   TString HSANA=gSystem->Getenv("HSANA");
   gROOT->LoadMacro(HSANA+"/HSControl.C");
   HSControl(kTRUE,kFALSE); //where the proof stuff is set, kTRUE means use PROOF
   
   //Use an event list from a previous analysis 
   //HSEntryList("~dglazier/Work/Research/HaSpect/data/pippippimMn_HS2/ParentEventList.root");
   
   //RUN
   tree->Process("CLAStoHS.C++","K+:K-:proton");
}
