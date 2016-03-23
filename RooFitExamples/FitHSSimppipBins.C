
{
  gROOT->LoadMacro("THSEventsPDF.C+");
  gROOT->LoadMacro("THSBins.C+");
  gROOT->LoadMacro("THSRooFit.C+");
  gROOT->LoadMacro("THSSimFit.C+");
  gBenchmark->Start("Binned");

  THSSimFit* RF=new THSSimFit();
  //RF->SetBinTrees();
  RF->SetOutDir("testSim4/");
  /////////////////////////////Make Model Signal
  RF->LoadVariable("Mmiss[-0.15,0.19]");//should be same name as variable in tree  
  //RF->LoadAuxVars("Eg[3,3.05]");//should be same name as variable in tree 
  RF->LoadBinVars("Eg",1,3,3.1);//should be same name as variable in tree 
  RF->LoadBinVars("MPipm",1,0.3,2.5);//should be same name as variable in tree 
  // RF->LoadAuxVars("fgID[0,1E12]");//should be same name as variable in tree 
  RF->LoadBinVars("t",1,0,1);//should be same name as variable in tree 

  RF->Factory("THSEventsPDF::Signal(Mmiss,alpha[0.001,0,0.03],off[0,-0.02,0.02],scale[1.,1,1],20)");
  TChain *chainmcL=new TChain("HSParticles","mcsignal");
  chainmcL->Add("/home/dglazier/Dropbox/g11sim/g11_pipi3/ppip/g11_pipi32*root");
  RF->SetModelEventsTree("Signal",chainmcL);

  TFile* csf=new TFile("CrossSection.root");
  TH1F* hist=(TH1F*)csf->Get("t");
  TFile *_filew = TFile::Open("CrossSection.root") ;
  (dynamic_cast<THSEventsPDF*>(RF->GetWorkSpace()->pdf("Signal")))->SetWeightHist((TH1*)_filew->Get("t"),"t");
  _filew->Close();
  delete _filew;
  /////////////////////////////Make Model Background

  RF->Factory("THSEventsPDF::BG(Mmiss,alphaB[0.005,0.,0.08],offB[0,-0.02,0.02],scaleB[1,1,1],20)");
  TChain chainmcB("HSParticles","mcsignal");
  chainmcB.Add("/home/dglazier/Dropbox/g11sim/g11_pipipi03/ppip/g11_pipipi031*root");
  // chainmcB.AddFile("/home/dglazier/Dropbox/g11sim/g11_pipi2/HS/Simplify/b_pippim_cor.root");
  //add mc data to make model
   RF->SetModelEventsTree("BG",&chainmcB);

 ////////////////////////////////Additional background
  RF->Factory("Chebychev::UBG(Mmiss,{a0[0,-1,1],a1[0.2,-1,1]})");


  RF->LoadSpeciesPDF("Signal");
  RF->LoadSpeciesPDF("BG");
  RF->LoadSpeciesPDF("UBG");
  ///////////////////////////Load Data
  TChain chain("HSParticles");
  chain.Add("/home/dglazier/Dropbox/g11pipi/testfit/twopi_ppip_pmiss.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  RF->RunWeights();
  gBenchmark->Stop("Binned");
  gBenchmark->Print("Binned");

}
