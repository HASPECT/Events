
{
  gSystem->AddIncludePath("-I$HSANA/");
  gROOT->LoadMacro("$HSANA/THSBins.C+");
  gROOT->LoadMacro("$HSANA/THSWeights.C+");
  gROOT->LoadMacro("$HSANA/THSRooFit.C+");
  gROOT->LoadMacro("$HSANA/THSsPlot.C+");
  gROOT->LoadMacro("PhiAsymmetry.cxx+"); //Load my new PDF class too!

  THSRooFit* RF=new THSRooFit("AFit");
  RF->SetOutDir("outAsymmetry/");
 ///////////////////////////////Load Variables
  RF->LoadVariable("Phi[-180,180]");//going to fit this
  RF->LoadVariable("Pol[0,1]");//but fix polarisation event-by-event
  RF->LoadVariable("PolState[Polp=1,Polm=-1]");//important this syntax loads the integer PolState as a RooCategory which is required for asymmetries, not RooRealVar

  RF->LoadBinVars("Mmiss",1,0,10);//for consistency same range as signal fit
  RF->LoadBinVars("Eg",4,3,4);//here use smae energy bins

  /////////////////////////////Make asymmetry model A and B are fit parameters
  RF->Factory("PhiAsymmetry::SigAsym( Phi,Pol,PolState,A[0,-1,1],B[0,-1,1] )");
  RF->LoadSpeciesPDF("SigAsym");
   ///////////////////////////Load Data
  TChain chain("MyModel");
  chain.AddFile("Data.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  RF->LoadWeights("outSignal/WeightsbinFit.root ","WeightMap");
  RF->SetWeightName("Signal"); //Same as Signal species in FitHSSimpleBins


  gBenchmark->Start("Binned");
  RF->FitWithBins(1);//argument gives number of parameter fits to perform
   gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");

 


}
