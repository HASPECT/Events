//--Author      DI Glazier 30/06/2014
//--Rev
//--Update
//--Description
//HASPECT Event Reconstruction
//THSHisto
//Facilitates histogram declaration and filling
//Users must implement :
//HistogramList to declare which types of histogram they need
//FillHistogram to fill the histogram with datamembers of their selector class
//If the histogram fHisbins is defined it will be used to split
//the histograms into different (kinematic) bins

#define THSHisto_cxx

#include "THSHisto.h"


void THSHisto::LoadCut(TString sCut){
  //Declare the cuts you want to use, just takes a string name
  //the user is responsible for implementing the cut themselves
  //via a call to FillHistograms(sCut,bin) in an apporpriate place
  cout<<"Adding Cut "<<sCut<<endl;
  fCurrCut=sCut; //keep the cut name for use in MapHist()
  fMapCuts[sCut]=fVecCuts.size();
  fVecCuts.push_back(sCut); //Enter the cut into the map
}

void THSHisto::LoadHistograms(){
  //Loads histograms for each defined bin and cut
  //The histograms required should be given in the users HistogramList Function
  //Note HistogramList should give the histograms to fOutput, which will take
  //responsibility for deleting them as SetOwner() is called in its contructor
  //GetKinematic bins
  Int_t Nbins=0;
  if(fHisbins){
    if(TString(fHisbins->ClassName()).Contains("1"))Nbins=(fHisbins->GetNbinsX()+2);//+2 for overflow and underflow;
    else if(TString(fHisbins->ClassName()).Contains("2"))Nbins=(fHisbins->GetNbinsX()+2)*(fHisbins->GetNbinsY()+2);//+2 for overflow and underflow;
    else if(TString(fHisbins->ClassName()).Contains("3"))Nbins=(fHisbins->GetNbinsX()+2)*(fHisbins->GetNbinsY()+2)*(fHisbins->GetNbinsZ()+2);//+2 for overflow and underflow;
  }
  
  Int_t ncut=0;
  if(!Nbins){  //Only 1 bin
    //iterate over cuts
    for(vector< TString >::iterator nextcut=fVecCuts.begin();nextcut!=fVecCuts.end();++nextcut){
      fVecBinNames.resize(1);
      fVecBinNames[0]=TString("_All");
      //Call the users Histogram list for each declared cut
      HistogramList(fVecCuts[ncut++] + fVecBinNames[0]); //cut + bin gets added to histogram name 
      fVecHistCut.push_back(fHistNameMap); //enter the histograms for this cut into a vector element
      fHistNameMap.clear(); //clear the previous cut histograms
    }
    fVecHistBin.push_back(fVecHistCut);//Add all the histograms for this kin bin
    fVecHistCut.clear(); //clear the previous bin histograms  
  }
  //ifmultiple kinematic bins defined make a histogram for each bin
  TString sBin;
  for(Int_t ib=0;ib<Nbins;ib++){
    sBin.Form("_Bin%d",ib); //make bin label
    //Load all the cuts for this bin
    ncut=0;
    //iterate over cuts
    for(vector< TString >::iterator nextcut=fVecCuts.begin();nextcut!=fVecCuts.end();++nextcut){
      //Call the users Histogram list for each declared cut
      // HistogramList(fVecCuts[ncut++]+sBin); //construct all histograms for this bin, note for ChangeNames to work _Bin%d must be the last part of the name
      HistogramList(fVecCuts[ncut++]+fVecBinNames[ib]); //construct all histograms for this bin,
      fVecHistCut.push_back(fHistNameMap); //enter the histograms for this cut into a vector element
      fHistNameMap.clear(); //clear the previous cut histograms
    }
    fVecHistBin.push_back(fVecHistCut);//Add all the histograms for this kin bin
    fVecHistCut.clear(); //clear the previous bin histograms
  }
}

