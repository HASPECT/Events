#include "THSRooFit.h"
#include <TH1.h>
#include <TH2.h>
#include <TDirectory.h>
#include <TLegend.h>
#include <RooGenericPdf.h>
#include <stdexcept>      // std::out_of_range
#include <algorithm>      // std::sort

ClassImp(THSWeightMap);
ClassImp(THSRooFit);

THSRooFit::THSRooFit() : TNamed(), fWS(0), fModel(0), fData(0), fCanvases(0),fHists(0),fTree(0),fSPlot(0),fResult(0),fWMap(0),fDataBins(0){
 fWS = new RooWorkspace("wHS");
 fIDBranchName="fgID";//defualt ID branch name
 fYld="Yld_";
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
  if(rf->GetBins())fDataBins=(THSBins*)rf->GetBins()->Clone();
  if(rf->fID) fID=rf->fID;
}
// THSRooFit::THSRooFit(THSRooFit* rf,Bool_t newModel){
//   //Copy constructor for new datset and option to remove model pdf
//   THSRooFit();
//    if(rf->GetWorkSpace()){fWS=(RooWorkspace*)rf->GetWorkSpace()->Clone();}
//    for(Int_t i=0;i<rf->GetVariables().getSize();i++)
//     fVariables.add(*(fWS->var(rf->GetVariables()[i].GetName())));
//    for(Int_t i=0;i<rf->GetAuxVars().getSize();i++)
//     fAuxVars.add(*(fWS->var(rf->GetAuxVars()[i].GetName())));
//    for(Int_t i=0;i<rf->GetParameters().getSize();i++)
//      fParameters.add(*(fWS->var(rf->GetParameters()[i].GetName())));
//    for(Int_t i=0;i<rf->GetYields().getSize();i++)
//      fYields.add(*(fWS->var(rf->GetYields()[i].GetName())));
//    for(Int_t i=0;i<rf->GetPDFs().getSize();i++)
//      fPDFs.add(*(fWS->var(rf->GetPDFs()[i].GetName())));
//   if(rf->GetBins())fDataBins=(THSBins*)rf->GetBins()->Clone();
//    // LoadDataSet(ds);
//   cout<<"loaded data set"<<endl;
//   // if(newModel) {
//   //   fWS->pdf("TotalPDF")->SetNameTitle("OldPDF","OldPDF");
//   // }
   
// }

THSRooFit::~THSRooFit(){

  if(fWS) delete fWS;
 if(fCanvases)delete fCanvases;
 if(fHists)delete fHists;
 if(fResult) delete fResult;
 if(fWMap) delete fWMap;
 if(fSPlot) delete fSPlot;
 //if(fRooFits) delete fRooFits;
 if(fDataBins) delete fDataBins;
}

