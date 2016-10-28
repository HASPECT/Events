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
#include <RooConstVar.h>
#include <algorithm>      // std::sort

ClassImp(THSRooFit);


THSRooFit::THSRooFit() : TNamed(), fWS(0), fID(0),fModel(0), fData(0), fCanvases(0),fHists(0),fTree(0),fResult(0),fRooFits(0),fInWeights(0),fDataBins(0),fBinnedFit(kFALSE),fFiti(0),fYld("Yld_"),fSingleSp(""){};

THSRooFit::THSRooFit(TString name) : TNamed(name,name), fWS(0),fID(0), fModel(0), fData(0), fCanvases(0),fHists(0),fTree(0),fResult(0),fRooFits(0),fInWeights(0),fDataBins(0),fBinnedFit(kFALSE),fFiti(0),fYld("Yld_"),fSingleSp(""){
 fWS = new RooWorkspace("wHS");
 fIDBranchName="DontHaveOneYet";//defualt ID branch name
 fGotID=kFALSE;
 fCanvases=new TList();fCanvases->SetOwner();
 fHists=new TList();fHists->SetOwner();
 fRooFits=new TList();fRooFits->SetOwner();

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
  for(Int_t i=0;i<rf->GetConstraints().getSize();i++)
    fConstraints.add(*(fWS->var(rf->GetConstraints()[i].GetName())));
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
  //if(fRooFits) delete fRooFits;
  if(fDataBins) delete fDataBins;
  // if(fID) delete fID;
}

