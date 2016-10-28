
{
  /////Load the necessary HS classes
  gSystem->AddIncludePath("-I$HSANA/");
  gROOT->LoadMacro("$HSANA/THSBins.C+");
  gROOT->LoadMacro("$HSANA/THSWeights.C+");
  gROOT->LoadMacro("$HSANA/THSRooFit.C+");
  gROOT->LoadMacro("$HSANA/THSsPlot.C+");
  gROOT->LoadMacro("$HSANA/THSEventsPDF.C++");
  gROOT->LoadMacro("$HSANA/THSEventsFit.C++");
  /////Loaded done
  
  THSEventsFit* RF=new THSEventsFit("SF");
  RF->SetOutDir("outBins/");
  // RF->SetBinnedFit(); //Make it much faster binned chi2 for parameters
  RF->SetWithConstraints(); //Use Gaussian contraints on alpha,off,scale
  ///////////////////////////////Load Variables
  RF->LoadVariable("Mmiss[2,8]");//should be same name as variable in tree 
  RF->GetWorkSpace()->var("Mmiss")->setBins(100); 
  RF->LoadBinVars("Eg",1,3,4);//should be same name as variable in tree 
  RF->LoadBinVars("M1",1,0,10);//should be same name as variable in tree 

  /////////////////////////////Make Model Signal
  //Create event based PDF class
  //alpha=smearing(convoluted gaussian), off=x-axis offset, scale=x-axis scale factor (narrows>1/broadens<1 peak), 100 = number of bins PDF histogram given, 20=number of alpha values used to create smearing
  RF->Factory("THSEventsPDF::Signal(Mmiss,alpha[0.01,0,1],off[0,-2,2],scale[1.,0.9,1.1],100,20)");
  //Make a chain of events to create PDF shape from
  TChain *chainmcL=new TChain("MyModel","mcsignal");
  chainmcL->Add("SigData.root");
  //Connect model events to EventsPDF
  RF->SetModelEventsTree("Signal",chainmcL);
  //Add this species to the fit
  RF->LoadSpeciesPDF("Signal",2);
  //RF->GetWorkSpace()->var("alpha")->setConstant();
  //RF->GetWorkSpace()->var("scale")->setConstant();
  //RF->GetWorkSpace()->var("off")->setConstant();
  ////////////////////////////////Additional background
  RF->Factory("THSEventsPDF::BG(Mmiss,alphaB[0,0,1],offB[0,0,0],scaleB[1.,1,1],100,20)");
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
  RF->LoadDataSet(&chain);
  gBenchmark->Start("Binned");
  RF->RunWeights(2); //Do 5 parameter fits and take the "best"
  RF->DrawTreeVar("Mmiss",100,0,10);
  RF->DrawTreeVar("M1",100,0,10);
  RF->DrawTreeVar("M2",100,0,10);
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");

  RF->SavePlots("");
  RF->GetWeights()->Save();
 }