void THSRooFit::LoadDataSet(TTree* tree){
  //Take a tree and convert to RooDataSet
  if(!tree) {cout<<"Tree does not exist "<<endl;exit(0);}
  fTree=tree;
  RooArgSet dataVars(fVariables,fAuxVars);
  dataVars.add(*fID);
  RooDataSet data(tree->GetTitle(),tree->GetTitle(),dataVars,RooFit::Import(*tree));
  fWS->import(data);
  fData=fWS->data(tree->GetTitle());
  cout<<"THSRooFit::LoadDataSet(TTree*) Print dataset for "<<tree->GetName()<<endl;
  fData->Print();
}
void THSRooFit::RemoveDataSet(){
  if(!fTree)return;//no tree dataset
  delete fTree;
  if(fData) fData->reset();
  //if(fData) delete fData;//crashes workspace...
}
void THSRooFit::LoadDataSet(RooAbsData* data){
  if(!data) {cout<<"LoadDataSet(RooAbsData*) No valid data set "<<endl;return;}
  //Take a tree and convert to RooDataSet
  if(fWS->pdf("TotalPDF"))fWS->pdf("TotalPDF")->SetNameTitle("OldPDF","OldPDF");
  fWS->import(*data);
  fData=data;
  cout<<"THSRooFit::LoadDataSet Print dataset for "<<fData->GetName()<<endl;
  fData->Print();
}
void THSRooFit::LoadWorkSpace(RooWorkspace* ws){
  //load model from workspace, does not load data
  //This allows to create a new THSRooFit object for different trees
  if(!fModel)TotalPDF();//create model even if PDFs not created
  if(ws)fWS=new RooWorkspace(*ws);
  else return;
  if(fWS->pdf("TotalPDF"))fModel=fWS->pdf("TotalPDF");
  if(fWS->set("Variables")){fVariables.add(*(fWS->set("Variables")));fWS->import(fVariables);}
  if(fWS->set("AuxVars"))fAuxVars.add(*(fWS->set("AuxVars")));
  if(fWS->set("BinVars"))fBinVars.add(*(fWS->set("BinVars")));
  if(fWS->set("Parameters"))fParameters.add(*(fWS->set("Parameters")));
  if(fWS->set("Yields"))fYields.add(*(fWS->set("Yields")));
  if(fWS->set("PDFs"))fPDFs.add(*(fWS->set("PDFs")));
  fIDBranchName=(fWS->set("ID"))->first()->GetName();
  fID=fWS->var(fIDBranchName);
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
  if(!fDataBins) fDataBins=new THSBins();
  //Load a variable that is to be used to define the kinmatic bins
  //opt must be the name of a variable in the tree
  // LoadAuxVars(opt+Form("[%lf,%lf]",min,max));//dummy range, needed for factory to create var
  fBinVars.add(*(fWS->factory(opt+Form("[%lf,%lf]",min,max))));
  fDataBins->AddAxis(opt,nbins,min,max);

}
RooRealVar* THSRooFit::GetVar(TString name){
  RooRealVar* var=0;
  if(var=GetVariable(name)) return var;
  else if(var=GetAuxVar(name)) return var;
  else cout<<"THSRooFit::GetVar no variable with name "<<name<<endl;
}
void THSRooFit::LoadSpeciesPDF(TString opt){
  //Get PDF from workspace and add to PDF list
  fPDFs.add(*((RooGenericPdf*)fWS->pdf(opt)));//RooGeneric is just a dummy, add does not take RooAbsPdf
  //fPDFs.find(opt);
  fParameters.add(*(fPDFs.find(opt)->getParameters(RooArgSet(fVariables,fAuxVars))));// get parameters not in fVariables 
  //Add a yield parameter for this species
  fYields.add(*(fWS->factory(fYld+opt+"[1E3,0,1E12]")));//default yields limits

}
void THSRooFit::TotalPDF(){
  //Construct a total PDF whcih is the sum of the species PDFs
  RooAddPdf model("TotalPDF","total model",
		  fPDFs, 
		  fYields);
  fWS->import(model);
  fModel=fWS->pdf("TotalPDF");
  //define var types in workspace
  fWS->defineSet("Variables",fVariables);
  fWS->defineSet("AuxVars",fAuxVars);
  fWS->defineSet("BinVars",fBinVars);
  fWS->defineSet("Parameters",fParameters);
  fWS->defineSet("Yields",fYields);
  fWS->defineSet("PDFs",fPDFs);
}
void THSRooFit::Fit(){   
  cout<<"THSRooFit::Fit() "<<endl;
  fModel->Print();
  if (fResult) delete fResult;
  //Initialise yield fit parameters
  for(Int_t iy=0;iy<fYields.getSize();iy++){//set yields below entries in dataset
    fWS->var(fYields[iy].GetName())->setRange(0,fData->sumEntries()*2); 
    fWS->var(fYields[iy].GetName())->setVal(fData->sumEntries()/2); 
  }
  fResult=fModel->fitTo(*fData,RooFit::Extended(),RooFit::SumW2Error(kTRUE),RooFit::NumCPU(1));
}
void THSRooFit::sPlot(){
  cout<<"THSRooFit::sPlot() weights will be synchronised to branch "<<fIDBranchName<<" "<<fTree<<" "<<fTree->GetBranch(fIDBranchName)<<endl;
  //Perform fit of yields only to get sWeights
  //Check if there is an ID branch for synchronisation
  //check if ID branch exists
  if(fTree){
    if(fTree->GetBranch(fIDBranchName)){
      fGotID=kTRUE;
      cout<<"THSRooFit::sPlot Loading ID branch data "<<fIDBranchName<<endl;
     }
    else {cout<<"No ID branch called "<<fIDBranchName<<endl; return;}
  }
  //Set all the model parameters to be constant, so only fitting the species yields
  fParameters.setAttribAll("Constant");
  //calculate the sWeights 
  fSPlot = new RooStats::SPlot("splot_fit", "sPlot Fit", *((RooDataSet*)fData),fModel ,fYields);
  //insert weights in std::map
  if(fTree)ExportTreeWeights();
}
Double_t THSRooFit::GetSWeight(Long64_t event,TString species){
  //Note species should include the prepend fYld
  return fSPlot->GetSWeight(event,species);
}
// void THSRooFit::ExportWeights(TString filename,TString wmname){
//   cout<<"Exporting Weights to "<<filename<<endl;
//   fWMap=new THSWeightMap(wmname);//initialise map
//   Int_t NSpecies=fYields.getSize();
//   VecDouble_t eventW(NSpecies); //initialise weights vector
//   //  eventW.clear();
//   for(Long64_t ev=0;ev<fData->numEntries();ev++){//loop over events
//     for(Int_t iw=0;iw<NSpecies;iw++)//loop over species
//       eventW[iw]=GetSWeight(ev,fYields.at(iw)->GetName());//get weight for this species
//     if(fGotID){//use ID from initial tree
//       const RooArgSet* vars=fData->get(ev);
//       fWMap->SetWeight(dynamic_cast<RooRealVar*>(vars->find(fIDBranchName))->getValV(),eventW);
//       } //ID not defined just use entry number on tree
//     else fWMap->SetWeight(ev,eventW);
//   }
//   if(filename.Contains(".root")){//if valid filename given save map to file
//     TFile* file=new TFile(filename,"recreate");
//     fWMap->Write();
//     file->Close();
//     delete file;
//   }
//   cout<<"Exported Weights "<<fWMap<<endl;
// }
void THSRooFit::ExportTreeWeights(TString wmname){
  cout<<"THSRooFit::ExportTreeWeights "<<fTree->GetName()<<" "<<fTree->GetEntries()<<" "<<fGotID<<endl;
  if(!fTree) {cout<<"THSRooFit::ExportTreeWeights( No tree found"<<endl;return;}
  fWMap=new THSWeightMap(wmname);//initialise map
  Int_t NSpecies=fYields.getSize();
  VecDouble_t eventW(NSpecies); //initialise weights vector
  for(Int_t iw=0;iw<NSpecies;iw++)//set name for each species, remove leading Yld_
    fWMap->SetSpecies(iw,TString(fYields.at(iw)->GetName()).Remove(0,4));
  //  eventW.clear();
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
      fWMap->SetWeight(tid,eventW); //fill the weight for every tree event
    }
    //Below will give problems if not all tree events are in dataset...
    else fWMap->SetWeight(ev,eventW);//no tree id just fill with entry number
  }
  // std::sort(fWMap->GetMap().begin(),fWMap->GetMap().end());
}
void THSRooFit::ExportWeightsToFile(TString filename){
  cout<<"Exporting Weights to "<<filename<<endl;
  if(filename.Contains(".root")){//if valid filename given save map to file
    TFile* file=new TFile(filename,"recreate");
    fWMap->Write();
    file->WriteObject((fWMap->GetMapp()),fWMap->GetName());
    file->WriteObject((fWMap->GetSpeciesp()),fWMap->GetName()+TString("Species"));
    file->Close();
    delete file;
  }
  cout<<"Exported Weights "<<fWMap<<endl;
}
void THSRooFit::AddWeightMap(WeightMap_t wm){
  cout<<"THSRooFit::AddWeightMap Start add weights "<<wm.size()<<endl;
  if(!fWMap){//construct new map if not existing
    fWMap=new THSWeightMap("WeightMap");
    Int_t NSpecies=fYields.getSize();
    VecDouble_t eventW(NSpecies); //initialise weights vector
    for(Int_t iw=0;iw<NSpecies;iw++)//set name for each species, remove leading Yld_
      fWMap->SetSpecies(iw,TString(fYields.at(iw)->GetName()).Remove(0,4));
  }
  fWMap->Add(wm);
}
void THSRooFit::AddSubWeights(){
  //look through sub fits and add the maps to the total
  //loop over subfits
  for(Int_t i=0;i<fRooFits->GetEntries();i++)
    AddWeightMap(dynamic_cast<THSRooFit*>(fRooFits->At(i))->GetWeights()->GetMap());
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
  if(!fCanvases){fCanvases=new TList();fCanvases->SetOwner();}
  //Loop over variables
  for(Int_t idr=0;idr<fVariables.getSize();idr++){ 
    RooRealVar* var=fWS->var(fVariables[idr].GetName());//get variable
    fCanvases->Add(canvas=new TCanvas(TString(GetName())+fVariables[idr].GetName(),TString(GetName())+fVariables[idr].GetName()));//create new canvas for drawing on
    RooPlot* frame = var->frame(); // RooFit frame 
    fData->plotOn(frame, DataError(RooAbsData::SumW2) ) ; //plot the data
    fModel->plotOn(frame,LineStyle(kDashed), LineColor(kRed)) ; //model = signal + back fit result 
    fModel->paramOn(frame,
		 Label("Global Fit parameters:"),
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
  if(fTree->GetBranch(fIDBranchName))fGotID=kTRUE;
  if(fWMap->GetMap().size()==0) return;
  if(fWMap->GetMap().size()!=(ULong64_t)fTree->GetEntries()) {cout<<"DrawTreeVar : Tree and Map have different entries!"<<" " <<fTree->GetEntries()<<" "<<fWMap->GetMap().size()<<endl;return;}
  Double_t dVar=0;
  if(fTree->SetBranchAddress(VarName,&dVar)) {cout<<"No variable in tree "<<VarName<<endl; return;}
  Double_t tid=0;
  if(fGotID)  fTree->SetBranchAddress(fIDBranchName,&tid);
  vector<TH1F* > hists;
  //create histogram for each species
  Int_t Nspecies=fPDFs.getSize();
  for(Int_t isp=0;isp<Nspecies;isp++){
    hists.push_back(new TH1F(fPDFs[isp].GetName()+VarName+GetName(),VarName+" SWeighted "+fPDFs[isp].GetName(),nbins,hmin,hmax));
    fHists->Add(hists[isp]);
  }
  Long64_t id=0; //can be diffent from event number, and can be filtered in dataset
  Long64_t nds=0;
  if(!fTree->GetEntries()==fWMap->GetMap().size()) {cout<<"Map - Tree size mismatch "<< fTree->GetEntries()<<" "<<fWMap->GetMap().size()<<endl;return;}
  //loop over tree and fill with weight
  for(Long64_t ient=0;ient<fTree->GetEntries();ient++){
    fTree->GetEvent(ient);
    //get the id for the event if not in tree
    if(!fGotID) id=ient;
    else id=(Long64_t)tid; //must use ExportTreeWeights or map will crash when there is no entry in dataset
    for(Int_t isp=0;isp<Nspecies;isp++)//Fill each species
      hists[isp]->Fill(dVar,fWMap->GetWeight((Long64_t)id,isp));
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
  for(Int_t isp=1;isp<fPDFs.getSize();isp++){
    hists[isp]->SetLineColor(isp);
    leg->AddEntry(hists[isp], hists[isp]->GetTitle(), "l");
    hists[isp]->Draw("same");
    if(maxVal<hists[isp]->GetMaximum()) maxVal=hists[isp]->GetMaximum();
  }
  hists[0]->SetMaximum(maxVal+0.05*maxVal);
  leg->Draw();
  //First check the correlations with other variables in tree
  CorrelationWithVars(VarName);
  fTree->ResetBranchAddresses();
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
  //Make sure model created
  if(!fModel)TotalPDF();
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
THSRooFit*  THSRooFit::CreateSubFitBins(TTree* ctree){//events already selected
  cout<<"THSRooFit::CreateSubFitBins with tree "<<ctree->GetName()<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  THSRooFit* RFa=new THSRooFit();
  fRooFits->Add(RFa);
  RFa->SetName(TString(GetName())+ctree->GetName());
  //Make sure model created
  if(!fModel)TotalPDF();
  RFa->LoadWorkSpace(fWS);
  RFa->LoadDataSet(ctree->CopyTree(""));//will use any EntryList
  return RFa;
}

void THSRooFit::SaveHists(TString filename){
  TFile* file=new TFile(filename,"recreate");
  fHists->Write();
  file->Close();
  delete file;
}
void THSRooFit::SavePlots(TString filename){
  TFile* file=new TFile(filename,"recreate");
  fCanvases->Write();
  file->Close();
  delete file;
}
void THSRooFit::MakeBins(){
  fDataBins->InitialiseLists("DataBins",fOutDir+"DataEntries.root");
  fDataBins->RunEntryList(fTree);
  fDataBins->SaveLists();
}
void THSRooFit::MakeTreeBins(){
  fDataBins->InitialiseTrees("DataBins",fOutDir+"DataEntries.root");
  cout<<"THSRooFit::MakeTreeBins() with "<<fVariables[0].GetName()<<endl;
  fDataBins->RunTree(fTree);
  fDataBins->SaveTrees();
}
void THSRooFit::RunWithBins(){
  fWS->Print();
  MakeBins();
  cout<<"THSRooFit::RunWithBins(); number of bins "<<fDataBins->GetN()<<endl;
  TDirectory *saveDir=gDirectory;
  TFile* tfile=new TFile(fOutDir+"DataEntries.root");
  saveDir->cd();
  TObjArray* oa=(TObjArray*)tfile->Get("DataBins");
  for(Int_t i=0;i<fDataBins->GetN();i++){
    //Make fit object for events in this bin
    //retrieve from saved entries file
    //   TFile* tfile=new TFile(fOutDir+"DataEntries.root");
    // TObjArray* oa=(TObjArray*)tfile->Get("DataBins");
    // TEntryList* DataList=dynamic_cast<TEntryList*>(oa->At(i)->Clone());
    // DataList->SetDirectory(0);
    // fTree->SetEntryList(DataList);
    // tfile->Close();
    // delete tfile;
    TEntryList* DataList=dynamic_cast<TEntryList*>(oa->At(i));
    fTree->SetEntryList(DataList);
    // THSRooFit* rf=CreateSubFitBins(TNamed(fDataBins->GetBinName(i),TString("")));
    THSRooFit* rf=CreateSubFitBins(fTree);
 
    rf->TotalPDF();
    rf->Fit();
    rf->PlotDataModel();
    rf->sPlot();
    rf->SavePlots(fOutDir+TString("Plots")+fDataBins->GetBinName(i)+".root");
    AddWeightMap(rf->GetWeights()->GetMap());
    rf->GetWeights()->PrintWeight();
    rf->RemoveDataSet();//save memory
    delete rf;
  }
  cout<<"THSRooFit::RunWithBins() Done all Fits "<<endl;
  tfile->Close();
  delete oa;
  delete tfile;
  
  //AddSubWeights();
  GetWeights()->Print();
  ExportWeightsToFile(fOutDir+TString("Weights")+".root");
}
void THSRooFit::RunWeights(){
  //Do a single sWeights fit
 TotalPDF();
 Fit();
 PlotDataModel();
 sPlot();
 SavePlots(fOutDir+TString("Plots")+".root");
 GetWeights()->PrintWeight();
 ExportWeightsToFile(fOutDir+TString("Weights")+".root");
}

void THSRooFit::RunWithTrees(){
  fWS->Print();
  MakeTreeBins();
  cout<<"THSRooFit::RunWithTrees(); number of bins "<<fDataBins->GetN()<<endl;
  TDirectory *saveDir=gDirectory;
  TFile* tfile=new TFile(fOutDir+"DataEntries.root");
  saveDir->cd();
  TObjArray* oa=(TObjArray*)tfile->Get("DataBins");
  for(Int_t i=0;i<fDataBins->GetN();i++){
    TTree* DataTree=dynamic_cast<TTree*>(oa->At(i));
    THSRooFit* rf=CreateSubFitBins(DataTree);
    rf->TotalPDF();
    rf->Fit();
    rf->PlotDataModel();
    rf->sPlot();
    rf->SavePlots(fOutDir+TString("Plots")+fDataBins->GetBinName(i)+".root");
    AddWeightMap(rf->GetWeights()->GetMap());
    rf->GetWeights()->PrintWeight();
    rf->RemoveDataSet();//save memory
    delete rf;
   }
  cout<<"THSRooFit::RunWithBins() Done all Fits "<<endl;
  //AddSubWeights();
  GetWeights()->PrintWeight();
  ExportWeightsToFile(fOutDir+TString("Weights")+".root");
  tfile->Close();
  delete oa;
  delete tfile;

}
//////////////////////////////////////////////////////////////////
//WeightMap functions
void THSWeightMap::PrintWeight(){
  cout<<"Map "<<GetName()<<" contains "<<Size() <<" events"<<endl;
  cout<<"Species are : "<<endl;
  for(map<TString,Int_t>::iterator it=fSpecies.begin();it!=fSpecies.end();++it)
    cout<<it->first<<endl;
  Int_t Nit=0;
  cout<<"The first ten entries are :"<<endl;
  for(WeightMap_t::iterator itm=fWeights.begin();Nit<10;++itm,Nit++)
    cout<<(itm->first)<<" "<<(itm->second[0])<<endl;
    
}

void THSWeightMap::Multiply(THSWeightMap* other,TString species){
  //if these weights are calculated from fits that already used weights
  //then must mutliply through by previous weights
  if(Size()!=other->Size()) {cout<<"Maps have different number of entries can't multiply!"<<endl; return;}
  //Need to find which species weight we are multiplying by
  Int_t sp=other->GetSpeciesID(species);
  WeightMap_t::iterator it0;
  WeightMap_t::iterator it1=other->GetMapp()->begin();//Note this must be the map pointer!
  Int_t count=0;
  for(it0 = GetMapp()->begin(); it0 != GetMapp()->end(); ++it0) {
    if(it0->first!=it1->first){cout<<"THSWeightMap::Multiply ID mismatch "<< it0->first<<" "<<it1->first<<endl;return;}
    for(vector<Double_t>::iterator itsp=it0->second.begin();itsp!=it0->second.end();++itsp)//multiply each species weight by the selected species in other
      *itsp=(*itsp)*it1->second[sp];
    ++it1;//move the other iterator on
  }
  
}
