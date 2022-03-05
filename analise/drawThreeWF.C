void drawThreeWF(string filename, int ch1st,int ch2nd, int ch3rd, int event){

  TCanvas* c01 = new TCanvas("c01","Tanca",0,0,800,400);
  c01->SetGrid();
	
  TFile *f = new TFile(filename.c_str(), "READ");
  TTree *t = (TTree*)f->Get("data");

  cout<<"Entries="<<t->GetEntries()<<endl;
  
  stringstream ss[3];
  ss[0]<<ch1st;
  ss[1]<<ch2nd;
  ss[2]<<ch3rd;
  TString c1 = ss[0].str();
  TString c2 = ss[1].str();
  TString c3 = ss[2].str();
	
  vector<double> *chA = new vector<double>;
  vector<double> *chB = new vector<double>;
  vector<double> *chC = new vector<double>;


  double xinc=4e-9;
  double tm;
  
   	  
   t->SetBranchAddress("ch"+c1, &chA);
   t->SetBranchAddress("ch"+c2, &chB);
   t->SetBranchAddress("ch"+c3, &chC);
  
  t->SetBranchAddress("time", &tm);

  t->GetEntry(event);
	
  auto mg  = new TMultiGraph();
  auto gr1 = new TGraph(); gr1->SetMarkerStyle(20);
  auto gr2 = new TGraph(); gr2->SetMarkerStyle(21);
  auto gr3 = new TGraph(); gr3->SetMarkerStyle(23);
	
	
  for(int i=0; i<chA->size(); i++){
    gr1->SetPoint(i, i*xinc/1e-9, chA->at(i)); 
	gr2->SetPoint(i, i*xinc/1e-9, chB->at(i));
	gr3->SetPoint(i, i*xinc/1e-9, chC->at(i));
  }

   mg->Add(gr1,"PL");
   mg->Add(gr2,"PL");
   mg->Add(gr3,"PL");
	
  mg->GetXaxis()->SetTitle("Time (ns)");
  mg->GetYaxis()->SetTitle("ADC counts");

  mg->Draw("A pmc plc");
  
  f->Close();


}
