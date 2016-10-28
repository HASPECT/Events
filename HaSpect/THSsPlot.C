#include "THSsPlot.h"
#include <TH1.h>
#include <TH2.h>
#include <TLegend.h>
#include <algorithm>      // std::sort

ClassImp(THSsPlot);


THSsPlot::THSsPlot() : THSRooFit(),fSPlot(0),fWeights(0){
  fSRange[0]=0;fSRange[1]=0; 
  AddFitOption(RooFit::Extended());
};

THSsPlot::THSsPlot(TString name) : THSRooFit(name),fSPlot(0),fWeights(0){
  fSRange[0]=0;fSRange[1]=0; 
  AddFitOption(RooFit::Extended());

}
THSsPlot::THSsPlot(THSsPlot* rf){
   THSRooFit(dynamic_cast<THSRooFit*>(rf));
   //copy constructor, but do not copy the data tree, load that explicitly
 
}

THSsPlot::~THSsPlot(){

  if(fWeights) delete fWeights;
  if(fSPlot) delete fSPlot;
 }

void THSsPlot::MergeModelSpecies(){
  //Functon to combine best fit pdfs into single PDF 
  //the allows sPlot fit to just be dependent on 2 yields
  //Where one is defined by fSingleSp and the other is the 
  //sum of the rest 
  if(!fWS->pdf(fSingleSp)) {cout<<"THSsPlot::MergeModelSpecies() single Species not found "<<fSingleSp<<endl;}
  RooArgList yields=fYields;
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
      cout<<"THSsPlot::MergeModelSpecies() chenge rangeo of "<<((RooRealVar*)&yields[iy])->GetName()<<" "<<((RooRealVar*)&yields[iy])->getVal()<<" by "<<inte2/inte1<<" to "<<yd<<endl;
      ((RooRealVar*)&yields[iy])->setVal(yd);
      iy++;
    }	
    //change yield of single pdf too
    Double_t inte1=sPdf->createIntegral(fVariables)->getVal();
    Double_t inte2=sPdf->createIntegral(fVariables,RooFit::Range("sRange"))->getVal();
    Double_t yd=sYield->getVal();
    yd=yd*inte2/inte1;
    cout<<"THSsPlot::MergeModelSpecies() chenge rangeo of "<<sYield->GetName()<<" "<<sYield->getVal()<<" by "<<inte2/inte1<<" to "<<yd<<endl;
  
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
    //cout<<iy<<"THSsPlot::MergeModelSpecies() CHECKING COEF "<<((RooRealVar*)&yields[iy])->getVal()<<endl;	
  }
  cout<<"Ym "<< Ym<<endl;
  RooRealVar* ymerge=(RooRealVar*)fWS->factory(fYld+"Merged[0,0,1]");
  (ymerge)->setMin(0,0);
  (ymerge)->setMax(0,2*Ym);
  (ymerge)->setVal(Ym);
  fYields.add(*ymerge);
  ymerge->Print();
  cout<<"THSsPlot::MergeModelSpecies() Print pdfs and yeilds"<<endl;
  fPDFs.Print();
  fYields.Print();
  TotalPDF();
 
}
void THSsPlot::sPlot(){
  if(fTree)
    cout<<"THSsPlot::sPlot() weights will be synchronised to branch "<<fIDBranchName<<" "<<fTree<<" "<<fTree->GetBranch(fIDBranchName)<<endl;
  //Perform fit of yields only to get sWeights
  //Check if there is an ID branch for synchronisation
  //check if ID branch exists
  if(fTree){
    if(fTree->GetBranch(fIDBranchName)){
      fGotID=kTRUE;
      cout<<"THSsPlot::sPlot Loading ID branch data "<<fIDBranchName<<endl;
     }
    else {cout<<"THSsPlot::sPlot() warning No ID branch called will just use tree entry number. This means weights will fail to synchronise with initial tree if you cut events or use bins"<<fIDBranchName<<endl;}
  }
  //Option here to sum background contributions and fix relative yields
  if(fSingleSp!=TString("")) MergeModelSpecies();
   //Set all the model parameters to be constant, so only fitting the species yields
  //calculate the sWeights 
  fModel->Print();
  fYields.Print();
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
    SetDataWeight();//if weighted data reassign the weight
    fFiti++;
    PlotDataModel();
    ((TCanvas*)fCanvases->Last())->SetTitle(Form("%s Model on new Range = %d",GetName(),fChi2));
    
  }
  //  if(fBinnedFit) {fBinnedFit=kFALSE;Fit();fBinnedFit=kTRUE;}//if doing multiple binned fits do final MLL for splot
  // PlotDataModel();
  //((TCanvas*)fCanvases->Last())->SetTitle(Form("%s Final Likelihood Fit  = %d",GetName(),fChi2));
  // if(fData->isNonPoissonWeighted()){
  //   Int_t removeY=0;
  //   for(Int_t iy=0;iy<fYields.getSize();iy++){//remove yields below 1 entry in dataset
  //     if(fWS->var(fYields[iy].GetName())->getVal()<=1) {
  // 	//	fWS->var(fYields[iy].GetName())->setVal(1);
  // 	//	fWS->var(fYields[iy].GetName())->setConstant();
  // 	GetPDFsp()->remove(GetPDFs()[iy]);
  // 	GetYieldsp()->remove(fYields[iy]);
  // 	removeY++;
  //     }
  //   }
  //   if(removeY){
  //     GetWorkSpace()->removeSet("Yields");
  //     GetWorkSpace()->removeSet("PDFs");
  //     GetWorkSpace()->defineSet("Yields",GetYields());
  //     GetWorkSpace()->defineSet("PDFs",GetPDFs());
  //     //delete fModel;
  //     fModel->SetName("OldModel");
  //     TotalPDF();
  //     fParameters.setAttribAll("Constant");	
  //     Fit();
  //   }
  // }
  cout<<"THSsPlot::sPlot() about to start"<<endl;
  fData->Print();
  if(fData->sumEntries()<1){cout<<"Warning : void THSsPlot::sPlot() negative data entries no sPlot"<<endl;return ;}//might be -ve with weights, in which case it may crash...also <1
  fParameters.setAttribAll("Constant");
  fSPlot = new RooStats::SPlot("splot_fit", "sPlot Fit", *((RooDataSet*)fData),fModel ,fYields);
  //Check that the fit was succesfull
  Double_t TotalYield=0;
  for(Int_t iy=0;iy<fYields.getSize();iy++)
    TotalYield+=((RooRealVar*)&fYields[iy])->getVal();

  fFiti++;
  fData->Print();
 
  cout<<"void THSsPlot::sPlot() Total Yield after fit "<<TotalYield<<endl;
  if(TotalYield>0){ //got some weights
    fWeights=new THSWeights("HSsWeights");//initialise weights
    fWeights->SetTitle(GetName());
    fWeights->SetFile(fOutDir+TString("Weights")+GetName()+".root");
    // if(fTree)ExportTreeWeights();
    //else ExportWeights();
    ExportWeights();
    fData->Print();
    PlotDataModel();
    ((TCanvas*)fCanvases->Last())->SetTitle(Form("%s sWeights Fit chi2 = %d",GetName(),fChi2));
  }
  else cout<<"Warning :void THSsPlot::sPlot() total weights 0, fit did not converge. Make sure the non-sweight fit to fix parameters was succesful. No weights will be assigned for these events"<<endl;
  
  //Splot has removed any original weights from the dataset
  //For plotting we need to merge them again.
  // SetDataWeight();

}
Double_t THSsPlot::GetSWeight(Long64_t event,TString species){
  //Note species should include the prepend fYld
   return fSPlot->GetSWeight(event,species);
}
void THSsPlot::ExportWeights(TString wmname){
  cout<<"THSsPlot:ExportWeights "<<endl;
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
void THSsPlot::ExportTreeWeights(TString wmname){
  //This function would give a weight for each entry in the tree
  //=0 if event not in fit (outside llimits)
  //as such the weight tree could be used with TTree::AddFriend
  cout<<"THSsPlot::ExportTreeWeights "<<fTree->GetName()<<" "<<fTree->GetEntries()<<" "<<fGotID<<endl;
  if(!fTree) {cout<<"THSsPlot::ExportTreeWeights( No tree found"<<endl;return;}
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
void THSsPlot::ExportWeightsToFile(TString filename){
  cout<<"THSsPlot::ExportWeightsToFile Exporting Weights to "<<filename<<endl;
  if(fWeights->GetWeightList())fWeights->SortWeights();//if merged from differen  
   fWeights->Save();
 }
void THSsPlot::AddWeightMap(THSWeights *Wts){
  cout<<"THSsPlot::AddWeightMap Start add weights "<<Wts->Size()<<endl;
  if(!fWeights){//construct new map if not existing
    cout<<"Construct new weightmap"<<endl;
    fWeights=new THSWeights("WeightMap");
    fWeights->SetTitle(GetName());
    fWeights->SetFile(fOutDir+TString("Weights")+GetName()+".root");
  }
  fWeights->Add(Wts);
}
void THSsPlot::AddSubWeights(){
  //look through sub fits and add the maps to the total
  //loop over subfits
  if(!fWeights){//construct new map if not existing
    fWeights=new THSWeights("WeightMap");
    fWeights->SetTitle(GetName());
    //fWeights->SetFile(fOutDir+TString("Weights")+GetName()+".root");
  }
  for(Int_t i=0;i<fRooFits->GetEntries();i++)
    fWeights->Add(dynamic_cast<THSsPlot*>(fRooFits->At(i))->GetWeights());
}

void THSsPlot::DrawTreeVar(TString VarName,Int_t nbins,Double_t hmin,Double_t hmax){
  cout<<"THSsPlot::DrawTreeVar "<<VarName<<" Check weights "<<endl;

  if(fTree->GetBranch(fIDBranchName))fGotID=kTRUE;
  if(fWeights->GetTree()->GetEntries()!=fTree->GetEntries()) {cout<<"DrawTreeVar : Tree and Map have different entries!"<<" " <<fWeights->GetTree()->GetEntries()<<" "<<fTree->GetEntries()<<endl;}
  if(!fWeights->IsSorted()) fWeights->SortWeights();
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
  //loop over tree and fill with weight
  fWeights->SetCurrEntry(0);
  for(Long64_t ient=0;ient<fTree->GetEntries();ient++){
    fTree->GetEvent(ient);
   //get the id for the event if not in tree
    if(!fGotID) id=ient;
    else id=(Long64_t)tid; 
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
  fTree->ResetBranchAddresses();
}
THSRooFit*  THSsPlot::CreateSubFitBins(TTree* ctree,Bool_t CopyTree){//events already selected

  cout<<"THSsPlot::CreateSubFitBins with tree "<<ctree->GetName()<<" "<<CopyTree<<endl;
  //create a fit object for a subset of data either by setting cut
  //or by fTree->SetEntryList prior to calling this function 
  //It will be deleted by this object
  cout<<fOutDir<<endl;
  THSsPlot* RFa=new THSsPlot();
  RFa->SetName(ctree->GetName());
  if(fBinnedFit)RFa->SetBinnedFit();
  RFa->SetSingleSpecies(fSingleSp);
  RFa->SetOutDir(fOutDir);
  RFa->SetSPlotRange(fSRange[0],fSRange[1]);
  RFa->SetInWeights(fInWeights);
  RFa->SetWeightName(fWeightName);
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
void THSsPlot::SaveHists(TString filename){
  TFile* file=new TFile(filename,"recreate");
  fHists->Write();
  file->Close();
  delete file;
}

void THSsPlot::RunWeights(Int_t Nfits){
  
  if(!fDataBins) return RunSingleWeights(Nfits);
  
  DefineSets();
  MakeBins();
  cout<<"THSsPlot::RunWeights(); number of bins "<<fDataBins->GetN()<<endl;
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
    THSsPlot* rf=static_cast<THSsPlot*>(CreateSubFitBins(savedBins->GetBinnedTree(fTree,i),kFALSE));
  
    rf->RunSingleWeights(Nfits);
    //Add sub weights to total weights for this sPlot
    if(rf->GetWeights()){
      rf->GetWeights()->PrintWeight();
      AddWeightMap(rf->GetWeights());
    }
    rf->RemoveDataSet();//save memory
    delete rf;
  }
  cout<<"THSsPlot::RunWeights() Done all Fits "<<endl;
  delete savedBins;
  
  if(fWeights)GetWeights()->PrintWeight();
  if(fWeights)GetWeights()->SortWeights();
  if(fSingleSp!=TString("")) MergeModelSpecies(); //to be consistent with sub fits
}
void THSsPlot::RunSingleWeights(Int_t Nfits){
  //Run this if just 1 bin flr all events and not used LoadBinVar
  //Do a sWeights fit
  //Maybe new fit so construct PDF if not laready loaded
  if(!fWS->set("PDFs"))DefineSets();
  //if(!fModel)TotalPDF();
  if(!InitialiseFit()) return;
  cout<<fModel<<endl;
  //Fit the model to data with all paramters free
  FitMany(Nfits);
  //Fit the model to data with only species yields as free pars
  //calculate weights and import to WeightMap
  sPlot();
  //save any canvases produced
  SavePlots(fOutDir+TString("Plots")+GetName()+".root");
  //save weights to file
  if(GetWeights()){
    GetWeights()->PrintWeight();
    GetWeights()->SortWeights();
    if(fDataBins)GetWeights()->Save();//don't save if single bin so we can draw
  }
  // 
}


Bool_t THSsPlot::InitialiseFit(){
  cout<<"Initialise "<<GetDataSet()<<" "<<GetDataSet()->numEntries()<<" "<<GetPDFs().getSize()<<endl;
  if(GetDataSet()->numEntries()<2) {return kFALSE;}
  if(GetPDFs().getSize()) TotalPDF();
  else {
    cout<<" THSsPlot::InitialiseFit() no model found for "<<GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
    return kFALSE;
  }
  return kTRUE;
    
}
