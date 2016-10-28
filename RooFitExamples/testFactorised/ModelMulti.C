
//Run with 
//root 'ModelMulti.C( "Data.root" ,113)'
//root 'ModelMulti.C( "Model.root" ,11545)'

void ModelMulti(TString filename,Int_t seed=1111){
  //model where the signal distribution has 
  //the same shpa as one of the backgrounds
  //in mmiss, but different in M1
  //Hence first fit seperates signal and bg1 from bg2
  //second fit seperates signal from bg1

  gRandom->SetSeed(seed);
  Double_t Eg,Mmiss,M1,M2,fgID,Sig,Mmiss_M1;
  TTree* tree=new TTree("MyModel","MyModel");
  tree->Branch("Eg",&Eg,"Eg/D");
  tree->Branch("Mmiss",&Mmiss,"Mmiss/D");
  tree->Branch("M1",&M1,"M1/D");
  tree->Branch("Mmiss_M1",&Mmiss_M1,"Mmiss_M1/D");
  tree->Branch("M2",&M2,"M2/D");
  tree->Branch("fgID",&fgID,"ID/D");
  tree->Branch("Sig",&Sig,"Sig/D");

  TTree* tsig=tree->CloneTree(0);
  TTree* tbg=tree->CloneTree(0);
  TTree* tbg2=tree->CloneTree(0);
  //signal
  TF1* fM1s=new TF1("m1s","gaus(0)+gaus(3)+[6]",0,10);
  fM1s->SetParameters(1,3,0.5,0.5,7,2,0.1);
  TF1* fM2s=new TF1("m2s","gaus(0)+[3]*x",0,10);
  fM2s->SetParameters(1,5,0.1,0.1);
  fMmisss=new TF1("mmisss","gaus(0)",0,10);
  //fMmisss->SetParameters(1,0.1,1);
  fMmisss->SetParameters(1,5,0.7);
  
  //bakcground 1
  TF1* fM1b=new TF1("m1b","2-[0]*x",0,10);
  fM1b->SetParameter(0,0.1);
  TF1* fM2b=new TF1("m2b","2-[0]*x",0,10);
  fM2b->SetParameter(0,0.05);
  // fMmissb=new TF1("mmissb","[0]*(x-4)+2",0,10);
  //fMmissb->SetParameter(0,0);


  TF1* fM1b2=new TF1("m1b2","1+[0]*x*x",0,10);
  fM1b2->SetParameter(0,0.3);
  TF1* fM2b2=new TF1("m2b2","2+[0]*x",0,10);
  fM2b2->SetParameter(0,0.05);
  fMmissb2=new TF1("mmissb2","pol0(0)",0,10);
  fMmissb2->SetParameter(0,1);
  //background 2

  Int_t Nev=100000;
  for(Int_t i=0;i<Nev;i++){
    fgID=i;
    Double_t rand=gRandom->Uniform();
    if(rand>0.6){
      Eg=gRandom->Uniform(3,4);
      M1=fM1s->GetRandom();
      M2=fM2s->GetRandom();
      Mmiss=fMmisss->GetRandom();
      Sig=1;
      tsig->Fill();
     }
    else if(rand>0.3){
      Eg=gRandom->Uniform(3,4);
      M1=fM1b->GetRandom();
      M2=fM2b->GetRandom();
      // fMmissb->SetParameter(0,0.5);
      // fMmissb->SetParameter(0,0);
      Mmiss=fMmisss->GetRandom();
      Sig=-1;
      tbg->Fill();
    }
    else{
      Eg=gRandom->Uniform(3,4); //note different E range
      M1=fM1b2->GetRandom();
      M2=fM2b2->GetRandom();
      //fMmissb2->SetParameter(0,0.2);
      // fMmissb->SetParameter(0,0);
      Mmiss=fMmissb2->GetRandom();
      Sig=-2;
      tbg2->Fill();
      
    }
    tree->Fill();
  }
  
  //Save data
 TFile* file=new TFile(filename,"recreate");
  tree->Write();
  file->Close();

  //Save signal shape
  file=new TFile(TString("Sig")+filename,"recreate");
  tsig->Write();
  file->Close();
  //save background shapes
  file=new TFile(TString("BG1")+filename,"recreate");
  tbg->Write();
  file->Close();

  file=new TFile(TString("BG2")+filename,"recreate");
  tbg2->Write();
  file->Close();


}
