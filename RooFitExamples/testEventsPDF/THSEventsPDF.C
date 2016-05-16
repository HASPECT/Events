#include "Riostream.h" 

#include "THSEventsPDF.h" 
#include "RooAbsReal.h" 
#include "RooAbsCategory.h" 
#include "RooBinning.h"
#include <math.h> 
#include "TMath.h" 
#include "TF2.h" 
#include "TVectorD.h" 
#include "TChain.h" 
#include "TEntryList.h" 
#include "TRandom3.h" 
#include "RooArgList.h"

ClassImp(THSEventsPDF)

THSEventsPDF::THSEventsPDF(const char *name, const char *title, 
			   RooAbsReal& _x,
			   RooAbsReal& _alpha,
			   RooAbsReal& _offset,
			   RooAbsReal& _scale,
			   Int_t NBins ,
			   Int_t NAlphBins  ) :
  RooAbsPdf(name,title), 
  x("x","x",this,_x),
  offset("offset","offset",this,_offset),
  scale("scale","scale",this,_scale),
  alpha("alpha","alpha",this,_alpha),
  fx_off(0),
  falpha(0),
  fHist(0),
  fHistPdf(0),
  fRHist(0),
  fWeightHist(0),
  fTree(0)
{ 
  
  RooRealVar *rx=dynamic_cast<RooRealVar*>(&_x);
  RooRealVar *ra=dynamic_cast<RooRealVar*>(&_alpha);
  RooRealVar *rs=dynamic_cast<RooRealVar*>(&_scale);
  RooRealVar *ro=dynamic_cast<RooRealVar*>(&_offset);
  //Work out number of bins for x axis
  //Take bin width as being 1/10 of the alpha range (arbitrary!) or 100 whichever is larger
  // NbinX=40;
  Double_t rsmin=1;
  fOldScale=rs->getVal();
  if(rs->getMin()) rsmin=rs->getMin();
  else cout<<"THSEventsPDF::THSEventsPDF Warning no scale minimum set take = 1"<<endl;
  Double_t mid=(rx->getMax()+rx->getMin())/2;
  Double_t diff=(rx->getMax()-rx->getMin())/2;
  Double_t rMin=mid-diff/rsmin + ro->getMin(); //additional range for possible tranformation or scaling
  Double_t rMax=mid+diff/rsmin + ro->getMax();
  Int_t NbinX=NBins/rsmin;
  cout<<GetName()<<" THSEventsPDF::THSEventsPDF  hist ranges "<<rMin<<" to "<<rMax<<endl;
  if(NbinX<10) NbinX=10;
   fRHist=new TH2F(TString("hmc_model_")+_x.GetName()+name,TString("MC model for ")+_x.GetName(),NbinX,rMin,rMax,NAlphBins,ra->getMin(),ra->getMax());
  fRHist->Sumw2();
  //  fRHist=new TH2F(TString("hmc_model_")+_x.GetName()+name,TString("MC model for ")+_x.GetName(),NbinX,rx->getMin(),rx->getMax(),NAlphBins,ra->getMin(),ra->getMax());
  fx=new RooRealVar(_x.GetName(),"Vx",mid,x.min(),x.max());
  fx_off=new RooRealVar(_x.GetName(),"Vx_off",mid,rMin,rMax);
  falpha=new RooRealVar("Valpha","Valpha",0,alpha.min(),alpha.max());
  
  fNWdim=0;
} 


