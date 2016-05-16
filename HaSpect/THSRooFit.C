#include "THSRooFit.h"
#include <TSystem.h>
#include <TRandom.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <TDirectory.h>
#include <TLegend.h>
#include <TVectorD.h>
#include <RooGenericPdf.h>
#include <RooDataHist.h>
#include <RooChi2Var.h>
#include <algorithm>      // std::sort

ClassImp(THSRooFit);


THSRooFit::THSRooFit() : TNamed(), fWS(0), fID(0),fModel(0), fData(0), fCanvases(0),fHists(0),fTree(0),fSPlot(0),fResult(0),fWeights(0),fDataBins(0),fBinnedFit(kFALSE),fFiti(0),fYld("Yld_"),fSingleSp(""){fSRange[0]=0;fSRange[1]=0;};

THSRooFit::THSRooFit(TString name) : TNamed(name,name), fWS(0),fID(0), fModel(0), fData(0), fCanvases(0),fHists(0),fTree(0),fSPlot(0),fResult(0),fWeights(0),fDataBins(0),fBinnedFit(kFALSE),fFiti(0),fYld("Yld_"),fSingleSp(""){
 fWS = new RooWorkspace("wHS");
 fIDBranchName="DontHaveOneYet";//defualt ID branch name
 fGotID=kFALSE;
 fCanvases=new TList();fCanvases->SetOwner();
 fHists=new TList();fHists->SetOwner();
 fRooFits=new TList();fRooFits->SetOwner();
 fSRange[0]=0;fSRange[1]=0;
}
THSRooFit::THSRooFit(THSRooFit* rf){
   THSRooFit();
   //copy constructor, but do not copy the data tree, load that explicitly
   if(rf->GetWorkSpace()){fWS=(RooWorkspace*)rf->GetWorkSpace()->Clone();}
  if(rf->GetModel())fModel=fWS->pdf(rf->GetModel()->GetName());
  for(Int_t i=0;i<rf->GetVariables().getSize();i++)
    fVariables.add(*(fWS->var(rf->GetVariables()[i].GetName())));
  for(Int_t i=0;i<rf->GetAuxVars().getSize();i++)
    fAuxVars.add(*(fWS->var(rf->GetAuxVars()[i].GetName())));
  for(Int_t i=0;i<rf->GetBinVars().getSize();i++)
    fBinVars.add(*(fWS->var(rf->GetBinVars()[i].GetName())));
  for(Int_t i=0;i<rf->GetParameters().getSize();i++)
    fParameters.add(*(fWS->var(rf->GetParameters()[i].GetName())));
  for(Int_t i=0;i<rf->GetYields().getSize();i++)
    fYields.add(*(fWS->var(rf->GetYields()[i].GetName())));
  for(Int_t i=0;i<rf->GetPDFs().getSize();i++)
    fPDFs.add(*(fWS->var(rf->GetPDFs()[i].GetName())));
  if(rf->GetBins())fDataBins=(THSBins*)rf->GetBins()->Clone();
  if(rf->fID) fID=rf->fID;
  fBinnedFit=rf->fBinnedFit;
  fSingleSp=rf->fSingleSp;

}

THSRooFit::~THSRooFit(){

  if(fWS) delete fWS;
  //if(fCanvases)delete fCanvases;
  if(fHists)delete fHists;
  if(fResult) delete fResult;
  if(fWeights) delete fWeights;
  if(fSPlot) delete fSPlot;
  //if(fRooFits) delete fRooFits;
  if(fDataBins) delete fDataBins;
  // if(fID) delete fID;
}