void THSRooFit::LoadDataSet(TTree* tree,Bool_t toWS){
  ftoWS=toWS;
  //Take a tree and convert to RooDataSet
  if(!tree) {cout<<"Tree does not exist "<<endl;exit(0);}
  fTree=tree;
  RooArgSet dataVars(fVariables,fAuxVars);
  if(fID) dataVars.add(*fID);
  fData=new RooDataSet(tree->GetName(),tree->GetTitle(),dataVars,RooFit::Import(*tree)); //make roodatset if not splitting onto bins
  if(toWS) fWS->import(*fData); //import if told to
  else if(!(fDataBins))if(!fInWeights)fWS->import(*fData); //if not told to import if not bins
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
  ftoWS=toWS;
  if(!data) {cout<<"LoadDataSet(RooAbsData*) No valid data set "<<endl;return;}
  //Take a tree and convert to RooDataSet
  //if(fWS->pdf("TotalPDF"))fWS->pdf("TotalPDF")->SetNameTitle("OldPDF","OldPDF");
  if(toWS) fWS->import(*data);
  else if(!(fDataBins))if(!fInWeights)fWS->import(*data); //if not told to import if not bins
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
  if(!ws) {cout<<"Warning void THSRooFit::LoadWorkSpace NULL workspace supplied returning"<<endl; return;}
    if(fWS) delete fWS;
  fWS=new RooWorkspace(*ws);
  if(fWS->pdf("TotalPDF"))fModel=fWS->pdf("TotalPDF");
  if(fWS->set("Variables")){fVariables.add(*(fWS->set("Variables")));fWS->import(fVariables);}
  if(fWS->set("AuxVars"))fAuxVars.add(*(fWS->set("AuxVars")));
  if(fWS->set("BinVars"))fBinVars.add(*(fWS->set("BinVars")));
  if(fWS->set("Parameters"))fParameters.add(*(fWS->set("Parameters")));
  if(fWS->set("Yields"))fYields.add(*(fWS->set("Yields")));
  if(fWS->set("PDFs"))fPDFs.add(*(fWS->set("PDFs")));
  if(fWS->set("Constraints"))fConstraints.add(*(fWS->set("Constraints")));
  if(fWS->set("ID")) {
    fIDBranchName=(fWS->set("ID"))->first()->GetName();
    fID=fWS->var(fIDBranchName);
    fGotID=kTRUE;
  }
  else fGotID=kFALSE;

  //DefineSets();
  fWS->Print();
}
void THSRooFit::LoadWeights(TString wfile,TString wname){
  //GetWeights object 
  cout<<"void THSRooFit::LoadWeights "<<wfile<<" "<<wname<<endl;
  if(fInWeights) delete fInWeights;
  fInWeights=0;
  fInWeights=new THSWeights();
  fInWeights->LoadSaved(wfile,wname);
  
  fInWeights->PrintWeight();
}
void THSRooFit::SetDataWeight(){
  if(!fInWeights) return;
  if(fInWeights->GetSpeciesID(fWeightName)<0) return;
  RooArgList setWeights;
  RooRealVar wvar(fWeightName,fWeightName,0);
  setWeights.add(wvar);
  setWeights.Print();
  RooDataSet wData("weights","weights",setWeights) ;

  //Loop over the weights find the entry for the event ID
  for(Int_t ev=0;ev<fData->numEntries();ev++){
    const RooArgSet* vars=fData->get(ev);
    fInWeights->GetEntryBinarySearch((Long64_t)vars->getRealValue(fIDBranchName));
    // wvar.setVal(fInWeights->GetWeight(fWeightName));
    wvar.setVal(fInWeights->GetWeight(fWeightName));
    wData.add(setWeights);
  }
  wData.Print();
  ((RooDataSet*)fData)->merge(&wData);
  fData->Print();
  RooDataSet* tmpData=0;
  tmpData=new RooDataSet(TString("Weight_")+fWeightName+fData->GetName(),fData->GetTitle(),(RooDataSet*)fData,*fData->get(),0,fWeightName);
  fData->SetName("Empty");fData->reset();fData=0;
  fData=tmpData;
  fData->Print();
  if(ftoWS) fWS->import(*fData); //import if told to
  else if(!(fDataBins))fWS->import(*fData); //or no databins to confuse it 
}
void THSRooFit::SetWeightName(TString WName){
  if(!fInWeights) return;
  if(fInWeights->GetSpeciesID(WName)<0) return;
  fWeightName=WName;
}
void THSRooFit::LoadVariable(TString opt){
  fVariables.add(*(fWS->factory(opt)));
}
// void THSRooFit::LoadCategory(TString opt){
//   fVariables.add(*(fWS->factory(opt)));
// }
void THSRooFit::LoadAuxVars(TString opt){
  fAuxVars.add(*(fWS->factory(opt)));
}
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
  fWS->defineSet("Constraints",fConstraints);
}
void THSRooFit::Fit(Bool_t randPar){   
  cout<<"THSRooFit::Fit() "<<endl;
  if(!fModel) fModel=fWS->pdf("K0Lambda");
  fModel->Print();
  if(randPar) RandomisePars();
  if (fResult) delete fResult;
  //Initialise yield fit parameters
  for(Int_t iy=0;iy<fYields.getSize();iy++){//set yields below entries in dataset
    if(fData->sumEntries()<=1){
      cout<<"void THSRooFit::Fit SUMENTRIES "<<fData->sumEntries()<<endl;;
      fWS->var(fYields[iy].GetName())->setRange(0,fData->numEntries()); 
      fWS->var(fYields[iy].GetName())->setVal(1E3/2/fWS->var(fYields[iy].GetName())->getVal());//scale by value give in LoadSpecies 

    }
    fWS->var(fYields[iy].GetName())->setRange(0,fData->sumEntries()*2); 
    fWS->var(fYields[iy].GetName())->setVal(fData->sumEntries()/2/fWS->var(fYields[iy].GetName())->getVal());//scale by value give in LoadSpecies 
  }
  if(fFiti==0){
    DefaultFitOptions();
    //Add any constraints to the likelihood
    fConstraints.Print();
    if(fConstraints.getSize()) AddFitOption(RooFit::ExternalConstraints(fConstraints));
  }
    fFitOptions.Print("");
  if(fBinnedFit){
    if(fConstraints.getSize())cout<<"Warning : THSRooFit::Fit Constraints defined for binned fit, this is will not work in RooFit has to be ML fit"<<endl;  
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
    RooCmdArg dataerr=RooFit::DataError(RooAbsData::SumW2);
    fFitOptions.Print("");
    if(!fFitOptions.FindObject(&dataerr)) fFitOptions.Add(&dataerr);

    if(!ForceMLL)fResult=fModel->chi2FitTo(rdh,fFitOptions);
    else fResult=fModel->fitTo(rdh,fFitOptions);
    
    fFitOptions.Remove(&dataerr);
    delete hist;
    
  }
  else
    fResult=fModel->fitTo(*fData,fFitOptions);
  
  fFiti++;

}
void THSRooFit::RandomisePars(){
  //randomise fit parameters
  for(Int_t ip=0;ip<fParameters.getSize();ip++){
    RooRealVar *par=((RooRealVar*)&fParameters[ip]);
    //check if par this is fxed constant.
    if(par->isConstant()) continue;
    //Look through constraints to see if one is defined for this parameter
    Bool_t hadCon=kFALSE;
    for(Int_t ic=0;ic<fConstraints.getSize();ic++){
      RooAbsPdf *pdfCon=((RooAbsPdf*)&fConstraints[0]);//get RooPdf constraint
      if(pdfCon->getObservables(fParameters)->contains(*par)){ //does it contain par?
	//Yes, must generate random number from constraint
	RooArgSet setPar(*par); //make an argset from this 1 par as needed for..
	RooDataSet *oneEv=pdfCon->generate(setPar,1); //gen 1 event
	const RooArgSet* theEv = oneEv->get(); //get the event
	theEv->getRealValue(par->GetName()); //get par value of event
	hadCon=kTRUE;
	delete oneEv;
	break;//can only have 1!	
      }
    }
    //If there was no constraint to select from just take random in range 
    if(!hadCon)par->setVal(gRandom->Uniform(par->getMin(""),par->getMax("")));
  }
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
    cout<<"Plotting versus "<<fVariables[idr].GetName()<<endl;
    RooRealVar* var=fWS->var(fVariables[idr].GetName());//get variable
    
    if(!var) continue;
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

THSRooFit*  THSRooFit::CreateSubFit(TNamed cut){//cut.fName=cut selectionl fcut.fTitle=name
  cout<<"CreateSubFit "<<cut.GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  THSRooFit* RFa=new THSRooFit();
  fRooFits->Add(RFa);
  RFa->SetName(cut.GetName());
 if(fBinnedFit)RFa->SetBinnedFit();
  RFa->SetSingleSpecies(fSingleSp);
  RFa->SetOutDir(fOutDir);
  // RFa->SetSPlotRange(fSRange[0],fSRange[1]);
  RFa->SetInWeights(fInWeights);
  RFa->SetWeightName(fWeightName);
  RFa->LoadWorkSpace(GetWorkSpace());
  //speed up copy by turning off redundant branches
  // RFa->SetBranchStatus("*",0);
  //need iterator over fVariables and getName 
  RFa->LoadDataSet(GetTree()->CopyTree(cut.GetTitle()));//will use any EntryList
  RFa->SetDataWeight();//if defined weights use them for this dataset
  return RFa;
}
THSRooFit*  THSRooFit::CreateSubFitBins(TNamed cut){//cut.fName=cut selectionl fcut.fTitle=name
  cout<<"THSRooFit::CreateSubFitBins "<<cut.GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  THSRooFit* RFa=new THSRooFit();
  fRooFits->Add(RFa);
  if(fBinnedFit)RFa->SetBinnedFit();
  RFa->SetSingleSpecies(fSingleSp);
  RFa->SetOutDir(fOutDir);
  // RFa->SetSPlotRange(fSRange[0],fSRange[1]);
  RFa->SetInWeights(fInWeights);
  RFa->SetWeightName(fWeightName);
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
  RFa->SetDataWeight();//if defined weights use them for this dataset
  return RFa;
}
THSRooFit*  THSRooFit::CreateSubFitBins(TTree* ctree,Bool_t CopyTree){//events already selected
  cout<<"THSRooFit::CreateSubFitBins with tree "<<ctree->GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  cout<<fOutDir<<endl;
  THSRooFit* RFa=new THSRooFit();
  RFa->SetName(ctree->GetName());
  if(fBinnedFit)RFa->SetBinnedFit();
  RFa->SetSingleSpecies(fSingleSp);
  RFa->SetOutDir(fOutDir);
  // RFa->SetSPlotRange(fSRange[0],fSRange[1]);
  RFa->SetInWeights(fInWeights);
  RFa->SetWeightName(fWeightName);

  // RFa->AddFitOption(RooFit::ExternalConstraints(*((RooAbsPdf*)fWS->pdf("fconstraint")->clone())));
  //Done configuring RF
  fRooFits->Add(RFa);
 
  RFa->LoadWorkSpace(fWS);

  for(Int_t ill=0;ill<fFitOptions.GetSize();ill++)
    RFa->AddFitOption(*((RooCmdArg*)fFitOptions.At(ill)));
  
  TDirectory *saveDir=gDirectory;
  ctree->GetDirectory()->cd();
  if(CopyTree)RFa->LoadDataSet(ctree->CopyTree(""));//will use any EntryList
  else RFa->LoadDataSet(ctree);//use whole tree
  saveDir->cd();
  RFa->SetDataWeight();//if defined weights use them for this dataset
  return RFa;
}
void THSRooFit::SavePlots(TString filename){
  TFile* file=0;
  file=new TFile(fOutDir+TString("Results")+GetName()+".root","recreate");
  if(fCanvases) fCanvases->Write();
  if(fResult) fResult->Write("HSFitResult");
  file->Close();
  delete file;
}
void THSRooFit::MakeBins(){
  //also include fit variables in bins as THSBins can then be used to check events
  // for(Int_t ib=0;ib<fVariables.getSize();ib++)//only if RooRealVar, not for example RooCategory
    // if(dynamic_cast<RooRealVar*>(&fVariables[ib]))fDataBins->AddAxis(fVariables[ib].GetName(),1,((RooRealVar*)&fVariables[ib])->getMin(""),((RooRealVar*)&fVariables[ib])->getMax(""));
  fDataBins->InitialiseBinTree("HSDataBins",fOutDir+"DataEntries.root");
  fDataBins->RunBinTree(fTree);
  fDataBins->Save();
}

void THSRooFit::FitWithBins(Int_t Nfits){
  DefineSets();
  MakeBins();
  cout<<"THSRooFit::FitWithBins(); number of bins "<<fDataBins->GetN()<<endl;
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
  
    // rf->TotalPDF();
    rf->FitMany(Nfits);
    rf->SavePlots(fOutDir+TString("Results")+fDataBins->GetBinName(i)+".root");

    // delete rf;
  }
  cout<<"THSRooFit::FitWithBins() Done all Fits "<<endl;
}

