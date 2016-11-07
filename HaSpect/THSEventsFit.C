#include "THSEventsFit.h"
#include "THSEventsPDF.h"

ClassImp(THSEventsFit);
THSEventsFit::THSEventsFit() : THSsPlot(){
  fAccFrac=0.0005;
  fWithConstraints=kFALSE;
 }
void THSEventsFit::SetModelEventsTree(TString name,TTree* tree){
  //Note fit varibale not added to bins here as we want a wider ranges
  //for our PDF in case of offsets or scaling
  THSEventsPDF* mpdf=dynamic_cast<THSEventsPDF*>(fWS->pdf(name));
   mpdf->SetTree(tree);//set the tree for the MC model
   cout<< "THSEventsFit::SetMCModelTree(TString name,TTree* tree) "<<name<<" "<<GetOutDir()<<endl;

   //if bins are defined split tree up into bins 
   if(fDataBins){//use either entrylists
     cout<<"SetModelEventsTree "<<fDataBins<<" "<<fOutDir<< " "<<mpdf->GetName()<<endl;
      fDataBins->InitialiseBinTree("MCModelBins",fOutDir+TString("Bins")+mpdf->GetName()+".root");
      fDataBins->RunBinTree(tree);
      fDataBins->Save();
   }

   //Add gaussian constraint for alpha parameter
   if(fWithConstraints){
     cout<<"Adding Constraints void THSEventsFit::SetModelEventsTree(TString name,TTree* tree) "<<fWithConstraints<<endl;
     AddGausConstraint(mpdf->AlphaConstraint());
     AddGausConstraint(mpdf->OffConstraint());
     AddGausConstraint(mpdf->ScaleConstraint());
   }
}
void THSEventsFit::RunWeights(Int_t Nbins){
  if(!fDataBins) {cout<<"Error : Need to define at least 1 bin for this class e.g. RF->LoadBinVars(\"Eg\",1,3,4);"<<endl; exit(0);}
  DefineSets();
  MakeBins();
  cout<<"THSEventsFit::RunWithBins(); number of bins "<<fDataBins->GetN()<<endl;
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
    Long64_t PdfN[GetPDFs().getSize()];
    for(Int_t ipdf=0;ipdf<GetPDFs().getSize();ipdf++)  PdfN[ipdf]=0;
    Long64_t TotalN=0;
    THSsPlot* rf=static_cast<THSsPlot*>(CreateSubFitBins(savedBins->GetBinnedTree(fTree,i),kFALSE));
    //iterate over models and set their entry lists id THSEventsPdf
    RooAbsPdf* pdf=0;
    for(Int_t ip=0;ip<rf->GetPDFs().getSize();ip++){
      pdf=(RooAbsPdf*)&(rf->GetPDFs()[ip]);
      THSEventsPDF* mpdf=0;
      cout<<"THSEventsFit::RunWithBins(); PDFs "<<pdf->GetName()<<endl;
      if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){//Only applies to THSEventsPDF
	THSBins* savedMCBins=new THSBins("MCModelBins",fOutDir+TString("Bins")+mpdf->GetName()+".root");
  	mpdf->SetTree(savedMCBins->GetBinnedTree(dynamic_cast<THSEventsPDF*>(fPDFs.find(mpdf->GetName()))->GetTree(),i));
	delete savedMCBins;
       	PdfN[ip]=mpdf->AddSmearedModel(0,rf->GetAuxVars());
	cout<<"CHCK "<<PdfN[ip]<<" "<<TotalN<<endl;
	TotalN+=PdfN[ip];
	cout<<"CHCK "<<PdfN[ip]<<" "<<TotalN<<endl;
	if(PdfN[ip]<10) {
	  cout<<"THSEventsFitfrom::RunWithBins() no events found for "<<fDataBins->GetBinName(i)<<" MODEL: "<<pdf->GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
	  rf->GetPDFsp()->remove(rf->GetPDFs()[ip]);
	  rf->GetYieldsp()->remove(*(rf->GetWorkSpace()->var(fYld+pdf->GetName())));
	  rf->GetWorkSpace()->removeSet("Yields");
	  rf->GetWorkSpace()->removeSet("PDFs");
	  rf->GetWorkSpace()->defineSet("Yields",rf->GetYields());
	  rf->GetWorkSpace()->defineSet("PDFs",rf->GetPDFs());
	}
      }
    }
    for(Int_t ip=0;ip<GetPDFs().getSize();ip++){
 	pdf=(RooAbsPdf*)&(rf->GetPDFs()[ip]);
	THSEventsPDF* mpdf=0;
 	if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){
	  if(Double_t(PdfN[ip])/TotalN<fAccFrac){
	    cout<<" THSEventsFit::InitialiseFit()  "<<GetName()<<" MODEL: "<<GetPDFs()[ip].GetName()<<" N events below acceptance fraction so not considered significant enough background " <<PdfN[ip] <<" out of "<<TotalN<<" "<<fAccFrac<<endl;
	    GetPDFsp()->remove(GetPDFs()[ip]);
	    GetYieldsp()->remove(*(GetWorkSpace()->var(fYld+pdf->GetName())));
	    GetWorkSpace()->removeSet("Yields");
	    GetWorkSpace()->removeSet("PDFs");
	    GetWorkSpace()->defineSet("Yields",GetYields());
	    GetWorkSpace()->defineSet("PDFs",GetPDFs());	

	  }
	}
    }
    if(rf->GetPDFs().getSize()) rf->TotalPDF();
    else {
      cout<<"THSEventsFit::RunWithBins() no model found for "<<fDataBins->GetBinName(i)<<" probably no events kinmatically allowed in this bin" <<endl;
      rf->RemoveDataSet();//save memory
      delete rf;
      continue;
    }
    cout<<fData->numEntries()<<endl;
    if(rf->GetDataSet()->numEntries()<2) {delete rf;continue;}
    rf->FitMany(Nbins);
    rf->sPlot();
    rf->SavePlots("");//save plots for each bin fit
    if(rf->GetWeights()){
      rf->GetWeights()->PrintWeight();
      AddWeightMap(rf->GetWeights());
      rf->GetWeights()->Save();
    }
    rf->RemoveDataSet();//save memory
    delete rf;
  }
  //tfile->Close();
   delete savedBins;
   //delete tfile;
  fTree->SetBranchStatus("*",1);
  cout<<"THSRooFit::RunWithBins() Done all Fits "<<endl;
  // AddSubWeights();
  if(fWeights)GetWeights()->PrintWeight();
  if(fWeights) GetWeights()->SortWeights();
}