void THSRooFit::LoadDataSet(TTree* tree,Bool_t toWS){
  //Take a tree and convert to RooDataSet
  if(!tree) {cout<<"Tree does not exist "<<endl;exit(0);}
  fTree=tree;
  RooArgSet dataVars(fVariables,fAuxVars);
  if(fID) dataVars.add(*fID);
  fData=new RooDataSet(tree->GetName(),tree->GetTitle(),dataVars,RooFit::Import(*tree));
  if(toWS) fWS->import(*fData); //import if told to
  else if(!(fDataBins))fWS->import(*fData); //if not told to import if not bins
  //else if not told to import and there are bins don't import
  cout<<"THSRooFit::LoadDataSet(TTree*) Print dataset for "<<tree->GetName()<<endl;
  fData->Print();
}
void THSRooFit::RemoveDataSet(){
  if(!fTree)return;//no tree dataset
  delete fTree;
  if(fData) {fData->SetName("Empty");fData->reset();}
  //if(fData) delete fData;//crashes workspace...
}
void THSRooFit::LoadDataSet(RooAbsData* data,Bool_t toWS){
  if(!data) {cout<<"LoadDataSet(RooAbsData*) No valid data set "<<endl;return;}
  //Take a tree and convert to RooDataSet
  //if(fWS->pdf("TotalPDF"))fWS->pdf("TotalPDF")->SetNameTitle("OldPDF","OldPDF");
  if(toWS) fWS->import(*data);
  else if(!(fDataBins))fWS->import(*data); //if not told to import if not bins
  //else if not told to import and there are bins don't import
  fData=data;
  cout<<"THSRooFit::LoadDataSet Print dataset for "<<fData->GetName()<<endl;
  fData->Print();
}
void THSRooFit::LoadWorkSpaceData(RooWorkspace* ws){
  LoadWorkSpace(ws);
  LoadDataSet(ws->allData().front());//assumes only 1 data set!!
}
void THSRooFit::LoadWorkSpace(RooWorkspace* ws){
  //load model from workspace, does not load data
  //This allows to create a new THSRooFit object for different trees

  if(ws)fWS=new RooWorkspace(*ws);
  if(fWS->pdf("TotalPDF"))fModel=fWS->pdf("TotalPDF");
  if(fWS->set("Variables")){fVariables.add(*(fWS->set("Variables")));fWS->import(fVariables);}
  if(fWS->set("AuxVars"))fAuxVars.add(*(fWS->set("AuxVars")));
  if(fWS->set("BinVars"))fBinVars.add(*(fWS->set("BinVars")));
  if(fWS->set("Parameters"))fParameters.add(*(fWS->set("Parameters")));
  if(fWS->set("Yields"))fYields.add(*(fWS->set("Yields")));
  if(fWS->set("PDFs"))fPDFs.add(*(fWS->set("PDFs")));
  if(fWS->set("ID")) {
    fIDBranchName=(fWS->set("ID"))->first()->GetName();
    fID=fWS->var(fIDBranchName);
    fGotID=kTRUE;
  }
  else fGotID=kFALSE;

  //DefineSets();
  fWS->Print();
}
void THSRooFit::LoadVariable(TString opt){
  fVariables.add(*(fWS->factory(opt)));
}
void THSRooFit::LoadAuxVars(TString opt){
  fAuxVars.add(*(fWS->factory(opt)));
}
// void THSRooFit::LoadAuxVarsBins(TString opt,Int_t nbins,Double_t min,Double_t max){
//   if(!fDataBins) fDataBins=new THSBins();
void THSRooFit::LoadBinVars(TString opt,Int_t nbins,Double_t min,Double_t max){
  if(!fDataBins) fDataBins=new THSBins("HSDataBins");
  //Load a variable that is to be used to define the kinmatic bins
  //opt must be the name of a variable in the tree
  // LoadAuxVars(opt+Form("[%lf,%lf]",min,max));//dummy range, needed for factory to create var
  fBinVars.add(*(fWS->factory(opt+Form("[%lf,%lf]",min,max))));
  fDataBins->AddAxis(opt,nbins,min,max);

}
void THSRooFit::LoadBinVars(TString opt,Int_t nbins,Double_t* xbins){
  if(!fDataBins) fDataBins=new THSBins("HSDataBins");
  //Load a variable that is to be used to define the kinmatic bins
  //opt must be the name of a variable in the tree
  // LoadAuxVars(opt+Form("[%lf,%lf]",min,max));//dummy range, needed for factory to create var
  fBinVars.add(*(fWS->factory(opt+Form("[%lf,%lf]",xbins[0],xbins[nbins-1]))));
  fDataBins->AddAxis(opt,nbins,xbins);

}
RooRealVar* THSRooFit::GetVar(TString name){
  RooRealVar* var=0;
  if(var=GetVariable(name)) return var;
  else if(var=GetAuxVar(name)) return var;
  else cout<<"THSRooFit::GetVar no variable with name "<<name<<endl;
}
void THSRooFit::LoadSpeciesPDF(TString opt,Int_t Scale0){
  //Scale gives relative starting value to other species
  //Get PDF from workspace and add to PDF list
  fPDFs.add(*((RooGenericPdf*)fWS->pdf(opt)));//RooGeneric is just a dummy, add does not take RooAbsPdf
  //fPDFs.find(opt);
  fParameters.add(*(fPDFs.find(opt)->getParameters(RooArgSet(fVariables,fAuxVars))));// get parameters not in fVariables 
  //Add a yield parameter for this species
  fYields.add(*(fWS->factory(fYld+opt+Form("[%d,0,1E12]",Scale0))));//default yields limits

}
void THSRooFit::TotalPDF(){
  //Construct a total PDF whcih is the sum of the species PDFs
  // if(fWS->pdf("TotalPDF"))fWS->pdf("TotalPDF")->SetName("OldTotalPDF");
  RooAddPdf model("TotalPDF","total model",
		  fPDFs, 
		  fYields);
  Int_t Nm=0;  
  while(fWS->pdf(Form("TotalPDF%d",Nm++)));
  model.SetName(Form("TotalPDF%d",Nm-1));
    
  fWS->import(model); //and replace any existing model memory leak?
  fModel=fWS->pdf(model.GetName());
  fModel->Print();
}
void THSRooFit::DefineSets(){	
  //define var types in workspace, for easy reloading
  fWS->defineSet("Variables",fVariables);
  fWS->defineSet("AuxVars",fAuxVars);
  fWS->defineSet("BinVars",fBinVars);
  fWS->defineSet("Parameters",fParameters);
  fWS->defineSet("Yields",fYields);
  fWS->defineSet("PDFs",fPDFs);
}
void THSRooFit::Fit(Bool_t randPar){   
  cout<<"THSRooFit::Fit() "<<endl;
  fModel->Print();
  if(randPar) RandomisePars();
  if (fResult) delete fResult;
  //Initialise yield fit parameters
  for(Int_t iy=0;iy<fYields.getSize();iy++){//set yields below entries in dataset
    fWS->var(fYields[iy].GetName())->setRange(0,fData->sumEntries()*2); 
    fWS->var(fYields[iy].GetName())->setVal(fData->sumEntries()/2/fWS->var(fYields[iy].GetName())->getVal());//scale by value give in LoadSpecies 
  }
 
  if(fBinnedFit){
    RooRealVar *var=((RooRealVar*)&fVariables[0]);//assume fitting first variable
    Double_t hmin;
    Double_t hmax;
    fData->getRange(*var,hmin,hmax);
    // var->setRange("R1",hmin,hmax);
    //TH1D* hist=(TH1D*)fData->fillHistogram(new TH1D("binned","binned",var->getBins(),var->getMin(""),var->getMax("")),RooArgList(*var));
    TH1D* hist=(TH1D*)fData->fillHistogram(new TH1D("binned","binned",var->getBins(),hmin,hmax),RooArgList(*var));
    if(hist->GetMinimum()<10) hist->Rebin();
    Bool_t ForceMLL=kFALSE;
    if(hist->GetMinimum()<5){cout<<"THSRooFit::Fit binned fit requested but some bins have few entries, switching to likelihood "<<endl;ForceMLL=kTRUE;}
    hist->Sumw2();
    RooDataHist rdh(Form("chi2hist%d",fFiti),Form("chi2hist%d",fFiti),RooArgList(*var),RooFit::Import(*hist));
    cout<<"MAX MIN VALUES "<<hmin<<" "<<hmax<<endl;
    //if data histgram has non integer weights  must do chi2 fit as likelihood won't deal with weighted histogram
    if(!ForceMLL)fResult=fModel->chi2FitTo(rdh,RooFit::Save(kTRUE),RooFit::Extended(),RooFit::Save(kTRUE),RooFit::DataError(RooAbsData::SumW2),RooFit::Warnings(kFALSE));
    else fResult=fModel->fitTo(rdh,RooFit::Save(kTRUE),RooFit::Extended(),RooFit::Save(kTRUE),RooFit::DataError(RooAbsData::SumW2),RooFit::Warnings(kFALSE));

    delete hist;
   
  }
  else
    fResult=fModel->fitTo(*fData,RooFit::Extended(),RooFit::SumW2Error(kTRUE),RooFit::NumCPU(1),RooFit::Save(kTRUE),RooFit::Warnings(kFALSE));

  fFiti++;

}
void THSRooFit::RandomisePars(){
  //randomise fit parameters
  for(Int_t ip=0;ip<fParameters.getSize();ip++){
    RooRealVar *par=((RooRealVar*)&fParameters[ip]);
    par->setVal(gRandom->Uniform(par->getMin(""),par->getMax("")));
  }
}
void THSRooFit::MergeModelSpecies(){
  if(!fWS->pdf(fSingleSp)) {cout<<"THSRooFit::MergeModelSpecies() single Species not found "<<fSingleSp<<endl;}
  // RooArgList pdfs=fPDFs;
  // pdfs.Print();
  // pdfs.remove(*(pdfs.find(fSingleSp)));
  RooArgList yields=fYields;
  //	 RooArgList yields=((RooAddPdf*)fModel)->coefList();
  // yields.Print();
  // cout<<yields.find(fYld+fSingleSp)<<fYld+fSingleSp<<endl;
  RooRealVar* sYield=(RooRealVar*)yields.find(fYld+fSingleSp);
  yields.remove(*sYield);
  RooArgSet addpdfs = ((RooAddPdf*)fModel)->pdfList();
  RooAbsPdf* sPdf=(RooAbsPdf*)addpdfs.find(fSingleSp);
  addpdfs.remove(*sPdf);
  RooAbsArg* icoef=0;	
 
  //sPlot Range change must recalculate yields for new range!
   if(fSRange[0]!=fSRange[1]){
    RooRealVar* var=((RooRealVar*)&fVariables[0]);
    var->setRange("sRange",fSRange[0],fSRange[1]);
    RooFIter pdfIter = addpdfs.fwdIterator();
    RooAbsArg* ipdf=0;	
    addpdfs.Print();
    yields.Print();
    Int_t iy=0;
    while((ipdf=pdfIter.next())){ 	     
      Double_t inte1=((RooAbsPdf*)ipdf)->createIntegral(fVariables)->getVal();
      Double_t inte2=((RooAbsPdf*)ipdf)->createIntegral(fVariables,RooFit::Range("sRange"))->getVal();
      Double_t yd=((RooRealVar*)&yields[iy])->getVal();
      yd=yd*inte2/inte1;
      cout<<"THSRooFit::MergeModelSpecies() chenge rangeo of "<<((RooRealVar*)&yields[iy])->GetName()<<" "<<((RooRealVar*)&yields[iy])->getVal()<<" by "<<inte2/inte1<<" to "<<yd<<endl;
      ((RooRealVar*)&yields[iy])->setVal(yd);
      iy++;
    }	
    //change yield of single pdf too
    Double_t inte1=sPdf->createIntegral(fVariables)->getVal();
    Double_t inte2=sPdf->createIntegral(fVariables,RooFit::Range("sRange"))->getVal();
    Double_t yd=sYield->getVal();
    yd=yd*inte2/inte1;
    cout<<"THSRooFit::MergeModelSpecies() chenge rangeo of "<<sYield->GetName()<<" "<<sYield->getVal()<<" by "<<inte2/inte1<<" to "<<yd<<endl;
  
   }
   yields.setAttribAll("Constant");
   RooAddPdf* rap=new RooAddPdf(fSingleSp+"Merged",fSingleSp+"Merged",addpdfs,yields);
   rap->getParameters(*fData)->setAttribAll("Constant");
  //fWS->import(*rap);

  //remove all pdfs and add back single species and summed pdfs
  fPDFs.releaseOwnership();
  fPDFs.removeAll();
  fPDFs.add(*(sPdf));
  fPDFs.add(*rap);

  fYields.releaseOwnership();
  fYields.removeAll();
  fYields.add(*(sYield)); //add back in single species yld
  
  //construct new yeild paramter for sum of other contribtutions
  Double_t Ym=0; //first sum the fitted yields
  for(Int_t iy=0;iy<yields.getSize();iy++){
    Ym+=((RooRealVar*)&yields[iy])->getVal();
    //Ym+=((RooRealVar*)icoef)->getVal();
    //Double_t val=((RooRealVar*)icoef)->getVal();
    cout<<iy<<" CHECKING COEF "<<((RooRealVar*)&yields[iy])->getVal()<<endl;	
  }
  cout<<"Ym "<< Ym<<endl;
  RooRealVar* ymerge=(RooRealVar*)fWS->factory(fYld+"Merged[0,0,1]");
  (ymerge)->setMin(0,0);
  (ymerge)->setMax(0,2*Ym);
  (ymerge)->setVal(Ym);
  fYields.add(*ymerge);
  ymerge->Print();
  cout<<"THSRooFit::MergeModelSpecies() "<<endl;
  fPDFs.Print();
  fYields.Print();
  TotalPDF();
 
}
void THSRooFit::sPlot(){
  if(fTree)
    cout<<"THSRooFit::sPlot() weights will be synchronised to branch "<<fIDBranchName<<" "<<fTree<<" "<<fTree->GetBranch(fIDBranchName)<<endl;
  //Perform fit of yields only to get sWeights
  //Check if there is an ID branch for synchronisation
  //check if ID branch exists
  if(fTree){
    if(fTree->GetBranch(fIDBranchName)){
      fGotID=kTRUE;
      cout<<"THSRooFit::sPlot Loading ID branch data "<<fIDBranchName<<endl;
     }
    else {cout<<"THSRooFit::sPlot() warning No ID branch called will just use tree entry number. This means weights will fail to synchronise with initial tree if you cut events or use bins"<<fIDBranchName<<endl;}
  }
  //Option here to sum background contributions and fix relative yields
  if(fSingleSp!=TString("")) MergeModelSpecies();
   //Set all the model parameters to be constant, so only fitting the species yields
  //calculate the sWeights 
  fModel->Print();
  fYields.Print();
  // fSPlot = new RooStats::SPlot("splot_fit", "sPlot Fit", *((RooDataSet*)fData),fModel ,fYields);
  cout<<"Entries "<<fData->numEntries()<<endl;
  if(fSRange[0]!=fSRange[1]){//sub range set for sPlot fit, remake dataset
   ((RooRealVar*)&fVariables[0])->setRange(0,fSRange[0],fSRange[1]);
    RooArgSet dataVars(fVariables,fAuxVars);
    if(fID) dataVars.add(*fID);
    RooDataSet* tmpData=0;
    if(fTree) tmpData=new RooDataSet(fTree->GetName(),fTree->GetTitle(),dataVars,RooFit::Import(*fTree));
    else tmpData=new RooDataSet(fData->GetName(),fData->GetTitle(),(RooDataSet*)fData,dataVars);
    fData->SetName("Empty");fData->reset();fData=0;
    fData=tmpData;

    fFiti++;
    PlotDataModel();
    ((TCanvas*)fCanvases->Last())->SetTitle(Form("%s Model on new Range = %d",GetName(),fChi2));
    
  }
  if(fBinnedFit) {fBinnedFit=kFALSE;Fit();fBinnedFit=kTRUE;}//if doing multiple binned fits do final MLL for splot
  PlotDataModel();
  ((TCanvas*)fCanvases->Last())->SetTitle(Form("%s Final Likelihood Fit  = %d",GetName(),fChi2));
  fWS->var("Mmiss")->Print();
  cout<<"Entries "<<fData->numEntries()<<endl;
  fParameters.setAttribAll("Constant");
  fSPlot = new RooStats::SPlot("splot_fit", "sPlot Fit", *((RooDataSet*)fData),fModel ,fYields);
  fFiti++;

  fWeights=new THSWeights("HSsWeights");//initialise weights
  fWeights->SetTitle(GetName());
  fWeights->SetFile(fOutDir+TString("Weights")+GetName()+".root");
  if(fTree)ExportTreeWeights();
  else ExportWeights();

 PlotDataModel();
  ((TCanvas*)fCanvases->Last())->SetTitle(Form("%s sWeights Fit chi2 = %d",GetName(),fChi2));

}
Double_t THSRooFit::GetSWeight(Long64_t event,TString species){
  //Note species should include the prepend fYld
  return fSPlot->GetSWeight(event,species);
}
void THSRooFit::ExportWeights(TString wmname){
  cout<<"THSRooFit:ExportWeights "<<endl;
  //fWeights=new THSWeights(wmname);//initialise map
  Int_t NSpecies=fYields.getSize();
  TVectorD eventW(NSpecies); //initialise weights vector
  for(Int_t iw=0;iw<NSpecies;iw++)//set name for each species, remove leading Yld_	
    fWeights->SetSpecies(TString(fYields.at(iw)->GetName()).Remove(0,4));
  for(Long64_t ev=0;ev<fData->numEntries();ev++){//loop over events
    for(Int_t iw=0;iw<NSpecies;iw++)//loop over species
      eventW[iw]=GetSWeight(ev,fYields.at(iw)->GetName());//get weight for this species
    if(fGotID){//use ID from initial tree
      const RooArgSet* vars=fData->get(ev);
      //fWeights->SetWeight(dynamic_cast<RooRealVar*>(vars->find(fIDBranchName))->getValV(),eventW);
      // cout<<ev<<" "<<vars->getRealValue(fIDBranchName)<<" "<<fID->getValV()<<" "<<fData->numEntries()<<fID->GetName()<<endl;
      fWeights->FillWeights((Long64_t)vars->getRealValue(fIDBranchName),eventW);
      } //ID not defined just use entry number on tree
    else fWeights->FillWeights(ev,eventW);
  }
  fData->Print();
}
void THSRooFit::ExportTreeWeights(TString wmname){
  cout<<"THSRooFit::ExportTreeWeights "<<fTree->GetName()<<" "<<fTree->GetEntries()<<" "<<fGotID<<endl;
  if(!fTree) {cout<<"THSRooFit::ExportTreeWeights( No tree found"<<endl;return;}
  //fWeights=new THSWeights(wmname);//initialise map
  //fWeights->SetTitle(GetName());
  Int_t NSpecies=fYields.getSize();
  TVectorD eventW(NSpecies); //initialise weights vector
  for(Int_t iw=0;iw<NSpecies;iw++)//set name for each species,remove "Yld_"
    fWeights->SetSpecies(TString(fYields.at(iw)->GetName()).Remove(0,4));
  Double_t tid=0;
  if(fGotID)  fTree->SetBranchAddress(fIDBranchName,&tid);
  Long64_t Nds=0;
  for(Long64_t ev=0;ev<fTree->GetEntries();ev++){//loop over events
    fTree->GetEntry(ev);
    if(fGotID){//use ID from initial tree
      const RooArgSet* vars=fData->get(Nds);
      Long64_t id=-1;
      if(vars) id=(Long64_t)vars->getRealValue(fIDBranchName);
      if(id==(Long64_t)tid){//event in dataset 
	for(Int_t iw=0;iw<NSpecies;iw++)//loop over species
	  eventW[iw]=GetSWeight(Nds,fYields.at(iw)->GetName());//get weight for this species
	Nds++;//go to next event in dataset
      } 
      else{ //not in dataset no weight calculated for this event set to zero
	for(Int_t iw=0;iw<NSpecies;iw++)//loop over species
	  eventW[iw]=0;//set to zero
	//don't increment Nds
      } //ID not defined just use entry number on tree
      fWeights->FillWeights((Long64_t)tid,eventW); //fill the weight for every tree event
    }
    else {//need to be same events in dataset as tree for this to be meaningful
      for(Int_t iw=0;iw<NSpecies;iw++)//loop over species
	eventW[iw]=GetSWeight(ev,fYields.at(iw)->GetName());//get weight for      
      fWeights->FillWeights(ev,eventW);//no tree id just fill with entry number
    }
  }
 }
