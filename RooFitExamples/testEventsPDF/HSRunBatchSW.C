void HSRunBatchSW(TString JobName){
  cout<<"Running HSRunBatch on  "<<JobName<<endl;
  gROOT->LoadMacro("THSBins.C++");
  gROOT->LoadMacro("THSWeights.C++");
  gROOT->LoadMacro("THSRooFit.C++");
  gROOT->LoadMacro("THSsPlot.C++");
  gROOT->LoadMacro("THSEventsPDF.C++");

  TFile *JobFile = TFile::Open(TString("Farm")+JobName+".root");
  JobFile->ls();
  RooWorkspace* WS1=(RooWorkspace*)JobFile->Get("wHS");
  THSsPlot* RF=new THSsPlot(JobName);
  // RF->SetBinnedFit(); //Make it much faster binned chi2 for parameters

  WS1->Print();
  RF->SetOutDir("./");
  RF->LoadWorkSpaceData(WS1);
  //Here you could alter parameters,ranges etc, before doing the fit

  //Do Fit and calculate weights
  RF->RunWeights(5);
 
  delete RF;
  //Tidy up
  JobFile->Close();
  delete JobFile;
  if(WS1)delete WS1;
  
}