void THSEventsFit::PrepareForFarm(){
  if(!fDataBins) {cout<<"Error : Need to define at least 1 bin for this class e.g. RF->LoadBinVars(\"Eg\",1,3,4);"<<endl; exit(0);}
  DefineSets();
  MakeBins();
  cout<<" THSEventsFit::PrepareForFarm(); number of bins "<<fDataBins->GetN()<<endl;
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
     Long64_t PdfN[GetPDFs().getSize()];
     Long64_t TotalN=0;

    THSRooFit* rf=CreateSubFitBins(savedBins->GetBinnedTree(fTree,i),kFALSE);
    cout<<"THSEventsFit::PrepareForFarm() made data for "<<fDataBins->GetBinName(i)<<" with entries =" <<rf->GetDataSet()->numEntries()<<endl;
     //iterate over models and set their entry lists id THSEventsPdf
    RooAbsPdf* pdf=0;
    for(Int_t ip=0;ip<rf->GetPDFs().getSize();ip++){
      pdf=(RooAbsPdf*)&(rf->GetPDFs()[ip]);
      THSEventsPDF* mpdf=0;
      cout<<" THSEventsFit::PrepareForFarm(); PDFs "<<pdf->GetName()<<endl;
      if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){//Only applies to THSEventsPDF

	THSBins* savedMCBins=new THSBins("MCModelBins",fOutDir+TString("Bins")+mpdf->GetName()+".root");
	//Get tree for this bin
  	mpdf->SetTree(savedMCBins->GetBinnedTree(dynamic_cast<THSEventsPDF*>(fPDFs.find(mpdf->GetName()))->GetTree(),i));
      	PdfN[ip]=mpdf->AddSmearedModel(0,rf->GetAuxVars());
	TotalN+=PdfN[ip];
	
	if(PdfN[ip]<10) {
	  cout<<" THSEventsFit::InitialiseFit() no events found for "<<rf->GetName()<<" MODEL: "<<pdf->GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
	  rf->GetPDFsp()->remove(rf->GetPDFs()[ip]);
	  rf->GetYieldsp()->remove(*(rf->GetWorkSpace()->var(fYld+pdf->GetName())));
	  rf->GetWorkSpace()->removeSet("Yields");
	  rf->GetWorkSpace()->removeSet("PDFs");
	  rf->GetWorkSpace()->defineSet("Yields",rf->GetYields());
	  rf->GetWorkSpace()->defineSet("PDFs",rf->GetPDFs());	
	}
	delete savedMCBins;
      }
    }
   for(Int_t ip=0;ip<GetPDFs().getSize();ip++){
 	pdf=(RooAbsPdf*)&(rf->GetPDFs()[ip]);
	THSEventsPDF* mpdf=0;
 	if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){
	  if(Double_t(PdfN[ip])/TotalN<fAccFrac){
	    cout<<" THSEventsFit::InitialiseFit()  "<<GetName()<<" MODEL: "<<GetPDFs()[ip].GetName()<<" N events below acceptance fraction so not considered significant enough background " <<PdfN[ip] <<" out of "<<TotalN<<" "<<fAccFrac<<endl;
	    GetPDFsp()->remove(GetPDFs()[ip]);
	    GetYieldsp()->remove(*(GetWorkSpace()->var(fYld+pdf->GetName())));
	    GetWorkSpace()->removeSet("Yields");
	    GetWorkSpace()->removeSet("PDFs");
	    GetWorkSpace()->defineSet("Yields",GetYields());
	    GetWorkSpace()->defineSet("PDFs",GetPDFs());	
	    
	  }
	}
    }
  
    //also need to import custom PDF class
   rf->GetWorkSpace()->Print();
   //rf->GetWorkSpace()->importClassCode(THSEventsPDF::Class(),kTRUE);
    rf->GetWorkSpace()->writeToFile(fOutDir+TString("Farm")+fDataBins->GetBinName(i)+".root");
    // cout <<" THSEventsFit::PrepareForFarm() Saved Workspace with "<<rf->GetDataSet()->numEntries()<<" for bin "<<fDataBins->GetBinName(i)<<endl;
    rf->RemoveDataSet();//save memory
    delete rf;
  }
  delete savedBins;

  cout<<"THSRooFit::PrepareForFarm() Done all files "<<endl;
 }