void THSRooFit::ExportWeightsToFile(TString filename){
  cout<<"THSRooFit::ExportWeightsToFile Exporting Weights to "<<filename<<endl;
  if(fWeights->GetWeightList())fWeights->SortWeights();//if merged from differen  
   fWeights->Save();
 }
void THSRooFit::AddWeightMap(THSWeights *Wts){
  cout<<"THSRooFit::AddWeightMap Start add weights "<<Wts->Size()<<endl;
  if(!fWeights){//construct new map if not existing
    cout<<"Construct new weightmap"<<endl;
    fWeights=new THSWeights("WeightMap");
    fWeights->SetTitle(GetName());
    fWeights->SetFile(fOutDir+TString("Weights")+GetName()+".root");
  }
  fWeights->Add(Wts);
}
void THSRooFit::AddSubWeights(){
  //look through sub fits and add the maps to the total
  //loop over subfits
  if(!fWeights){//construct new map if not existing
    fWeights=new THSWeights("WeightMap");
    fWeights->SetTitle(GetName());
    //fWeights->SetFile(fOutDir+TString("Weights")+GetName()+".root");
  }
  for(Int_t i=0;i<fRooFits->GetEntries();i++)
    fWeights->Add(dynamic_cast<THSRooFit*>(fRooFits->At(i))->GetWeights());
}
void THSRooFit::PlotModel(TString varName,TString pdfName){
  TCanvas *canvas=0;
  if(!fCanvases){fCanvases=new TList();fCanvases->SetOwner();}
  RooRealVar* var=fWS->var(varName);//get variable
  fCanvases->Add(canvas=new TCanvas(TString(GetName())+varName+pdfName,TString(GetName())+varName+pdfName));//create new canvas for drawing on
  RooPlot* frame = var->frame(); // RooFit frame
  fWS->pdf(pdfName)->plotOn(frame,LineStyle(kSolid), LineColor(kRed));
  frame->SetTitle(pdfName+TString(" versus ")+varName);
  frame->Draw();
  canvas->Modified();
  canvas->Update();

}
void THSRooFit::PlotDataModel(){
  //Function to plot the data and fitted model for each variable

  TCanvas *canvas=0;
  if(!fCanvases){fCanvases=new TList();fCanvases->SetOwner();fCanvases->SetName(TString("RFPlots")+GetName());}
  //Loop over variables
  for(Int_t idr=0;idr<fVariables.getSize();idr++){ 
    RooRealVar* var=fWS->var(fVariables[idr].GetName());//get variable
    fCanvases->Add(canvas=new TCanvas(TString(GetName())+fVariables[idr].GetName()+Form("%d",fFiti),TString(GetName())+fVariables[idr].GetName()));//create new canvas for drawing on
    RooPlot* frame = var->frame(); // RooFit frame 
    fData->plotOn(frame, DataError(RooAbsData::SumW2) ) ; //plot the data
    fModel->plotOn(frame,LineColor(kRed)) ; //model = signal + back fit result 
    //Get the chi2
    fChi2=frame->chiSquare();
    cout<<fFiti<<" THSRooFit::PlotDataModel() chi2 = "<<fChi2<<endl;
    fModel->paramOn(frame,
		    Layout(0.1, 0.4, 0.9),
		    Format("NEU",AutoPrecision(2)),
		    ShowConstants()); //show fit parameters
    frame->SetTitle(TString("Fit components for ")+fVariables[idr].GetName());
    //Loop over components
    for(Int_t ic=0;ic<fPDFs.getSize();ic++)
      fModel->plotOn(frame,Components(fPDFs[ic]),LineStyle(kDashed),LineColor(ic%8+1)) ; //just the back fit result  
    frame->Draw() ;
    canvas->Modified();
    canvas->Update();
  }
}
void THSRooFit::CorrelationWithVars(TString VarName){
  for(Int_t iv=0;iv<fVariables.getSize();iv++){
    new TCanvas;
    // fTree->Draw(VarName+":"+fVariables[iv].GetName()+">>hcorr","","col1");
    fTree->Draw(VarName+":"+fVariables[iv].GetName()+">>hcorr","","goff");
     TH2* his=(TH2*)gDirectory->Get("hcorr");
     cout<<"Correlation factor of "<<VarName<<" with "<< fVariables[iv].GetName()<< " = "<<his->GetCorrelationFactor()<<endl;
     delete his;
  }
}
void THSRooFit::DrawTreeVar(TString VarName,Int_t nbins,Double_t hmin,Double_t hmax){
  cout<<"THSRooFit::DrawTreeVar "<<VarName<<endl;
  if(fTree->GetBranch(fIDBranchName))fGotID=kTRUE;
  if(fWeights->GetTree()->GetEntries()!=fTree->GetEntries()) {cout<<"DrawTreeVar : Tree and Map have different entries!"<<" " <<fWeights->GetTree()->GetEntries()<<" "<<fTree->GetEntries()<<endl;}
  Double_t dVar=0;
  fTree->SetBranchStatus(VarName,1);
  if(fTree->SetBranchAddress(VarName,&dVar)) {cout<<"No variable in tree "<<VarName<<endl; return;}
  Double_t tid=0;
  if(fGotID)  fTree->SetBranchAddress(fIDBranchName,&tid);
  vector<TH1F* > hists;
  //create histogram for each species
  Int_t Nspecies=fWeights->GetNSpecies();
  for(Int_t isp=0;isp<Nspecies;isp++){
    TH1F* htemp=new TH1F(fWeights->GetSpeciesName(isp)+VarName+GetName(),VarName+" SWeighted "+fWeights->GetSpeciesName(isp),nbins,hmin,hmax);
    htemp->Sumw2();
    hists.push_back(htemp);
    fHists->Add(hists[isp]); 
  }
  Long64_t id=0; //can be diffent from event number, and can be filtered in dataset
  Long64_t nds=0;
  // if(!fTree->GetEntries()==fWeights->GetTree()->GetEntries()) {cout<<"Map - Tree size mismatch "<< fTree->GetEntries()<<" "<<fWeights->GetTree()->GetEntries()<<endl;return;}
  //loop over tree and fill with weight
  fWeights->SetCurrEntry(0);
  //  fWeights->Mem();//read weight tree from file into memory
  for(Long64_t ient=0;ient<fTree->GetEntries();ient++){
    fTree->GetEvent(ient);
   //get the id for the event if not in tree
    if(!fGotID) id=ient;
    else id=(Long64_t)tid; //must use ExportTreeWeights or map will crash when there is no entry in dataset
    //fWeights->GetEntry(id);
    if(!fWeights->GetEntryBinarySearch(id)) continue;//look for this id in weights tree
    for(Int_t isp=0;isp<Nspecies;isp++)//Fill each species
       hists[isp]->Fill(dVar,fWeights->GetWeight(isp));
  }
  //create canvas and draw
  TCanvas *canvas=0;
  TLegend* leg = new TLegend(0.6, 0.7, 0.9, 0.9);
  leg->SetTextSize(0.03);
  leg->SetFillColor(0);
  
  fCanvases->Add(canvas=new TCanvas(VarName+"SW",VarName+"SW"));//create new canvas for drawing on
  hists[0]->Draw(); //First time so need axis
  leg->AddEntry(hists[0], hists[0]->GetTitle(), "l");
  Double_t maxVal=hists[0]->GetMaximum();
  for(Int_t isp=1;isp<Nspecies;isp++){
    hists[isp]->SetLineColor(isp);
    leg->AddEntry(hists[isp], hists[isp]->GetTitle(), "l");
    hists[isp]->Draw("same");
    if(maxVal<hists[isp]->GetMaximum()) maxVal=hists[isp]->GetMaximum();
  }
  hists[0]->SetMaximum(maxVal+0.05*maxVal);
  leg->Draw();
  //First check the correlations with other variables in tree
  //CorrelationWithVars(VarName);
  fTree->ResetBranchAddresses();
  //fWeights->Disk();
}
THSRooFit*  THSRooFit::CreateSubFit(TNamed cut){//cut.fName=cut selectionl fcut.fTitle=name
  cout<<"CreateSubFit "<<cut.GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  THSRooFit* RFa=new THSRooFit();
  fRooFits->Add(RFa);
  RFa->SetName(cut.GetName());
  RFa->LoadWorkSpace(GetWorkSpace());
  //speed up copy by turning off redundant branches
  // RFa->SetBranchStatus("*",0);
  //need iterator over fVariables and getName 
  RFa->LoadDataSet(GetTree()->CopyTree(cut.GetTitle()));//will use any EntryList
  return RFa;
}
THSRooFit*  THSRooFit::CreateSubFitBins(TNamed cut){//cut.fName=cut selectionl fcut.fTitle=name
  cout<<"THSRooFit::CreateSubFitBins "<<cut.GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  THSRooFit* RFa=new THSRooFit();
  fRooFits->Add(RFa);
   RFa->SetName(cut.GetName());
   RFa->LoadWorkSpace(fWS);
   //speed up copy by turning off redundant branches
  fTree->SetBranchStatus("*",0);
  //fTree->SetCacheSize(30000000);
  for(Int_t i=0;i<fVariables.getSize();i++){//only copy variable branches for speed
    fTree->SetBranchStatus(fVariables[i].GetName(),1);
    //fTree->AddBranchToCache(fVariables[i].GetName());//??testing if this is faster
  }
  //but always need ID branch
  if(fTree->GetBranch(fIDBranchName)){
   fTree->SetBranchStatus(fIDBranchName,1);
  }
  else cout<<"Warning : THSRooFit::CreateSubFitBins no ID branch set, omitting, and will not be able to save weights"<<endl;
  //need iterator over fVariables and getName 
  RFa->LoadDataSet(fTree->CopyTree(cut.GetTitle()));//will use any EntryList
  fTree->SetBranchStatus("*",1);
  return RFa;
}
THSRooFit*  THSRooFit::CreateSubFitBins(TTree* ctree,Bool_t CopyTree){//events already selected
  cout<<"THSRooFit::CreateSubFitBins with tree "<<ctree->GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  THSRooFit* RFa=new THSRooFit();
  RFa->SetName(ctree->GetName());
  if(fBinnedFit)RFa->SetBinnedFit();
  RFa->SetSingleSpecies(fSingleSp);
  RFa->SetOutDir(fOutDir);
  RFa->SetSPlotRange(fSRange[0],fSRange[1]);
  //Done configuring RF
  fRooFits->Add(RFa);
 
  RFa->LoadWorkSpace(fWS);
  TDirectory *saveDir=gDirectory;
  ctree->GetDirectory()->cd();
  if(CopyTree)RFa->LoadDataSet(ctree->CopyTree(""));//will use any EntryList
  else RFa->LoadDataSet(ctree);//use whole tree
  saveDir->cd();
  return RFa;
}

