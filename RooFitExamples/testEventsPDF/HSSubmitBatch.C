//run with
// root 'HSSubmitBatch.C(TString(gSystem->Getenv("PWD"))+"/outBinsFarm/")'
// note need ' '
void HSSubmitBatch(TString fileDir){
  gROOT->LoadMacro("$HSANA/THSBins.C++");
  //Open file containing binning information in a THSBins
  //This also contains entry lists for each event analysed
  TFile* file=new TFile(fileDir+"/DataEntries.root");
  THSBins* DataBins=(THSBins*)file->Get("HSDataBins");
 
  gSystem->Setenv("FILEDIR",fileDir);
  gSystem->Setenv("RUNMAC", TString(gSystem->Getenv("PWD"))+"/HSRunBatchSW.C");
  // gInterpreter->AddIncludePath("./*");
  for(Int_t i=0;i<DataBins->GetN();i++){	
  //for(Int_t i=3;i<4;i++){	
    //loop through the bins and get the workspace files for fitting
    TString JobName=DataBins->GetBinName(i);
    cout<<"sending JobName "<<endl;
    gSystem->Setenv("JOBNAME",JobName);
    gSystem->Exec("qsub pbs_run");
    //gSystem->Exec("./pbs_run");
    gSystem->Exec("sleep 2");
    cout<<" sent job "<< JobName<<endl;
  }
}
 