THSEventsPDF::THSEventsPDF(const THSEventsPDF& other, const char* name) :  
  RooAbsPdf(other,name), 
  x("x",this,other.x),
  offset("offset",this,other.offset),
  scale("scale",this,other.scale),
  alpha("alpha",this,other.alpha),
  fx_off(0),
  fx(0),
  falpha(0),
  fHist(0),
  fHistPdf(0),
  fRHist(0),
  fWeightHist(0),
  fTree(0)
{ 
  
  // cout<<"Copy contructor "<<fHistPdf<<" "<<other.fHistPdf<<endl;
  if(other.fx_off)fx_off=(RooRealVar*)other.fx_off->Clone();
  if(other.fx)fx=(RooRealVar*)other.fx->Clone();
  if(other.falpha)falpha=(RooRealVar*)other.falpha->Clone();
  if(other.fHist)fHist=(RooDataHist*)other.fHist->Clone();
  if(other.fHistPdf)fHistPdf=(RooHistPdf*)other.fHistPdf->Clone();
  if(other.fRHist)fRHist=(TH2F*)other.fRHist->Clone();
  fMean=other.fMean;
  fOldScale=other.fOldScale;
  fNWdim=other.fNWdim;
  if(other.fWeightHist)fWeightHist=(TH1*)other.fWeightHist->Clone();
  //NB. if statements needed for workspace factory creation
  //   if(fHist)fHist->Print();
} 

THSEventsPDF::~THSEventsPDF() {
  if(fHist)delete fHist; 
  if(fRHist)delete fRHist; 
  if(fx_off) delete fx_off;
  if(fx) delete fx;
  if(falpha)delete falpha;
  if(fHistPdf) delete fHistPdf;
  if(fWeightHist) delete fWeightHist;
}

Double_t THSEventsPDF::evaluate() const 
{ 
  if(!fHist) return 1;//needed in case pdf evalualed before model loaded
  //   Double_t arg=x-offset;
  Double_t arg=(x-fMean)*scale+fMean;
  
  // Double_t arg=(x)*scale;
  arg=arg-offset;
  fx_off->setVal(arg);
  falpha->setVal(Double_t(alpha));
  // return  fHist->weight(RooArgSet(*fx_off,*falpha),1,kTRUE)*TMath::Gaus(offset,0,(offset.max()-offset.min())*0.2); //Guassian prior with width  1/5*range 
  return  fHist->weight(RooArgSet(*fx_off,*falpha),1,kTRUE);//*TMath::Gaus(scale,1.,1);
 // return  fHist->weight(RooArgSet(*fx_off,*falpha),1,kTRUE)*TMath::Gaus(Double_t(alpha),0,alpha.max()/2);//alpha prior with width half the range
} 