void THSRooFit::SaveHists(TString filename){
  TFile* file=new TFile(filename,"recreate");
  fHists->Write();
  file->Close();
  delete file;
}
void THSRooFit::SavePlots(TString filename){
  TFile* file=0;
  cout<<"Writin plot "<<fCanvases->GetName()<<" "<<fResult<<endl;
  file=new TFile(fOutDir+TString("Plots")+GetName()+".root","recreate");
  fCanvases->Write();
  if(fResult) fResult->Write("HSFitResult");
  file->Close();
  delete file;
}
void THSRooFit::MakeBins(){
  //also include fit variables in bins as THSBins can then be used to check events
  for(Int_t ib=0;ib<fVariables.getSize();ib++)
    fDataBins->AddAxis(fVariables[ib].GetName(),1,((RooRealVar*)&fVariables[ib])->getMin(""),((RooRealVar*)&fVariables[ib])->getMax(""));
  fDataBins->InitialiseBinTree("HSDataBins",fOutDir+"DataEntries.root");
  fDataBins->RunBinTree(fTree);
  fDataBins->Save();
}
void THSRooFit::RunWithBins(Int_t Nfits){
  DefineSets();
  MakeBins();
  cout<<"THSRooFit::RunWithBins(); number of bins "<<fDataBins->GetN()<<endl;
  TDirectory *saveDir=gDirectory;
  THSBins* savedBins=new THSBins("HSDataBins",fOutDir+"DataEntries.root");
  fTree->SetBranchStatus("*",0);
  for(Int_t i=0;i<fVariables.getSize();i++){//only copy variable branches for speed
    fTree->SetBranchStatus(fVariables[i].GetName(),1);
  }
  //but always need ID branch
  if(fTree->GetBranch(fIDBranchName)){
   fTree->SetBranchStatus(fIDBranchName,1);
  }
  for(Int_t i=0;i<fDataBins->GetN();i++){
    THSRooFit* rf=CreateSubFitBins(savedBins->GetBinnedTree(fTree,i),kFALSE);
  
    rf->TotalPDF();
    rf->FitMany(Nfits);
    rf->sPlot();
    rf->SavePlots(fOutDir+TString("Plots")+fDataBins->GetBinName(i)+".root");
    rf->GetWeights()->PrintWeight();
    AddWeightMap(rf->GetWeights());
    rf->RemoveDataSet();//save memory
    delete rf;
  }
  cout<<"THSRooFit::RunWithBins() Done all Fits "<<endl;
  delete savedBins;
  
  GetWeights()->PrintWeight();
  GetWeights()->SortWeights();
  if(fSingleSp!=TString("")) MergeModelSpecies(); //to be consistent with sub fits
}
void THSRooFit::RunWeights(Int_t Nbins){
  //Do a sWeights fit
  //Maybe new fit so construct PDF if not laready loaded
  if(!fWS->set("PDFs"))DefineSets();
  //if(!fModel)TotalPDF();
  if(!InitialiseFit()) return;
  //Fit the model to data with all paramters free
  FitMany(Nbins);
  //Fit the model to data with only species yields as free pars
  //calculate weights and import to WeightMap
  sPlot();
  //save any canvases produced
  SavePlots(fOutDir+TString("Plots")+GetName()+".root");
  //save weights to file
  GetWeights()->PrintWeight();
  GetWeights()->SortWeights();
  GetWeights()->Save();
}
void THSRooFit::FitMany(Int_t Nfits){
  //Do the fit many times with different initial paramters
  //Maybe new fit so construct PDF if not laready loaded
    if(!fWS->set("PDFs"))DefineSets();
  if(!fModel)TotalPDF();
 
  //Store the likelihood value
  TVectorD loglh(Nfits);
  //Fit the model to data with all paramters free
  //first fit use initial paramter values
  Fit();
  //plot result
  PlotDataModel();
  cout<<"Done fit "<<endl;
  cout<<"Result "<<fResult<<endl;
  if(fBinnedFit) loglh[0]=fChi2; //actually did chi2 fit
  else  loglh[0]=fResult->minNll();
  ((TCanvas*)fCanvases->Last())->SetTitle(Form("Fit %d LogL = %lf",0,loglh[0]));
  fWS->saveSnapshot(Form("ssFit%d",0),RooArgSet(fYields,fParameters),kTRUE);
  TObjArray* fitResults=new TObjArray(Nfits);
  fitResults->SetOwner(kTRUE);
  fitResults->AddLast((RooFitResult*)fResult->clone());
  for(Int_t ifit=1;ifit<Nfits;ifit++){
    Fit(kTRUE);
    //   fWS->saveSnapshot(Form("ssFit%d",ifit),*(fModel->getVariables()),kTRUE);
    fWS->saveSnapshot(Form("ssFit%d",ifit),RooArgSet(fYields,fParameters),kTRUE);
    //fWS->saveSnapshot(Form("YssFit%d",ifit),fYields,kTRUE);
  //plot result
    PlotDataModel();
    //Can only get chi2 after PlotDataModel
    if(fBinnedFit) loglh[ifit]=fChi2; //actually did chi2 fit
    else  loglh[ifit]=fResult->minNll();
   ((TCanvas*)fCanvases->Last())->SetTitle(Form("Fit %d LogL = %lf",ifit,loglh[ifit]));
    fitResults->AddLast((RooFitResult*)fResult->clone());
  }
  cout<<"THSRooFit::RunWeightsMany  Likelihoods "<<endl;
  for(Int_t i=0;i<Nfits;i++){
    cout<<loglh[i]<<endl;
  }	
  //take best result for splot parameters
  Int_t best=TMath::LocMin(Nfits,loglh.GetMatrixArray());
  cout<<"THSRooFit::RunWeightsMany() Best likelihood was "<<loglh[best]<<" "<<best<<" so use those parameters"<<endl;
  fWS->loadSnapshot(Form("ssFit%d",best));
 
  fResult=dynamic_cast<RooFitResult*>(fitResults->RemoveAt(best));//keep for writing to file
  fResult->SetName(Form("Result",best));
  fResult->Print();

  for(Int_t iy=0;iy<fYields.getSize();iy++)
    cout<<((RooRealVar*)&fYields[iy])->GetName()<<" "<<((RooRealVar*)&fYields[iy])->getVal()<<endl;


}

