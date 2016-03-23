
{
  gROOT->LoadMacro("THSBins.C+");
  gROOT->LoadMacro("THSRooFit.C+");

  THSRooFit* RF=new THSRooFit();
  RF->SetOutDir("testSimple/");
  ///////////////////////////////Load Variables
  RF->LoadVariable("Mmiss[-0.15,0.2]");//should be same name as variable in tree  
  RF->LoadAuxVars("Eg[3,3.1]");//should be same name as variable in tree 
  RF->LoadAuxVars("t[0,0.2]");//should be same name as variable in tree 

  /////////////////////////////Make Model Signal
  RF->Factory("Gaussian::Signal( Mmiss, SIMm[0,-0.01,0.05], SIMw[0.001,0.0001,0.1] )");
  RF->LoadSpeciesPDF("Signal");


  ////////////////////////////////Additional background
  RF->Factory("Chebychev::BG(Mmiss,{a0[-0.8,-1,1],a1[-0.2,-1,1]})");
  RF->LoadSpeciesPDF("BG");

   ///////////////////////////Load Data
  TChain chain("HSParticles");
  chain.AddFile("/home/dglazier/Dropbox/g11pipi/testfit/twopi_ppip_pmiss.root");
  RF->SetIDBranchName("fgID");
  //import to RooFit
  RF->LoadDataSet(&chain);
  RF->RunWeights();

}
