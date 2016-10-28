
{
  gSystem->AddIncludePath("-I$HSANA/");
  gROOT->LoadMacro("$HSANA/THSBins.C++");
  gROOT->LoadMacro("$HSANA/THSWeights.C++");
  gROOT->LoadMacro("$HSANA/THSRooFit.C++");
  gROOT->LoadMacro("$HSANA/THSsPlot.C++");
  gROOT->LoadMacro("$HSANA/THSEventsPDF.C++");
  gROOT->LoadMacro("$HSANA/THSEventsFit.C++");

  THSEventsFit* RF=new THSEventsFit("EF");
  RF->SetOutDir("outMmiss/");
  // RF->SetBinnedFit(); //Make it much faster binned chi2 for parameters
  RF->SetSingleSpecies("BG2"); //Merge all backgrounds into 1 PDF for sFit
  // RF->SetSPlotRange(3,7); //Use a differnt range for sFit
  RF->SetWithConstraints(); //Use Gaussian contraints on alpha,off,scale

  ///////////////////////////////Load Variables
  RF->LoadVariable("Mmiss[0,10]");//should be same name as variable in tree 
  RF->GetWorkSpace()->var("Mmiss")->setBins(100); 
  RF->LoadBinVars("Eg",2,3,4);//should be same name as variable in tree 
  RF->LoadBinVars("M1",1,0,10);//should be same name as variable in tree 

  /////////////////////////////Make Model Signal
  //Create event based PDF class
  //alpha=smearing(convoluted gaussian), off=x-axis offset, scale=x-axis scale factor (narrows>1/broadens<1 peak), 100 = number of bins PDF histogram given, 20=number of alpha values used to create smearing
  RF->Factory("THSEventsPDF::Signal(Mmiss,alpha[0,0,1],off[0,-2,2],scale[1.,0.9,1.1],100,20)");
  //Make a chain of events to create PDF shape from
  TChain *chainmcL=new TChain("MyModel","mcsignal");
  chainmcL->Add("SigData.root");
  //Connect model events to EventsPDF
  RF->SetModelEventsTree("Signal",chainmcL);
  //Add this species to the fit
  RF->LoadSpeciesPDF("Signal",1);
  //RF->GetWorkSpace()->var("alpha")->setConstant();
  //RF->GetWorkSpace()->var("scale")->setConstant();
  //RF->GetWorkSpace()->var("off")->setConstant();

  ////////////////////////////////Additional background
  RF->Factory("THSEventsPDF::BG1(Mmiss,alphaB1[0,0,1],offB1[0,0,0],scaleB1[1.,1,1],100,20)");
  TChain *chainmcB1=new TChain("MyModel","mcsignal");
  chainmcB1->Add("BG1Model.root");
  RF->SetModelEventsTree("BG1",chainmcB1);
  RF->LoadSpeciesPDF("BG1",2);
  // RF->GetWorkSpace()->var("alphaB1")->setConstant();
  // RF->GetWorkSpace()->var("scaleB1")->setConstant();
  // RF->GetWorkSpace()->var("offB1")->setConstant();

  RF->Factory("THSEventsPDF::BG2(Mmiss,alphaB2[0,0,1],offB2[0,0,0],scaleB2[1.,1,1],100,20)");
  TChain *chainmcB2=new TChain("MyModel","mcsignal");
  chainmcB2->Add("BG2Model.root");
  RF->SetModelEventsTree("BG2",chainmcB2);
  RF->LoadSpeciesPDF("BG2",2);
  // RF->GetWorkSpace()->var("alphaB2")->setConstant();
  // RF->GetWorkSpace()->var("scaleB2")->setConstant();
  // RF->GetWorkSpace()->var("offB2")->setConstant();


   ///////////////////////////Load Data
  TChain chain("MyModel");
  chain.AddFile("Data.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  gBenchmark->Start("Binned");
  RF->RunWeights(2); //Do 3 parameter fits and take the "best"
  RF->DrawTreeVar("Eg",100,3,4);
  RF->DrawTreeVar("M1",100,0,10);
  RF->DrawTreeVar("M2",100,0,10);
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");

  RF->SavePlots("");
  RF->GetWeights()->Save();
}