Long64_t THSEventsPDF::AddSmearedModel(TTree* tree,RooArgList vars){
  //Construcnt a RooDataHist with MC data
  //y axis incorporates systematic uncertaintly on width of distribution
  //tree contains the MC events, 
  //vars provides all the variables that need to be cut to get same kinematics as data
  if(!tree) tree=fTree; //tree presumably aready set
  TH2F* temph=(TH2F*)fRHist->Clone("mctemp");
  temph->SetDirectory(0);
  //Loop over vars attaching branch values to brVal
  TVectorD brVal(vars.getSize());
  tree->SetBranchStatus("*","0");
  for(Int_t iv=0;iv<vars.getSize();iv++){
    //brVal[iv]=0;
    tree->SetBranchStatus((vars[iv].GetName()),"1");//STATUS must be called before ADDRESS!! see Important remarkse in TChain SetBranchStatus!
    tree->SetBranchAddress((vars[iv].GetName()),&brVal[iv]);
  }
  
  //get the fit variable branch
  Double_t tvar=0;
  tree->SetBranchStatus(fx_off->GetName(),"1");
  tree->SetBranchAddress(fx_off->GetName(),&tvar);
  TEntryList *Elist=0;
  //Loop over tree and create entry list that fits all variable limits
  if(vars.getSize()>0){ 
    Elist=new TEntryList();
    Elist->SetDirectory(0);
    //Loop over chain and create an entry list of events meet the cuts 
    //given in the vars variabels set
    for(Long64_t itr=0;itr<tree->GetEntries();itr++){
      tree->GetEntry(itr);
      //fit variable
      //if(tvar<fx_off->getMin("")||tvar>fx_off->getMax("")) continue;//remove as offset needs events outwith range
      //now other variables which may have been given
      Bool_t pass=kTRUE;
      for(Int_t iv=0;iv<vars.getSize();iv++){
	if(brVal[iv]<((RooRealVar*)&vars[iv])->getMin("")||brVal[iv]>((RooRealVar*)&vars[iv])->getMax("")) pass=kFALSE;}
      if(pass){Elist->Enter(itr,tree);}
    }
    //Set entry list for tree
    tree->SetEntryList(Elist);
  }
  else Elist=tree->GetEntryList();
  Int_t treenum=0;
  if(Elist)cout<<GetName()<<" THSEventsPDF::AddSmearedModel Using an EntryList with "<<Elist->GetN()<<" events "<<endl;
  TChain* chain=0; //need chain for proper entrylist access
  if(dynamic_cast<TChain*>(tree))chain=dynamic_cast<TChain*>(tree);
  
  //check if there is a valid weight histogram
  //this should be extended to other things than TH1s, even a weight branch
  TVectorD wVal(fNWdim);
  if(fWeightHist){
    if(tree->GetBranch(fWeightHist->GetXaxis()->GetTitle())){
      tree->SetBranchStatus(fWeightHist->GetXaxis()->GetTitle(),"1");
      tree->SetBranchAddress(fWeightHist->GetXaxis()->GetTitle(),&wVal[0]);
    }
    else cout<<"THSEventsPDF::AddSmearedModel tree has no branch named "<<fWeightHist->GetYaxis()->GetTitle()<<endl;
    if(fNWdim>1){
      if(tree->GetBranch(fWeightHist->GetYaxis()->GetTitle())){
	tree->SetBranchStatus(fWeightHist->GetYaxis()->GetTitle(),"1");
	tree->SetBranchAddress(fWeightHist->GetYaxis()->GetTitle(),&wVal[1]);
      }
      else cout<<"THSEventsPDF::AddSmearedModel tree has no branch named "<<fWeightHist->GetYaxis()->GetTitle()<<endl;
    }
    if(fNWdim>2){
      if(tree->GetBranch(fWeightHist->GetZaxis()->GetTitle())){
	tree->SetBranchStatus(fWeightHist->GetZaxis()->GetTitle(),"1");
	tree->SetBranchAddress(fWeightHist->GetZaxis()->GetTitle(),&wVal[2]);
      }
      else cout<<"THSEventsPDF::AddSmearedModel tree has no branch named "<<fWeightHist->GetYaxis()->GetTitle()<<endl;
    }
  }
  TTree* FastTree=fTree->CopyTree("");//make sure only loop over valid events for speedi.e. this enforces Elist
  Long64_t NFT=FastTree->GetEntries();
  cout<<GetName()<<" THSEvent final entries "<<NFT<<endl;
  if(FastTree->GetEntries()==0) return 0;
  for(Int_t ia=0;ia<fRHist->GetNbinsY();ia++){ 
    temph->Reset();
    Long64_t treeEntry=0;
    Long64_t chainEntry =0;
    Double_t vAlphb=fRHist->GetYaxis()->GetBinCenter(ia+1);
    Double_t vAlph=fRHist->GetYaxis()->GetBinLowEdge(ia+1);
    // for(Int_t itr=0;itr<Elist->GetN();itr++){
    //   treeEntry = Elist->GetEntryAndTree(itr,treenum);
    //   if(chain) treeEntry+=chain->GetTreeOffset()[treenum];
    //  tree->GetEntry(treeEntry);
    for(Int_t itr=0;itr<NFT;itr++){
      FastTree->GetEntry(itr);
      Double_t weight=1;
      if(fNWdim==1)weight=GetDistWeight(wVal[0]);
      else if(fNWdim==2)weight=GetDistWeight(wVal[0],wVal[1]);
      else if(fNWdim==3)weight=GetDistWeight(wVal[0],wVal[1],wVal[2]);
      //  cout<<GetName()<<" Found weight "<<weight<<endl;
      temph->Fill(gRandom->Gaus(tvar,vAlph),vAlphb,weight);
      temph->Fill(gRandom->Gaus(tvar,vAlph),vAlphb,weight);
      temph->Fill(gRandom->Gaus(tvar,vAlph),vAlphb,weight);
      temph->Fill(gRandom->Gaus(tvar,vAlph),vAlphb,weight);
      temph->Fill(gRandom->Gaus(tvar,vAlph),vAlphb,weight);
      temph->Fill(gRandom->Gaus(tvar,vAlph),vAlphb,weight);
      //call it 3 times ! 3 times as slow...but should be a bit smoother...
      //Could add full function on each time but slow NbinsX x slower!
      // for(Int_t itemp=1;itemp<temph->GetNbinsX();itemp++){//fill with Gaussian function
      // 	Float_t varb=temph->GetXaxis()->GetBinCenter(itemp);
      // 	temph->SetBinContent(itemp,ia+1,TMath::Gaus(varb,tvar,fRHist->GetYaxis()->GetBinCenter(ia+1)));
      // }
    }
    fRHist->Add(temph);
  }
  cout<<"Done loop"<<endl;
  fRHist->Smooth();//some additional smoothing
  // for(Int_t ib=1;ib<=fRHist->GetNbinsX();ib++)//see if non-zero bins help fit stability
  // if(fRHist->GetBinContent(ib)==0) fRHist->SetBinContent(0.1);
  //Store mean value of distributions for scaling around
  // fMean=fRHist->GetMean();
  Int_t bx,by,bz;
  fRHist->GetBinXYZ(fRHist->GetMaximumBin(),bx,by,bz);
  fMean=fRHist->GetXaxis()->GetBinCenter(bx);
  cout<<"Mean value of distribution is "<<fMean<<endl;
  //create hist for getting weight

  fHist = new RooDataHist(fRHist->GetName(),fRHist->GetName(),RooArgSet(*fx_off,*falpha),RooFit::Import(*fRHist));
  //create pdf for getting analytical integral
  fHistPdf = new RooHistPdf(TString("PDF")+fRHist->GetName(),TString("PDF")+fRHist->GetName(),RooArgSet(*fx_off,*falpha),RooArgSet(*fx,*falpha),*fHist,1); 
  delete temph;
  tree->SetEntryList(0); //detach entry list
  if(Elist) delete Elist;
  tree->SetBranchStatus("*",1);
  tree->ResetBranchAddresses();
  if(chain){chain->ResetBranchAddresses();chain->GetStatus()->Clear();}
  if(FastTree) delete FastTree;
  delete fTree;
  fTree=0;
  return NFT;
}
Int_t THSEventsPDF::getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars,const char* rangeName) const
{  
  // cout<<x.min()<<" "<<x.max()<<" "<<fx_off->getMin()<<" "<<fx_off->getMax()<<endl;
fx_off->setRange(x.min()-offset,x.max()-offset);
  return 0; //might be good to check numerical integral sometimes
   return fHistPdf->getAnalyticalIntegral(allVars,analVars);
}
Double_t THSEventsPDF::analyticalIntegral(Int_t code,const char* rangeName) const
{
  Double_t integral=fHistPdf->analyticalIntegral(code)/scale;
  //  cout<<"fitst "<<integral<<endl;
  fx_off->setRange(x.min()-offset,x.max()-offset);
  //  adjustBinning();//may be needed for proper integration when scale paramter is free...
  //integral=fHistPdf->analyticalIntegral(code)/scale;
  //fx_off->setRange(x.min(),x.max());//in case adjust binning called
  //cout<<"sec "<<integral<<endl;
  return integral;
}

