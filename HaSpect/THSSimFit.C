#include "THSSimFit.h"
#include </home/dglazier/Dropbox/hsana/Events/Projects/pipi/SPlotMC/THSEventsPDF.h>

ClassImp(THSSimFit);
THSSimFit::THSSimFit() : THSRooFit(){
  fIsBinTrees=kFALSE;
}
void THSSimFit::SetModelEventsTree(TString name,TTree* tree){
  THSEventsPDF* mpdf=dynamic_cast<THSEventsPDF*>(fWS->pdf(name));
   mpdf->SetTree(tree);//set the tree for the MC model
   cout<< "THSRooFit::SetMCModelTree(TString name,TTree* tree) "<<name<<endl;
 
   //if bins are defined split tree up into bins 
   if(fDataBins&&!fIsBinTrees){//use either entrylists
     fDataBins->InitialiseLists("MCModelBins",fOutDir+TString("Bins")+mpdf->GetName()+".root");
     fDataBins->RunEntryList(tree);
     fDataBins->SaveLists();
     
   }
   else if(fDataBins){//or trees
       fDataBins->InitialiseTrees("MCModelBins",fOutDir+TString("Bins")+mpdf->GetName()+".root");
       fDataBins->RunTree(tree);
       fDataBins->SaveTrees();
     }
   else mpdf->AddSmearedModel(0,GetAuxVars());//only one bin, go ahead and make the model, with ranges defined in aux vars
}
void THSSimFit::RunWeights(){
  if(!fDataBins) THSRooFit::RunWeights();
  else{
    if(fIsBinTrees) RunWithTrees();
    else RunWithBins();
  }
}
void THSSimFit::RunWithBins(){
  MakeBins();
  cout<<"THSSimFit::RunWithBins(); number of bins "<<fDataBins->GetN()<<endl;
  TDirectory *saveDir=gDirectory;
  TFile* tfile=new TFile(fOutDir+"DataEntries.root");
  saveDir->cd();
  TObjArray* oa=(TObjArray*)tfile->Get("DataBins");
  for(Int_t i=0;i<fDataBins->GetN();i++){
    TEntryList* DataList=dynamic_cast<TEntryList*>(oa->At(i));
    fTree->SetEntryList(DataList);
    // THSRooFit* rf=CreateSubFitBins(TNamed(fDataBins->GetBinName(i),TString("")));
    THSRooFit* rf=CreateSubFitBins(fTree);
    rf->SetName(fDataBins->GetBinName(i));
    //iterate over models and set their entry lists id THSEventsPdf
    RooAbsPdf* pdf=0;
    for(Int_t ip=0;ip<rf->GetPDFs().getSize();ip++){
      pdf=(RooAbsPdf*)&(rf->GetPDFs()[ip]);
      THSEventsPDF* mpdf=0;
      cout<<"THSSimFit::RunWithBins(); PDFs "<<pdf->GetName()<<endl;
      if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){//Only applies to THSEventsPDF
      	TFile* mfile=new TFile(fOutDir+TString("Bins")+mpdf->GetName()+".root");
      	TObjArray* oam=(TObjArray*)mfile->Get("MCModelBins");
      	//note the cloned entry list is deleted in THSEventsPDF
       	mpdf->SetTree(dynamic_cast<THSEventsPDF*>(fPDFs.find(mpdf->GetName()))->GetTree());
      	TEntryList* MCList=dynamic_cast<TEntryList*>(oam->At(i)->Clone());
      	MCList->SetDirectory(0);
      	mpdf->SetEntryList(MCList);
      	mfile->Close();
	delete oam;
      	delete mfile;
      	//COULD send it to the farm here
      	mpdf->AddSmearedModel(0,rf->GetAuxVars());
      }
    }
     
    if(!rf->GetModel()) rf->TotalPDF();
    rf->Fit();
    rf->PlotDataModel();
    rf->sPlot();
    rf->SavePlots(fOutDir+TString("Plots")+fDataBins->GetBinName(i)+".root");
    AddWeightMap(rf->GetWeights()->GetMap());
    rf->GetWeights()->PrintWeight();
    rf->RemoveDataSet();//save memory
    delete rf;
  }
   tfile->Close();
   delete oa;
   delete tfile;

  cout<<"THSRooFit::RunWithBins() Done all Fits "<<endl;
  // AddSubWeights();
  GetWeights()->PrintWeight();
  ExportWeightsToFile(fOutDir+TString("Weights")+".root");
}
void THSSimFit::RunWithTrees(){
  fWS->Print();
  cout<<"THSRooFit::RunWithBins(); number of bins "<<fDataBins->GetN()<<endl;
  MakeTreeBins();
  TFile* tfile=new TFile(fOutDir+"DataEntries.root");
  TObjArray* oa=(TObjArray*)tfile->Get("DataBins");
  for(Int_t i=0;i<fDataBins->GetN();i++){
    TTree* DataTree=dynamic_cast<TTree*>(oa->At(i));
    THSRooFit* rf=CreateSubFitBins(DataTree);
    //Make fit object for events in this bin
    //retrieve from saved entries file
    RooAbsPdf* pdf=0;
    //    while(pdf=(RooAbsPdf*)itPdf->Next()){
    TTree* MCTree=0;
    for(Int_t ip=0;ip<rf->GetPDFs().getSize();ip++){
      pdf=(RooAbsPdf*)&(rf->GetPDFs()[ip]);
      THSEventsPDF* mpdf=0;
      cout<<"THSRooFit::RunWithBins(); PDFs "<<pdf->GetName()<<endl;
      if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){//Only applies to THSEventsPDF
	TFile* mfile=new TFile(fOutDir+TString("Bins")+mpdf->GetName()+".root");
	TObjArray* oam=(TObjArray*)mfile->Get("MCModelBins");
	cout<<mfile<<" "<<oam<<endl;
	//note the cloned entry list is deleted in THSEventsPDF
	MCTree=dynamic_cast<TTree*>(oam->At(i)->Clone());
	MCTree->SetDirectory(0);
	//mpdf->SetEntryList(MCList);
	mpdf->SetTree(MCTree);
	mfile->Close();
	delete oam;
	delete mfile;
	cout<<MCTree<<" "<<mpdf<<" "<<rf->GetAuxVars()<<endl;
	//COULD send it to the farm here
	mpdf->AddSmearedModel(0,rf->GetAuxVars());
	//	mpdf->SetVarRange(TString(fExpLimits[il].GetName()).Remove(0,2))->setRange(fWS->var(fExpLimits[il].GetName())->getMin(),fWS->var(fExpLimits[il].GetName())->getMax());
	delete MCTree;
      }
    }
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
   tfile->Close();
   delete oa;
   delete tfile;

  cout<<"THSRooFit::RunWithBins() Done all Fits "<<endl;
  //AddSubWeights();
  GetWeights()->Print();
  ExportWeightsToFile(fOutDir+TString("Weights")+".root");
}