TH1* THSHisto::MapHist(TH1* hist){
  //Place histograms in map for quick look up via their name
  //This should be called in HistogramList and returns the histogram to add to the slector output
  fHistNameMap[TString(hist->GetName())]=hist;
  return hist;
}
Int_t THSHisto::GetKinBin(Float_t ev1,Float_t ev2,Float_t ev3){
  //Function that finds the bin number and converts it to a string
  Int_t evBin=0;
  if(fHisbins)evBin=fHisbins->FindBin(ev1,ev2,ev3); //find bin
  return evBin;
}
TH1* THSHisto::FindHist(TString hname){
  //function that finds the correct histogram for the user to fill
  //requires the current cut and bin be set beforehand, usually in FillHistograms
  return fVecHistBin[fCurrBin][fMapCuts[fCurrCut]][hname];
}
TString THSHisto::GetStrBin(Float_t ev1,Float_t ev2){
  //Function that finds the bin number and converts it to a string
  Int_t evBin=0;
  if(fHisbins)evBin=fHisbins->FindBin(ev1,ev2); //find bin
  TString sBin;
  sBin.Form("_Bin%d",evBin); //make label
  return sBin;
}
void THSHisto::ChangeNames(/*TFile* list*/){
  //make a map from fHisbins containing name tags
  if(!fHisbins) return;
  //We got a KinBins histogram, now make the name map
  if(fBinMap.empty()){
    UInt_t Naxis;
    if(TString(fHisbins->ClassName()).Contains("1"))Naxis=1;
    else if(TString(fHisbins->ClassName()).Contains("2"))Naxis=2;
    else if(TString(fHisbins->ClassName()).Contains("3"))Naxis=3;
    else {cout<<"THSHisto::ChangeNames() not defined behaviour for hist >TH3"<<endl; return;}
    Int_t Nbins=0;
    if(fHisbins){
      if(TString(fHisbins->ClassName()).Contains("1"))Nbins=(fHisbins->GetNbinsX()+2);//+2 for overflow and underflow;
      else if(TString(fHisbins->ClassName()).Contains("2"))Nbins=(fHisbins->GetNbinsX()+2)*(fHisbins->GetNbinsY()+2);//+2 for overflow and underflow;
      else if(TString(fHisbins->ClassName()).Contains("3"))Nbins=(fHisbins->GetNbinsX()+2)*(fHisbins->GetNbinsY()+2)*(fHisbins->GetNbinsZ()+2);//+2 for overflow and underflow;
    }
    
    TString tagname1,tagname2,tagname3,numb,binstr;
    UInt_t xyz=0;
    //note +2 for overfllow+ underflow
    fVecBinNames.resize(Nbins);
    for(Int_t xi=0;xi<fHisbins->GetXaxis()->GetNbins()+2;xi++){
      tagname1=fHisbins->GetXaxis()->GetTitle();
      numb.Form("%1.2f_",fHisbins->GetXaxis()->GetBinCenter(xi));
      tagname1+=numb;
      if(Naxis==1){
	binstr.Form("_Bin%d",xi);
	//	fBinMap[binstr]=tagname1;
	fVecBinNames[xi]=(tagname1);
	continue;
      }
      for(Int_t yi=0;yi<fHisbins->GetYaxis()->GetNbins()+2;yi++){
	tagname2=fHisbins->GetYaxis()->GetTitle();
	numb.Form("%1.2f_",fHisbins->GetYaxis()->GetBinCenter(yi));
	tagname2+=numb;
	if(Naxis==2){
	  xyz=fHisbins->GetBin(xi,yi);
	  binstr.Form("Bin%d",xyz);
	  //fBinMap[binstr]=tagname1+tagname2;
	  fVecBinNames[xyz]=(tagname1+tagname2);
	  continue;
	}
	for(Int_t zi=0;zi<fHisbins->GetZaxis()->GetNbins()+2;zi++){
	  tagname3=fHisbins->GetZaxis()->GetTitle();
	  numb.Form("%1.2f_",fHisbins->GetZaxis()->GetBinCenter(zi));
	  tagname3+=numb;
	 if(Naxis==2){
	   xyz=fHisbins->GetBin(xi,yi,zi);
	   binstr.Form("Bin%d",xyz);
	   //fBinMap[binstr]=tagname1+tagname2+tagname3;
	   fVecBinNames[xyz]=(tagname1+tagname2+tagname3);
	   continue;
	 } 
	}
      }
    }
  }
  
}
/* void THSHisto::DrawAll(){ */
/*   //one canvas per histogram */
/*   //one pad per bin (max 8 per canvas) */
/*   //all cuts drawn on same plot */

/*   //fVecHistBin[fCurrBin][fMapCuts[fCurrCut]][hname]; */

/* } */