void THSRooFit::PrepareForFarm(){
  DefineSets();
  MakeBins();
  cout<<"THSRooFit::PrepareForFarm(); number of bins "<<fDataBins->GetN()<<endl;
  TDirectory *saveDir=gDirectory;
 THSBins* savedBins=new THSBins("HSDataBins",fOutDir+"DataEntries.root");
  fTree->SetBranchStatus("*",0);
  for(Int_t i=0;i<fVariables.getSize();i++){//only copy variable branches for speed
    fTree->SetBranchStatus(fVariables[i].GetName(),1);
  }
  //but always need ID branch
  if(fTree->GetBranch(fIDBranchName)){
   fTree->SetBranchStatus(fIDBranchName,1);
  }
  for(Int_t i=0;i<fDataBins->GetN();i++){
    THSRooFit* rf=CreateSubFitBins(savedBins->GetBinnedTree(fTree,i),kFALSE);
    //Save workspace to file. This will fitted in a seperate job
    rf->GetWorkSpace()->writeToFile(fOutDir+TString("Farm")+fDataBins->GetBinName(i)+".root");
    
    cout <<"void THSRooFit::PrepareForFarm() Saved Workspace with "<<rf->GetDataSet()->numEntries()<<" for " <<fDataBins->GetBinName(i)<<endl;

    rf->RemoveDataSet();//save memory

    delete rf;
  }
  delete savedBins;

  cout<<"THSRooFit::PrepareForFarm() Done all files "<<endl;
}
void THSRooFit::WriteToFile(TString fname){
  //this currently crashes and could be fixed
  //need to change the version to 1 in ClassDef
  TFile rfFile(fname,"recreate");
  Write();
  rfFile.Close();
}
Bool_t THSRooFit::InitialiseFit(){
  if(GetDataSet()->numEntries()<2) {return kFALSE;}
  if(GetPDFs().getSize()) TotalPDF();
  else {
    cout<<" THSRooFit::InitialiseFit() no model found for "<<GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
    return kFALSE;
  }
  return kTRUE;
    
}