Bool_t THSEventsFit::InitialiseFit(){
  if(GetDataSet()->numEntries()<2) {cout<<" THSEventsFit::InitialiseFit() less than 2 entries in dataset!"<<endl; return kFALSE;}
  RooAbsPdf* pdf=0;
  Long64_t PdfN[GetPDFs().getSize()];
  Long64_t TotalN=0;
  for(Int_t ip=0;ip<GetPDFs().getSize();ip++){
    pdf=(RooAbsPdf*)&(GetPDFs()[ip]);
    THSEventsPDF* mpdf=0;
    cout<<" THSEventsFit::InitialiseFit(); PDFs "<<pdf->GetName()<<endl;
    if(mpdf=dynamic_cast<THSEventsPDF*>(pdf)){//Only applies to THSEventsPDF
      if(!(mpdf->GetHistPdf())){
  	cout<<"Start smeared model "<<gDirectory->GetName()<<endl;
	PdfN[ip]=mpdf->AddSmearedModel(0,GetAuxVars());
	cout<<"Stop smeared model "<<gDirectory->GetName()<<endl;
	TotalN+=PdfN[ip];
	if(PdfN[ip]<10) {
	  cout<<" THSEventsFit::InitialiseFit() no events found for "<<GetName()<<" MODEL: "<<pdf->GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
	  GetPDFsp()->remove(GetPDFs()[ip]);
	  GetYieldsp()->remove(*(GetWorkSpace()->var(fYld+pdf->GetName())));
	  GetWorkSpace()->removeSet("Yields");
	  GetWorkSpace()->removeSet("PDFs");
	  GetWorkSpace()->defineSet("Yields",GetYields());
	  GetWorkSpace()->defineSet("PDFs",GetPDFs());	
	}
      }
    }
  }
  for(Int_t ip=0;ip<GetPDFs().getSize();ip++){
    if(PdfN[ip]/TotalN<fAccFrac){
      //  cout<<" THSEventsFit::InitialiseFit()  "<<GetName()<<" MODEL: "<<GetPDFs()[ip]->GetName()<<" N events below acceptance fraction so not considered significant enough background " <<PdfN[ip] <<" out of "<<TotalN<<endl;
      GetPDFsp()->remove(GetPDFs()[ip]);
      GetYieldsp()->remove(*(GetWorkSpace()->var(fYld+pdf->GetName())));
      GetWorkSpace()->removeSet("Yields");
      GetWorkSpace()->removeSet("PDFs");
      GetWorkSpace()->defineSet("Yields",GetYields());
      GetWorkSpace()->defineSet("PDFs",GetPDFs());	
      
    }
    
  }
  if(GetPDFs().getSize()) TotalPDF();
  else {
    cout<<" THSEventsFit::InitialiseFit() no model found for "<<GetName()<<" probably no events kinmatically allowed in this bin" <<endl;
    return kFALSE;
  }
  return kTRUE;
  
}