void THSRooFit::FitMany(Int_t Nfits){
  //Do the fit many times with different initial paramters
  //Maybe new fit so construct PDF if not laready loaded
    if(!fWS->set("PDFs"))DefineSets();
    // if(!fModel)TotalPDF();
 
  //Store the likelihood value
  TVectorD loglh(Nfits);
  //Fit the model to data with all paramters free
  //first fit use initial paramter values
  Fit();
  //plot result
  PlotDataModel();
  cout<<"Done fit "<<endl;
  cout<<"Result "<<fResult<<endl;
  if(fBinnedFit&&!TMath::IsNaN(fChi2)) loglh[0]=fChi2; //actually did chi2 fit
  else if(!(TMath::IsNaN(fResult->minNll())||fResult->minNll()<-0.999e+30))  loglh[0]=fResult->minNll();
  else loglh[0]=1E300;
  //((TCanvas*)fCanvases->Last())->SetTitle(Form("Fit %d LogL = %lf",0,fChi2));
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
    // ((TCanvas*)fCanvases->Last())->SetTitle(Form("Fit %d LogL = %lf",ifit,fChi2));
    fitResults->AddLast((RooFitResult*)fResult->clone());
    //Can only get chi2 after PlotDataModel
    if(fBinnedFit&&!TMath::IsNaN(fChi2)) loglh[ifit]=fChi2; //actually did chi2 fit
    else if((TMath::IsNaN(fResult->minNll())||fResult->minNll()!=-1e+30))loglh[ifit]=fResult->minNll();  //loglh[ifit]=fResult->minNll();
    else loglh[ifit]=1E300;
    // cout<<"MANY FITS "<<fResult->covQual()<<endl;
    // if(fResult->status()==0)loglh[ifit]=fResult->minNll();
    //else   loglh[ifit]=1E300;

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

  for(Int_t iy=0;iy<fYields.getSize();iy++){
    cout<<((RooRealVar*)&fYields[iy])->GetName()<<" "<<((RooRealVar*)&fYields[iy])->getVal()<<endl;
    ((RooRealVar*)&fYields[iy])->setError(sqrt(((RooRealVar*)&fYields[iy])->getVal()));
  }

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
  cout<<" THSRooFit::InitialiseFit()"<<endl;
  if(GetDataSet()->numEntries()<2) {return kFALSE;}
  if(GetPDFs().getSize())0;// TotalPDF();
  else {
    cout<<" THSRooFit::InitialiseFit() no model found for "<<GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
    return kFALSE;
  }
  return kTRUE;
    
}
void THSRooFit::DefaultFitOptions(){
  // AddFitOption(RooFit::Extended());
  if(fData->isNonPoissonWeighted())AddFitOption(RooFit::SumW2Error(kTRUE));
  AddFitOption(RooFit::NumCPU(1));
  AddFitOption(RooFit::Save(kTRUE));
  AddFitOption(RooFit::Warnings(kFALSE));

  //if(!fFitOptions.FindObject(&extend)) fFitOptions.Add(&extend);
  //if(!fFitOptions.FindObject(&sumw2))fFitOptions.Add(&sumw2);
  //if(!fFitOptions.FindObject(&ncpu))fFitOptions.Add(&ncpu);
  //if(!fFitOptions.FindObject(save))fFitOptions.Add(save);
  //if(!fFitOptions.FindObject(&warn))fFitOptions.Add(&warn);

}
