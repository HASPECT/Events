
{
  // gSystem->AddIncludePath("-I");
  gROOT->LoadMacro("THSBins.C+");
  gROOT->LoadMacro("THSWeights.C+");
  gROOT->LoadMacro("THSRooFit.C+");
  gROOT->LoadMacro("THSEventsPDF.C+");
  gROOT->LoadMacro("THSSimFit.C+");

  THSSimFit* RF=new THSSimFit("SF");
  RF->SetOutDir("outBins0/");
  ///////////////////////////////Load Variables
  RF->LoadVariable("Mmiss[2,8]");//should be same name as variable in tree  
  RF->LoadBinVars("Eg",1,3,4);//should be same name as variable in tree 
  RF->LoadBinVars("M1",4,0,10);//should be same name as variable in tree 

  /////////////////////////////Make Model Signal
  RF->Factory("THSEventsPDF::Signal(Mmiss,alpha[0,0,1],off[0,-2,2],scale[1.,0.9,1.1],100,20)");
  TChain *chainmcL=new TChain("MyModel","mcsignal");
  chainmcL->Add("SigData.root");
  RF->SetModelEventsTree("Signal",chainmcL);
  RF->LoadSpeciesPDF("Signal",1);
  // RF->GetWorkSpace()->var("alpha")->setConstant();
  //RF->GetWorkSpace()->var("scale")->setConstant();
  //RF->GetWorkSpace()->var("off")->setConstant();

  ////////////////////////////////Additional background
  RF->Factory("THSEventsPDF::BG(Mmiss,alphaB[0,0,1],offB[0,0,0],scaleB[1.,1,1],25,20)");
  TChain *chainmcB=new TChain("MyModel","mcsignal");
  chainmcB->Add("BGData.root");
  RF->SetModelEventsTree("BG",chainmcB);
  RF->LoadSpeciesPDF("BG",2);
  // RF->GetWorkSpace()->var("alphaB")->setConstant();
  // RF->GetWorkSpace()->var("scaleB")->setConstant();
  // RF->GetWorkSpace()->var("offB")->setConstant();

   ///////////////////////////Load Data
  TChain chain("MyModel");
  chain.AddFile("Data.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain,kFALSE);
  gBenchmark->Start("Binned");
  RF->PrepareForFarm();
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");



}
