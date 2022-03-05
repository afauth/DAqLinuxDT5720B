
void HistoTancaOneCh(string filename, Int_t ch){
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
	
  float Ts=4.0; //ns	
  float Rin=50.0;// Ohm	
	
  TCanvas* c1 = new TCanvas("c1","Tanca",0,0,1000,800);
  c1->SetGrid();
  c1->Divide(1,3);	
			
  // Open the file containing the tree
  TFile *f = new TFile(filename.c_str(), "READ");
  TTree *t = (TTree*)f->Get("data");	
  Int_t entries=(Int_t)t->GetEntries();
  cout<< "Entries = " << entries << "   ";
  
  //TH1F uses double  
  vector<Double_t> *chA = new vector<Double_t>;
  Double_t xinc = 4e-9;//DT5720B, Ts=4ns
  Double_t *tm;

  stringstream ss;
  ss<<ch;
  TString c = ss.str();
	
  t->SetBranchAddress("ch"+c, &chA);
  t->SetBranchAddress("time", &tm);
	
  // Create a histogram to draw the charge spectrum
  TH1F *hQ= new TH1F("histQ", "Charge", 1000, 0,13000);
  TH1F *hAmp= new TH1F("histAmp", "Amplitude", 1000, -1000,130);	

  Int_t sum;
  //Use factor to Fill only part of the entries
  float factor = 1.;
  cout<<"Filled = "<<entries*factor<<endl;

   TGraph *gr = new TGraph();;	
	
  for(Int_t i=0;i<entries*factor;i++){
	t->GetEntry(i);
	Float_t Amp = 0;
	Int_t Amp_index = -1;  
	sum = 0; 
	Int_t recLen=chA->size();
	for(Int_t j=0;j<recLen;j++){
		sum=sum+(chA->at(j)-DCoffsetADC);
		if(i<1){gr->SetPoint(j, j*xinc/1e-9, (chA->at(j)-DCoffsetADC)*mVch);} //grafico de um pulso  
		
		if((*chA)[j] > Amp){
			Amp = (*chA)[j];
			Amp_index=j;
		}
	}
	  
	// Carga do pulso em pC
	Int_t Q = (sum*Ts*mVch)/Rin; 
	hQ->Fill(-Q);
	// Amplitude do pulso em mV  
	Amp = (Amp-DCoffsetADC)*mVch;  
	hAmp->Fill(-Amp);  
  }
	
	c1->cd(1);
	gr->GetXaxis()->SetTitle("Time (ns)");
    gr->GetYaxis()->SetTitle("V(mV)");
    gr->Draw("ALP");
	
	c1->cd(2);
	hAmp->Draw();
	
	c1->cd(3);
	hQ->Draw();


  //c1->SaveAs("CArapuca00.jpg");

}
