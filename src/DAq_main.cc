//includes do C/C++
#include <iostream>
#include <getopt.h>

//includes deste codigo
#include "DAq_main.h"
#include "Digitizer.h"
#include "xmlParser.h"
#include "help.h"
#include "parseOpt.h"


volatile sig_atomic_t flag=0;

void my_function(int sig){ // can be called asynchronously
  flag = 1; // set flag
}


int main(int argc, char *argv[]){
  cout<<"================================================="<<endl;
  cout<<"DAq using digitizer CAEN DT5720B"<<endl;
  cout<<"Usage: DAqTanca [-v] -x Tanca_v01.xml"<<endl;
  cout<<"================================================="<<endl;

  //Linux signal()
  signal(SIGINT, my_function); // SIGINT=Interrupt from keyboard

  XmlParser settings = getOpt(argc, argv);

  Digitizer digi(settings);

  if(verbose){
    cout<<digi;
    digi.setVerbose(verbose);
  }
  bool digiOpen = digi.OpenDigitizer();

  if(digiOpen){
     digi.Readout();
     digi.CloseDigitizer();
  }

}