void THSEventsPDF::adjustBinning(Int_t* offset1) const
{
   RooRealVar* xvar = fx_off ;
  if (!dynamic_cast<RooRealVar*>(xvar)) {
    coutE(InputArguments) << "RooDataHist::adjustBinning(" << GetName() << ") ERROR: dimension " << xvar->GetName() << " must be real" << endl ;
    assert(0) ;
  }
  Double_t xlo = xvar->getMin() ;
  Double_t xhi = xvar->getMax() ;
  //adjust bin range limits with new scale parameter
  //cout<<scale<<" "<<fMean<<" "<<xlo<<" "<<xhi<<endl;
  xlo=(xlo-fMean)/scale+fMean;
  xhi=(xhi-fMean)/scale+fMean;
  if(xvar->getBinning().lowBound()==xlo&&xvar->getBinning().highBound()==xhi) return;
  xvar->setRange(xlo,xhi) ;
  // Int_t xmin(0) ;
  // cout<<"THSEventsPDF::adjustBinning( "<<xlo <<" "<<xhi<<endl;
  //now adjust fitting range to bin limits??Possibly not
  if (fRHist->GetXaxis()->GetXbins()->GetArray()) {

    RooBinning xbins(fRHist->GetNbinsX(),fRHist->GetXaxis()->GetXbins()->GetArray()) ;

    Double_t tolerance = 1e-6*xbins.averageBinWidth() ;
    
    // Adjust xlo/xhi to nearest boundary
    Double_t xloAdj = xbins.binLow(xbins.binNumber(xlo+tolerance)) ;
    Double_t xhiAdj = xbins.binHigh(xbins.binNumber(xhi-tolerance)) ;
    xbins.setRange(xloAdj,xhiAdj) ;

    xvar->setBinning(xbins) ;
    if (fabs(xloAdj-xlo)>tolerance||fabs(xhiAdj-xhi)<tolerance) {
      coutI(DataHandling) << "RooDataHist::adjustBinning(" << GetName() << "): fit range of variable " << xvar->GetName() << " expanded to nearest bin boundaries: [" 
			  << xlo << "," << xhi << "] --> [" << xloAdj << "," << xhiAdj << "]" << endl ;
    }


  } else {

    RooBinning xbins(fRHist->GetXaxis()->GetXmin(),fRHist->GetXaxis()->GetXmax()) ;
    xbins.addUniform(fRHist->GetNbinsX(),fRHist->GetXaxis()->GetXmin(),fRHist->GetXaxis()->GetXmax()) ;

    Double_t tolerance = 1e-6*xbins.averageBinWidth() ;

    // Adjust xlo/xhi to nearest boundary
    Double_t xloAdj = xbins.binLow(xbins.binNumber(xlo+tolerance)) ;
    Double_t xhiAdj = xbins.binHigh(xbins.binNumber(xhi-tolerance)) ;
    xbins.setRange(xloAdj,xhiAdj) ;
    xvar->setRange(xloAdj,xhiAdj) ;
    //xvar->setRange(xlo,xhi) ;
 
  }
  return;
}
void THSEventsPDF::SetWeightHist(TH1* hw,TString sx,TString sy,TString sz){
  //set a histogram to weight PDF tree with
  //sx, sy and sz should be names in the tree corresponding to the histogram axis
  fWeightHist=(TH1*)hw->Clone("WeightHist");
  fWeightHist->SetDirectory(0);
  fNWdim=1;
  fWeightHist->GetXaxis()->SetTitle(sx);
  if(sy!=TString(""))  {fWeightHist->GetYaxis()->SetTitle(sy);fNWdim++;}
  if(sz!=TString(""))  {fWeightHist->GetZaxis()->SetTitle(sz);fNWdim++;}
}
Double_t THSEventsPDF::GetDistWeight(Double_t wx,Double_t wy,Double_t wz){
  //given values wx,wy,wz find the bin in the weight histogram and return its value
  if(fNWdim==1)return fWeightHist->GetBinContent(fWeightHist->GetXaxis()->FindFixBin(wx));
  else if(fNWdim==1)return fWeightHist->GetBinContent(fWeightHist->GetXaxis()->FindFixBin(wx),fWeightHist->GetYaxis()->FindFixBin(wy));
  else  if(fNWdim==2)return fWeightHist->GetBinContent(fWeightHist->GetXaxis()->FindFixBin(wx),fWeightHist->GetYaxis()->FindFixBin(wy),fWeightHist->GetZaxis()->FindFixBin(wz));
  else return 1;
}
