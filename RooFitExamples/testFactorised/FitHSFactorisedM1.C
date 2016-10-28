
{
  gSystem->AddIncludePath("-I$HSANA/");
  gROOT->LoadMacro("$HSANA/THSBins.C++");
  gROOT->LoadMacro("$HSANA/THSWeights.C++");
  gROOT->LoadMacro("$HSANA/THSRooFit.C++");
  gROOT->LoadMacro("$HSANA/THSsPlot.C++");
  gROOT->LoadMacro("$HSANA/THSEventsPDF.C++");
  gROOT->LoadMacro("$HSANA/THSEventsFit.C++");

  THSEventsFit* RF=new THSEventsFit("SF");
  RF->SetOutDir("outM1/");
  //  RF->SetBinnedFit(); //Make it much faster binned chi2 for parameters
  //  RF->SetSingleSpecies("BG2"); //Merge all backgrounds into 1 PDF for sFit
  //  RF->SetSPlotRange(3,7); //Use a differnt range for sFit
  RF->SetWithConstraints(); //Use Gaussian contraints on alpha,off,scale
  ///////////////////////////////Load Variables
  RF->LoadVariable("M1[0,10]");//should be same name as variable in tree 
  RF->GetWorkSpace()->var("M1")->setBins(100); 
  RF->LoadBinVars("Eg",1,3,4);//should be same name as variable in tree 
  RF->LoadBinVars("M2",1,0,10);//should be same name as variable in tree 

  cout<<"OUTDIR "<<RF->GetOutDir()<<endl;
  /////////////////////////////Make Model Signal
  //Create event based PDF class
  //alpha=smearing(convoluted gaussian), off=x-axis offset, scale=x-axis scale factor (narrows>1/broadens<1 peak), 100 = number of bins PDF histogram given, 20=number of alpha values used to create smearing
  RF->Factory("THSEventsPDF::Signal(M1,alpha[0,0,1],off[0,-2,2],scale[1.,0.9,1.1],100,20)");
  //Make a chain of events to create PDF shape from
  TChain *chainmcL=new TChain("MyModel","mcsignal");
  chainmcL->Add("SigData.root");
  //Connect model events to EventsPDF
  cout<<"OUTDIR "<<RF->GetOutDir()<<endl;
  RF->SetModelEventsTree("Signal",chainmcL);
  //Add this species to the fit
  RF->LoadSpeciesPDF("Signal",1);
  //RF->GetWorkSpace()->var("alpha")->setConstant();
  //RF->GetWorkSpace()->var("scale")->setConstant();
  //RF->GetWorkSpace()->var("off")->setConstant();

  ////////////////////////////////Additional background
  RF->Factory("THSEventsPDF::BG1(M1,alphaB1[0,0,1],offB1[0,0,0],scaleB1[1.,1,1],100,20)");
  TChain *chainmcB1=new TChain("MyModel","mcsignal");
  chainmcB1->Add("BG1Model.root");
  RF->SetModelEventsTree("BG1",chainmcB1);
  RF->LoadSpeciesPDF("BG1",2);
  // RF->GetWorkSpace()->var("alphaB1")->setConstant();
  // RF->GetWorkSpace()->var("scaleB1")->setConstant();
  // RF->GetWorkSpace()->var("offB1")->setConstant();


   ///////////////////////////Load Data
  TChain chain("MyModel");
  chain.AddFile("Data.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  gBenchmark->Start("Binned");

  RF->LoadWeights("outMmiss//WeightsEF.root","WeightMap");
  RF->SetWeightName("Merged");
  RF->SetDataWeight();

  RF->GetDataSet()->Print();

  RF->RunWeights(3); //Do 3 parameter fits and take the "best"
  RF->DrawTreeVar("Eg",100,3,4);
  RF->DrawTreeVar("M1",100,0,10);
  RF->DrawTreeVar("M2",100,0,10);
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");

  RF->SavePlots("");
  RF->GetWeights()->Save();
}
