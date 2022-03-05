#include "fileManager.h"
#include <sstream>
#include <stdint.h>
#include <sys/stat.h>

#include <algorithm>

void fileManager::init(string filename="DT5720B.root", uint16_t EnableMask=0, int saveInt=100){
  string temp = filename;
  finalFilename = filename;

  saveInterval=saveInt;


  string suffix=".root";
  size_t pos = temp.find(suffix);

  if (pos != std::string::npos) {
    // If found then erase it from string
    temp.erase(pos, suffix.length());
  }
  temp.erase(std::remove(temp.begin(), temp.end(), '/'), temp.end());

  dirname = util::dirName();

  string makeCommand = "mkdir -p "+dirname;

  int ret = system(makeCommand.c_str());
  if(ret!=0){
    cout<<"fileManager: Error creating temporary file directory"<<endl;
    exit(0);
  }

  fname=dirname+"/"+temp;

  mask = bitset<4>(EnableMask);
  RunStartTime=0;
  isOpen=false;
  for(int i=0; i<4; i++){
    lastTrigTime[i]=0;
    nRollover[i]=0;
  }
  verbose=false;

  initialized=true;
}


void fileManager::OpenFile(){

  stringstream ss;
  ss<<int(util::markTime());
  dirname = util::dirName();
  horamin = util::HoraMin();
  string firstFile = "./"+dirname+"/"+dirname+"_"+horamin+"tanca.root";
  ss.str("");

  if(verbose){
    cout<<"fileManager: Opening "<<firstFile<<endl;
    cout<<"fileManager: Save interval: "<<saveInterval<<endl;//coloquei saveInterval grande!!
  }

  f = new TFile(firstFile.c_str(), "RECREATE");  //open TFile
  t = new TTree("data", "Waveform data");  //initalize the TTree

  //resize the vector of vectors to have 4 entries
  data.resize(8);

  counter=0;
  fileCounter=0;

  //stringstream ss;

  //add vectors to TTree
  for(int i=0; i<4; i++){
    ss<<i;
    string branch="ch"+ss.str();
    ss.str("");
    if(mask[i]==1){
      t->Branch(branch.c_str(), &data[i]);
    }
  }

  //time is the unix time that the event occured
  t->Branch("time", &eventTime, "time/D");
  //t->Branch("xinc", &xinc, "xinc/D");

  xinc=4e-9;// DT5720B Ts=4ns

  isOpen=true;

}


void fileManager::CloseFile(){
  //if(isOpen){
  //cout<<"a file is open"<<endl;
//    ACF: na aquisição contínua se a aquisição for abortada com "q" salva o arquivo aberto.
    t->Write();
    f->Close();
    //}
//ACF: linhas comentadas para que na aquisição contínua do Tanca os arquivos temporários .root não são juntados
// num único arquivo .root
//    stringstream ss;
//    ss<<int(RunStartTime);// int(util::markTime());
//    finalFilename =ss.str()+finalFilename;
//    ss.str("");
//
//    if(fileCounter==0){
//
//      string mvCommand = "mv "+fname+"_0.root "+finalFilename;
//
//      int ret = system(mvCommand.c_str());
//
//      if(ret!=0){
//	cout<<"fileManager: Error moving temporary file. Temporary directory will not be removed\n";
//      } else {
//	DeleteDir();
//      }
//
//      return;
//    }
//
//
//  string files="";
//
//  for(int i=0; i<=fileCounter; i++){
//    ss<<i;
//    files += fname+"_"+ss.str()+".root ";
//
//    ss.str("");
//  }
//  string targetFile = finalFilename;
//
//  remove(targetFile.c_str());
//
//  string command = "hadd -v -f "+targetFile+" "+files;
//
//  if(verbose){
//    cout<<"fileManager: Merging temporary files"<<endl;
//  }
//  int ret = system(command.c_str());
//
//  bool mergeError=false;
//
//  if(ret!=0){
//    cout<<"fileManager: Error with merger command. Temporary files will not be deleted"<<endl;
//    mergeError=true;
//  }
//
//
//  if(!mergeError){
//    for(int i=0; i<=fileCounter; i++){
//      ss<<i;
//      remove((fname+"_"+ss.str()+".root").c_str());
//      ss.str("");
//    }
//
//    DeleteDir();
//  }

  if(verbose)
    cout<<"fileManager: Closed "<<fname<<endl;
}


void fileManager::addEvent(CAEN_DGTZ_EventInfo_t *EventInfo, CAEN_DGTZ_UINT16_EVENT_t *Event16){

  if(!isOpen)
    return;

  // choosing the time interval for close the .root files
  //double nowTime=util::markTime();
  hourNow = util::hour();
  minNow = util::minut();

  // use hourNow or minNow hourly files .root or every minute
  int TimeNow = hourNow;

  if (firstTime == 0) {TimeBefore = TimeNow; firstTime=1;}

  if(TimeNow != TimeBefore){
    OpenNewFile();
    if(verbose) cout<<"FileManager: OpenNewFile: "<<TimeNow<<endl;
    TimeBefore = TimeNow;
  }

  for(int ch=0; ch<4; ch++){
    if(mask[ch]==0)continue;
    int Size=Event16->ChSize[ch];
    if(Size<=0)continue;

    for(int i=0; i<Size; i++){
      data[ch].push_back(Event16->DataChannel[ch][i]);
    }

    if(EventInfo->TriggerTimeTag<lastTrigTime[ch]){
      nRollover[ch]++;
      lastTrigTime[ch]=0;

    }else
      lastTrigTime[ch]=EventInfo->TriggerTimeTag;

    eventTime = (double)EventInfo->TriggerTimeTag*8e-9+RunStartTime+nRollover[ch]*rolloverAdd;

  }

  t->Fill();

  for(int ch=0; ch<4; ch++)
    data[ch].clear();

  counter++;

}


void fileManager::OpenNewFile(){

  if(verbose){
    cout<<"fileManager: number of events so far = "<<counter<<endl;
  }

  t->Write(); //write the data TTree
  f->Close();

  fileCounter++;

  stringstream ss;
  ss<<int(util::markTime());
  dirname = util::dirName();
  horamin = util::HoraMin();
  string thisFile = "./"+dirname+"/"+dirname+"_"+horamin+"tanca.root";
  ss.str("");

  f = new TFile(thisFile.c_str(), "RECREATE");  //open TFile
  t = new TTree("data", "Waveform data");  //initalize the TTree

  //resize the vector of vectors to have 4 entries
  data.resize(8);

  //add vectors to TTree
  for(int i=0; i<4; i++){
    ss<<i;
    string branch="ch"+ss.str();
    ss.str("");
    if(mask[i]==1){
      t->Branch(branch.c_str(), &data[i]);
    }
  }

  //time is the unix time that the event occured
  t->Branch("time", &eventTime, "time/D");
  //t->Branch("xinc", &xinc, "xinc/D");
}


void fileManager::DeleteDir(){

//  string command = "rmdir "+dirname+"/";
//  int ret = system(command.c_str());
//  if(ret!=0)
    cout<<"fileManager: Could not delete temporary directory"<<endl;
}
