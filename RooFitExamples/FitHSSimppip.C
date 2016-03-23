
{
  gROOT->LoadMacro("THSEventsPDF.C+");
  gROOT->LoadMacro("THSBins.C+");
  gROOT->LoadMacro("THSRooFit.C+");
  gROOT->LoadMacro("THSSimFit.C+");

  THSSimFit* RF=new THSSimFit();
  RF->SetOutDir("testSim/");
  /////////////////////////////Make Model Signal
  RF->LoadVariable("Mmiss[-0.15,0.2]");//should be same name as variable in tree  
  RF->LoadAuxVars("Eg[3,3.05]");//should be same name as variable in tree 
  RF->LoadAuxVars("fgID[0,1E12]");//should be same name as variable in tree 
  RF->LoadAuxVars("t[2,3]");//should be same name as variable in tree 

  RF->Factory("THSEventsPDF::Signal(Mmiss,alpha[0.0,0,0.03],off[0,-0.01,0.01],scale[1.,1,1],20)");
  TChain *chainmcL=new TChain("HSParticles","mcsignal");
  chainmcL->AddFile("~/Dropbox/g11sim/g11_pipi/g11_pipi3/March16eloss/eloss_ppip_cor.root");
  RF->SetModelEventsTree("Signal",chainmcL);

  TFile* csf=new TFile("CrossSection.root");
  TH1F* hist=(TH1F*)csf->Get("t");
  TFile *_filew = TFile::Open("CrossSection.root") ;
  (dynamic_cast<THSEventsPDF*>(RF->GetWorkSpace()->pdf("Signal")))->SetWeightHist((TH1*)_filew->Get("t"),"t");
  _filew->Close();
  delete _filew;
  /////////////////////////////Make Model Background

  RF->Factory("THSEventsPDF::BG(Mmiss,alphaB[0.005,0,0.04],offB[0,-0.001,0.1],scaleB[1,1,1],10)");
  TChain chainmcB("HSParticles","mcsignal");
  chainmcB.AddFile("/home/dglazier/Dropbox/g11sim/g11_pipi2/HS/Simplify/b_ppip_corEg10.root");
  // chainmcB.AddFile("/home/dglazier/Dropbox/g11sim/g11_pipi2/HS/Simplify/b_pippim_cor.root");
  //add mc data to make model
  RF->SetModelEventsTree("BG",&chainmcB);


  RF->LoadSpeciesPDF("Signal");
  RF->LoadSpeciesPDF("BG");
   ///////////////////////////Load Data
  TChain chain("HSParticles");
  chain.AddFile("/home/dglazier/Dropbox/g11pipi/testfit/twopi_ppip_pmiss.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  gBenchmark->Start("Binned");
  RF->RunWeights();
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");

}
