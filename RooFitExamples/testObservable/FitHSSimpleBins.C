
{
  gSystem->AddIncludePath("-I$HSANA/");
  gROOT->LoadMacro("$HSANA/THSBins.C+");
  gROOT->LoadMacro("$HSANA/THSWeights.C+");
  gROOT->LoadMacro("$HSANA/THSRooFit.C++");
  gROOT->LoadMacro("$HSANA/THSsPlot.C++");

  THSsPlot* RF=new THSsPlot("binFit");
  RF->SetOutDir("outSignal/");
  //RF->SetBinnedFit(); //For parameter fits do binned chi2
  //RF->SetSPlotRange(3,7); //Use a differnt range for sFit
 ///////////////////////////////Load Variables
  RF->LoadVariable("Mmiss[0,10]");//should be same name as variable in tree
  RF->LoadBinVars("Eg",4,3,4);//should be same name as variable in tree 

  /////////////////////////////Make Model Signal
  RF->Factory("Gaussian::Signal( Mmiss, SIMm[6,4,7], SIMw[0.2,0.0001,3] )");
  RF->LoadSpeciesPDF("Signal");

  ////////////////////////////////Additional background
  RF->Factory("Chebychev::BG(Mmiss,{a0[0.1,-0.4,0.4],a1[-0.1,-0.3,0.3]})");
  RF->LoadSpeciesPDF("BG");

   ///////////////////////////Load Data
  TChain chain("MyModel");
  chain.AddFile("Data.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  gBenchmark->Start("Binned");
  RF->RunWeights(2);//argument gives number of parameter fits to perform
  RF->DrawTreeVar("M1",100,0,10);
  RF->DrawTreeVar("M2",100,0,10);
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");
  RF->GetWeights()->Save();
  //chain deleted so recreate for extra plots  
  TChain chain2("MyModel");
  chain2.AddFile("Data.root");
 


}
