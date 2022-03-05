void drawOneWF(string filename, int ch1, int event){
  
  //relação entre mV e chADC
  float Vpp=2000.;// input range da digitalizadora em mV
  float NbitDigi=12.; 
  float FSRdigi= pow(2.0,NbitDigi) -1; // Full-Scale Range, em chADC, da digitalizadora
  float mVch= Vpp/FSRdigi; // para a DT5720B mVch=2000./4095. [mV/chADC]
	
  // do valor lido em ChADC deve-se subtrair o DCoffset, para isso uso a relação entre o ADC de 16bits e o de 12bits (=4095/65535)
  // e depois multiplico por mVch
  float NbitDAC=16.;//no.bits do DAC utilizado no DCoffset de cada canal da digitalizadora
  float DCoffset=32768.; //DCoffset de cada chADC definido no arquivo .xml utilizado na aquisição
  float FSRdac=pow(2.0,NbitDAC) -1; // Full-Scale Range, em chADC, do DAC da DCoffset de cada canal da DT5720B
  float RelADCs=(FSRdigi/FSRdac);
  float DCoffsetADC=DCoffset*RelADCs;//DCoffset em ch do ADC de digitalização	

  //para uma maior exatidão podemos usar uma tensão de referência para calibrar a DT5720B
	
  TCanvas* c1 = new TCanvas("c1","Tanca",0,0,800,400);
  c1->SetGrid();
	
  TFile *f = new TFile(filename.c_str(), "READ");
  TTree *t = (TTree*)f->Get("data");

  cout<<"Entries="<<t->GetEntries()<<endl;
  
  stringstream ss;
  ss<<ch1;
  TString c = ss.str();

  vector<double> *chA = new vector<double>;
  double xinc=4e-9;
  double tm;
  
  t->SetBranchAddress("ch"+c, &chA);
  t->SetBranchAddress("time", &tm);

  t->GetEntry(event);
	
  TGraph *gr = new TGraph();;
	
   for(int i=0; i<chA->size(); i++){
    gr->SetPoint(i, i*xinc/1e-9, (chA->at(i)-DCoffsetADC)*mVch);   
   }

  gr->GetXaxis()->SetTitle("Time (ns)");
  gr->GetYaxis()->SetTitle("V(mV)");

  gr->Draw("ALP");
  
  f->Close();


}
