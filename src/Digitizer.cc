#include "Digitizer.h"
#include <sstream>
#include <bitset>


static CAEN_DGTZ_IRQMode_t INTERRUPT_MODE = CAEN_DGTZ_IRQ_MODE_ROAK;
extern volatile sig_atomic_t flag;


Digitizer::Digitizer(XmlParser settings){
  DefaultSettings();

  if(settings.isEmpty()) return;

  if(settings.fieldExists("verbose")){
    verbose = (bool)settings.getValue("verbose");
  }

  if(settings.fieldExists("duration")){
    string d = settings.getStringValue("duration");
    if(d.find(":")!= std::string::npos){
      timeLimit=true;
      char token=':';
      vector<string> parts = util::split(d.c_str(), token);

      if(parts.size()==3)
	DurationOfRun = 3600*atoi(parts[0].c_str())+60*atoi(parts[1].c_str())+atoi(parts[2].c_str());
      else if(parts.size()==2)
	DurationOfRun = 60*atoi(parts[0].c_str())+atoi(parts[1].c_str());
      else {
	cout<<"Digitizer: Invalid time format. Use HH:MM:DD"<<endl;
	exit(0);
      }

      startImmed=true;

    } else{
      numOfEvents=(int)settings.getValue("duration");
      if(numOfEvents == 0){ //0 is used for continuous data taking
       eventContinuous=true;
       startImmed=true;
      }else{
      eventLimit=true;
      startImmed=true;
      }

    }
  }

  if(settings.fieldExists("saveInterval")){
    saveInterval = (int)settings.getValue("saveInterval");
  }


  if(settings.fieldExists("reclen")){
    RecordLength = (uint32_t)settings.getValue("reclen");
  }

  if(settings.fieldExists("baseaddress")){
    BaseAddress = (int)settings.getValue("baseaddress");
  }

  if(settings.fieldExists("posttrigger")){
    PostTrigger = (int)settings.getValue("posttrigger");
  }

  if(settings.fieldExists("outfile")){
    fname=settings.getStringValue("outfile");
  }

  if(settings.fieldExists("triggermode")){
    if(settings.getStringValue("triggermode").compare("OR")==0){
      triggerOR=true;
      triggerMaj1=false;
    } else if(settings.getStringValue("triggermode").compare("Maj1")==0){
      triggerMaj1=true;
      triggerOR=false;
    }
  }

  if(settings.fieldExists("coincidencewindow")){
    c_window=(int)settings.getValue("coincidencewindow");
  }



  uint16_t tempEnableMask=0;
  stringstream ss;

  bool trigChannel=false;

  for(int i=0; i<MAX_SET; i++){
    ss<<i;
    string num = ss.str();
    ss.str("");

    if(settings.fieldExists("ch"+num)){
      if(settings.getValue("ch"+num)==1){
	tempEnableMask += (1<<i);
      }else{
	Threshold[i]=0;
	continue;
      }
    } else{
      Threshold[i]=0;
      continue;
    }

    if(settings.fieldExists("polarity"+num)){
      if(settings.getStringValue("polarity"+num).compare("POSITIVE")==0)
	PulsePolarity[i]=CAEN_DGTZ_PulsePolarityPositive;
      else
	PulsePolarity[i]=CAEN_DGTZ_PulsePolarityNegative;
    }

    if(settings.fieldExists("trslope"+num)){
      if(settings.getStringValue("trslope"+num).compare("POSITIVE")==0)
	TrigPolarity[i]=CAEN_DGTZ_TriggerOnRisingEdge;
      else
	TrigPolarity[i]=CAEN_DGTZ_TriggerOnFallingEdge;
    }


    if(settings.fieldExists("DCoffset"+num))
      DCoffset[i] = (uint32_t)settings.getValue("DCoffset"+num);

    if(settings.fieldExists("threshold"+num)){
      trigChannel=true;
      ChannelTriggerMode[i]=CAEN_DGTZ_TRGMODE_ACQ_ONLY;//CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      Threshold[i] = (uint32_t)settings.getValue("threshold"+num);
      thr_file[i] = (uint32_t)settings.getValue("threshold"+num);
      Version_used[i]=1;
    } else {
      Threshold[i]=0;
      ChannelTriggerMode[i]=CAEN_DGTZ_TRGMODE_DISABLED;
      Version_used[i]=0;
    }
  }

  if(tempEnableMask!=0)
    EnableMask=tempEnableMask;

  if(!trigChannel){
    cout<<"Digitizer: Warning: No trigger set! use --threshold<CH> to set the threshold for one of the enabled channels."<<endl;
    cout<<"           Threshold on first enabled channel will be set to 100"<<endl;


    bitset<4> mask(EnableMask);

    for(int i=0; i<MAX_SET; i++){
      if(mask[i]==1){
	ChannelTriggerMode[i]=CAEN_DGTZ_TRGMODE_ACQ_ONLY;
	Threshold[i]=100;
	thr_file[i]=100;
	Version_used[i]=1;
	break;
      }
    }

  }


}


void Digitizer::DefaultSettings(){
  CalibComplete=true;//retirei a calibração do DAC colocando true

  handle = -1;
  Nb=0;
  Ne=0;
  nCycles= 0;

  saveInterval=1e9; // coloquei um valor alto

  manualStop=false;
  verbose=false;

  triggerMaj1=true;
  triggerOR=false;

  c_window=10; // janela coincidencia c_window*8 ns

  //default settings
  LinkType = CAEN_DGTZ_USB;
  LinkNum=0;
  ConetNode=0;
  BaseAddress=0;//0x32200000;
  Nch = 4;
  Nbit = 12;
  Ts = 4.0;
  NumEvents=1;
  nevent=0;
  RecordLength=70;
  PostTrigger=33;
  InterruptNumEvents=0;
  TestPattern=0;
  FPIOtype=CAEN_DGTZ_IOLevel_TTL;//CAEN_DGTZ_IOLevel_NIM
  ExtTriggerMode=CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  EnableMask=1;

  ChannelTriggerMode[0]=CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  PulsePolarity[0]=CAEN_DGTZ_PulsePolarityPositive;
  DCoffset[0]=3276;
  Threshold[0]=10;
  Version_used[0]=1;
  TrigPolarity[0]=CAEN_DGTZ_TriggerOnRisingEdge;

  for(int i=1; i<MAX_SET; i++){
    ChannelTriggerMode[i]=CAEN_DGTZ_TRGMODE_DISABLED;
    PulsePolarity[i]=CAEN_DGTZ_PulsePolarityPositive;
    TrigPolarity[i]=CAEN_DGTZ_TriggerOnRisingEdge;
    DCoffset[i]=3276;
    Threshold[i]=0;
    Version_used[i]=0;
  }
  /*
  GWn   = 0;
  for(int i=0; i<MAX_GW; i++){
    GWaddr[i]=0;
    GWdata[i]=0;
    GWmask[i]=0;
    }*/

  Quit=0;
  AcqRun=0;
  PlotType=0;
  ContinuousTrigger=0;
  ContinuousWrite=1;
  SingleWrite=0;
  Restart=0;
  RunHisto=0;
  for(int i=0; i<MAX_CH; i++){
    fout[i]=NULL;
    dc_file[i]=50;
    thr_file[i]=100;
  }

  fname="DT5720B.root";


  eventLimit=false;
  eventContinuous=false;
  timeLimit=false;
  numOfEvents=-1;
  DurationOfRun=-1;
  nowTime=0;

  startImmed=false;

}


bool Digitizer::OpenDigitizer(){



  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  isVMEDevice= 0;

  if(verbose){
    cout<<"================================================="<<endl;
    cout<<"Digitizer: Opening digitizer"<<endl;
  }
  /* *************************************************************************************** */
  /* Open the digitizer and read the board information                                       */
  /* *************************************************************************************** */
  isVMEDevice = 0;
  isVMEDevice = BaseAddress ? 1 : 0;

  ret = CAEN_DGTZ_OpenDigitizer(LinkType, LinkNum, ConetNode, BaseAddress, &handle);
  if (ret) {
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    if(fman.isInit())
      fman.DeleteDir();
    return false;
  }


  int ReloadCfgStatus = 0x7FFFFFFF; // Init to the bigger positive number
  buffer = NULL;
  EventPtr = NULL;
  nCycles= 0;
  Event16=NULL; /* generic event struct with 16 bit data (10, 12, 14 and 16 bit digitizers */

  fman = fileManager(fname, EnableMask, saveInterval);
  fman.setVerbose(verbose);
  fman.OpenFile();



  ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
  if (ret) {
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return false;

  }
  if(verbose)cout<<"================================================="<<endl;
  printf("Connected to CAEN Digitizer Model %s\n", BoardInfo.ModelName);
  printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
  printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);

  // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
  sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
  if (MajorNumber >= 128) {
    printf("This digitizer has a DPP firmware\n");
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return false;
  }


  // Perform calibration (if needed).

  /* *************************************************************************************** */
  /* program the digitizer                                                                   */
  /* *************************************************************************************** */
  ret = ProgramDigitizer();
  if (ret) {
    return false;
  }


  // Read again the board infos, just in case some of them were changed by the programming
  // (like, for example, the TSample and the number of channels if DES mode is changed)
  if(ReloadCfgStatus > 0) {
    ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    if (ret) {
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return false;
    }

  }

  // Allocate memory for the event data and readout buffer
  ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);

  if (ret != CAEN_DGTZ_Success) {
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return false;
  }
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer,&AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
  if (ret) {
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return false;
  }

  return true;

}


void Digitizer::Readout(){

  if(startImmed){
    startAcq();
  }

  if (Restart && AcqRun)
    {
      if(!CalibComplete) Calibrate_DC_Offset(); //ACF: retirei a calibraçao d DAC
      CAEN_DGTZ_SWStartAcquisition(handle);
      RunStartTime = util::markTime();
      fman.setRunStartTime(RunStartTime);
    }
  else if(!startImmed)
    printf("[s] start/stop the acquisition, [q] quit, [SPACE] help\n");
  else
    printf("[q] to quit, [s] to stop/start run\n");

  Restart = 0;


  /* *************************************************************************************** */
  /* Readout Loop                                                                            */
  /* *************************************************************************************** */

  PrevRateTime = util::get_time();

  Quit=false;
  //ACF: para o DAq do Tanca esse loop é infinito. Retiro no final este while as condiçoes de saida do loop.
  while(!Quit) {
    if(flag==1){
      Quit=true;
      manualStop=true;
    }

    // Check for keyboard commands (key pressed)
    //if(!eventLimit&&!timeLimit&&!eventContinuous)
    CheckKeyboardCommands();
   //cout<<nevent<<endl;

    if (AcqRun == 0)
      continue;

    /* Send a software trigger */
    if (ContinuousTrigger) {
      CAEN_DGTZ_SendSWtrigger(handle);
    }

    /* Wait for interrupt (if enabled) */
    if (InterruptNumEvents > 0) {
      int32_t boardId;
      int VMEHandle = -1;
      int InterruptMask = (1 << VME_INTERRUPT_LEVEL);

      BufferSize = 0;
      NEvents = 0;
      // Interrupt handling
      if (isVMEDevice) {
	ret = CAEN_DGTZ_VMEIRQWait ((CAEN_DGTZ_ConnectionType)LinkType, LinkNum, ConetNode, (uint8_t)InterruptMask, INTERRUPT_TIMEOUT, &VMEHandle);
      }
      else
	ret = CAEN_DGTZ_IRQWait(handle, INTERRUPT_TIMEOUT);
      if (ret == CAEN_DGTZ_Timeout)  // No active interrupt requests
	goto InterruptTimeout;
      if (ret != CAEN_DGTZ_Success)  {
	cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
	exit(0);
      }
      // Interrupt Ack
      if (isVMEDevice) {
	printf("isvme\n");
	ret = CAEN_DGTZ_VMEIACKCycle(VMEHandle, VME_INTERRUPT_LEVEL, &boardId);
	if ((ret != CAEN_DGTZ_Success) || (boardId != VME_INTERRUPT_STATUS_ID)) {
	  goto InterruptTimeout;
	} else {
	  if (INTERRUPT_MODE == CAEN_DGTZ_IRQ_MODE_ROAK)
	    ret = CAEN_DGTZ_RearmInterrupt(handle);
	}
      }
    }

    /* Read data from the board */
    ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
    if (ret) {
      cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
      exit(0);
    }
    NEvents = 0;
    if (BufferSize != 0) {
      ret = CAEN_DGTZ_GetNumEvents(handle, buffer, BufferSize, &NEvents);
      if (ret) {
	cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
	exit(0);
      }
    }
    else {
      uint32_t lstatus;
      ret = CAEN_DGTZ_ReadRegister(handle, CAEN_DGTZ_ACQ_STATUS_ADD, &lstatus);
      if (ret) {
	printf("Warning: Failure reading reg:%x (%d)\n", CAEN_DGTZ_ACQ_STATUS_ADD, ret);
	cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
      }
      else {
	if (lstatus & (0x1 << 19)) {
	  exit(0);
	}
      }
    }


  InterruptTimeout:
    /* Calculate throughput and trigger rate (every second) */
    Nb += BufferSize;
    Ne += NEvents;
    nevent +=NEvents;
    CurrentTime = util::get_time();
    ElapsedTime = CurrentTime - PrevRateTime;

    nCycles++;
    if (ElapsedTime > 10000) {// in milliseconds
      if (Nb == 0)
	if (ret == CAEN_DGTZ_Timeout) printf ("Timeout...\n"); else printf("No data...\n");
      else
	if(verbose)
	  printf("Reading at %.2f MB/s (Trg Rate: %.2f Hz, %d events)\n", (float)Nb/((float)ElapsedTime*1048.576f), (float)Ne*1000.0f/(float)ElapsedTime, nevent);
      nCycles= 0;
      Nb = 0;
      Ne = 0;
      PrevRateTime = CurrentTime;
    }

    /* Analyze data */
    for(int i = 0; i < (int)NEvents; i++) {

      /* Get one event from the readout buffer */
      ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, i, &EventInfo, &EventPtr);
      //cout<<ret<<endl;
      if (ret) {
	cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
	continue;
	//exit(0);
      }
      /* decode the event */
      ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);

      if (ret) {
	cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
	exit(0);
      }

      /* Write Event data to file */
      if (SingleWrite||ContinuousWrite) {
	fman.addEvent(&EventInfo, Event16);
	if (SingleWrite==1) {
	  printf("Single Event saved to output files\n");
	  SingleWrite = 0;
	}
      }

    }

//ACF ATENCAO: DEVO eliminar a parte qeu junta os arquivos .root temporários num arquivo único .root
// 1) isso foi feito comentando as linhas no "void fileManager::CloseFile()"
// 2) em "void fileManager::addEvent" modifico a abertura dos novos arquivos temporários (que agora são definitivos)
//    para a cada 01:00:00

    nowTime=util::markTime();
    if(timeLimit&&nowTime-RunStartTime>=DurationOfRun){
      Quit=true;
      if(verbose) cout<<"Digitizer: end time: "<<nowTime<<endl;
      break;
    }

    if(eventLimit&&nevent!=0&&nevent>=numOfEvents){
      Quit=true;
      if(verbose) cout<<"Digitizer: end numOfEvents: "<<nowTime<<endl;
      break;
    }
  }

}


void Digitizer::CloseDigitizer(){

  if(verbose)
    cout<<"Digitzer: Closing digitizer\n";

  /* stop the acquisition */
  CAEN_DGTZ_SWStopAcquisition(handle);

  /* close the output files and free histograms*/
  for (int ch = 0; ch < Nch; ch++) {
    if (fout[ch])
      fclose(fout[ch]);
  }

  /* close the device and free the buffers */
  CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_CloseDigitizer(handle);

  fman.CloseFile();



  stringstream ss;
  string subMessage="";
  if(manualStop){
    subMessage="Run aborted";
  }else if(eventLimit){
    ss<<numOfEvents;
    subMessage="Finished measureing "+ss.str()+" events";
  }else if(timeLimit){
    ss<<DurationOfRun;
    subMessage="Finshed "+ss.str()+" second run";
  } else
    return;

  string quote="\"";


#ifdef NOTIFY

  char *cd;
  cd = getenv("PWD");
  string pwd = cd;
  string icon = pwd+"/icon/CAEN.png";

  string command = "notify-send "+quote+subMessage+quote+" -i "+icon;
  int a = system(command.c_str());
  if(a){
    cout<<"Something went wrong. "<<a<<endl;
  }

#endif
  cout<<subMessage<<endl;



}


CAEN_DGTZ_ErrorCode Digitizer::ProgramDigitizer(){
  int i,ret = 0;

  if(verbose){
    cout<<"================================================="<<endl;
    cout<<"Digitizer: Programming digitizer\n";
  }
  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset(handle);
  if (ret != 0) {
    printf("Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program\n");
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return (CAEN_DGTZ_ErrorCode)ret;
  }

  // Set the waveform test bit for debugging
  if (TestPattern)
    ret |= CAEN_DGTZ_WriteRegister(handle, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);

  ret |= CAEN_DGTZ_SetRecordLength(handle, RecordLength);
  ret |= CAEN_DGTZ_GetRecordLength(handle, &RecordLength);

  ret |= CAEN_DGTZ_SetPostTriggerSize(handle, PostTrigger);

  ret |= CAEN_DGTZ_SetIOLevel(handle, FPIOtype);
  if( InterruptNumEvents > 0) {
    // Interrupt handling
    if( ret |= CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE, VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID, (uint16_t)InterruptNumEvents, INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
      printf( "\nError configuring interrupts. Interrupts disabled\n\n");
      InterruptNumEvents = 0;
    }
  }


  ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, NumEvents);
  ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, ExtTriggerMode);


  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, EnableMask);
  for (i = 0; i < Nch; i++) {
    if (EnableMask & (1<<i)) {
      ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, DCoffset[i]);
      ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, Threshold[i]);
      ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, TrigPolarity[i]); //.TriggerEdge
    }
  }

  if(triggerOR)
    ret |= OrProgrammer();
  else if(triggerMaj1)
    ret |= Maj1Programmer();

  if(verbose){
    cout<<"Digitizer: Trigger enabled:\n";
    for(int i=0; i<4; i++){
      CAEN_DGTZ_TriggerMode_t mode;
      CAEN_DGTZ_GetChannelSelfTrigger(handle, i, &mode);
      cout<<"\tChannel "<<i<<"\tEnabled: "<<mode<<endl;
    }
  }



  if (ret){
    printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
  }

  //reading the address Global Trigger Mask
  uint32_t d32;
  ret = CAEN_DGTZ_ReadRegister(handle, 0x810C, &d32);
  if(ret != CAEN_DGTZ_Success) cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
  if(verbose==1) cout<<"Global Trigger Mask 0x810C address = "<<bitset<32>(d32)<<endl;

  return CAEN_DGTZ_Success;
}

// setting trigger Majority = 1 (any double or triple coincidence)
CAEN_DGTZ_ErrorCode Digitizer::Maj1Programmer(){
  if(!CalibComplete) Calibrate_DC_Offset(); //ACF: retirei a calibração do DAC, ver se DT5720B precisa!!!!

  int ret=0;
  uint32_t message;
  uint32_t coincAddress=0x810C;
  uint32_t mask = 0xFFFFFFFF;

  int MajorityLevel = 1;
  //0x1A00007 , 0000 0001 1010 0000 0000 0000 0000 0111
  message = 0x7+(MajorityLevel<<24)+(c_window<<20);
  ret |= WriteRegisterBitmask(coincAddress, message, mask);

  return (CAEN_DGTZ_ErrorCode)ret;
}

// setup for an 'OR' trigger
CAEN_DGTZ_ErrorCode Digitizer::OrProgrammer(){
  int ret=0;

  //0x7 , 0000 0000 0000 0000 0000 0000 0000 0111
  uint32_t message = 0x7;
  ret |= WriteRegisterBitmask(0x810C, message, 0xFFFFFFFF);

  return (CAEN_DGTZ_ErrorCode)ret;
}


CAEN_DGTZ_ErrorCode Digitizer::WriteRegisterBitmask(uint32_t address, uint32_t data, uint32_t mask) {
  int32_t ret = CAEN_DGTZ_Success;
  uint32_t d32 = 0xFFFFFFFF;

  ret = CAEN_DGTZ_ReadRegister(handle, address, &d32);
  if(ret != CAEN_DGTZ_Success){
    cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
    return (CAEN_DGTZ_ErrorCode)ret;
  }

  data &= mask;
  d32 &= ~mask;
  d32 |= data;
  ret = CAEN_DGTZ_WriteRegister(handle, address, d32);

  return (CAEN_DGTZ_ErrorCode)ret;
}


CAEN_DGTZ_ErrorCode Digitizer::Calibrate_DC_Offset(){

  //Esse codigo de calibração do DAC está no WaveDump.c
  // A DT5720B com o WaveDump faz a calibração e grava numa memoria flash da digitalizadora.
  // A V1730 preciso verificar
  // A DT5720B precisa desta calibração feita neste codigo???

  float cal = 1;
  float offset = 0;
  int i = 0, k = 0, p = 0, acq = 0, ch=0;
  CAEN_DGTZ_ErrorCode ret;
  CAEN_DGTZ_AcqMode_t mem_mode;
  uint32_t  AllocatedSize;

  uint32_t BufferSize;

  CAEN_DGTZ_EventInfo_t       EventInfo;
  char *buffer = NULL;
  char *EventPtr = NULL;
  CAEN_DGTZ_UINT16_EVENT_t    *Event16 = NULL;

  ret = CAEN_DGTZ_GetAcquisitionMode(handle, &mem_mode);//chosen value stored
  if (ret) printf("Error trying to read acq mode!!\n");

  ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
  if (ret) cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;

  ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
  if (ret) printf("Error trying to set ext trigger!!\n");

  ret = CAEN_DGTZ_SetPostTriggerSize(handle, 0);
  if (ret) printf("Error trying to set post trigger!!\n");

  ///malloc
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
  if (ret) {cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
            return ret;
            }

  ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
  if (ret != CAEN_DGTZ_Success) {cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;}

  ret = CAEN_DGTZ_SWStartAcquisition(handle);
  if (ret) { printf("Warning: error starting acq\n");
             cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
             return ret;
            }

  float avg_value[NPOINTS] = { 0 };

  uint32_t dc[NPOINTS] = {25,75}; //test values (%)

  for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++)
  {
    if (EnableMask & (1 << ch))
    {
      if(verbose) printf("Digitizer: Starting channel %d DAC calibration...\n", ch);
      ret = CAEN_DGTZ_SetChannelSelfTrigger(handle,CAEN_DGTZ_TRGMODE_DISABLED, (1 << ch));
      if (ret) printf("Warning: error disabling ch %d self trigger\n", ch);

      cal_ok[ch] = 1;
      if (cal_ok[ch])
      {
        for (p = 0; p < NPOINTS; p++)
        {
        //cout<<abs((int)dc[p] - 100)<<endl;
        ret = CAEN_DGTZ_SetChannelDCOffset(handle, (uint32_t)ch, (uint32_t)((float)(fabs((int)dc[p] - 100))*(655.35))); //ACF: ??
        if (ret) printf("Warning: error setting ch %d test offset\n", ch);

        usleep(200000);

        int value[NACQS] = { 0 };
        for (acq = 0; acq < NACQS; acq++)
        {
         CAEN_DGTZ_SendSWtrigger(handle);
         ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
	     if (ret){  cout<<"ReadData "<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
                    exit(0);
                 }

	     ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, 0, &EventInfo, &EventPtr);
	     if (ret) { cout<<"GetEventInfo "<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
                    return ret;
                   }
	     // decode the event //
	     ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);
	     if (ret) {  cout<<"DecodeEvent "<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
                     return ret;
                  }

	     for (i = 1; i < 7; i++)
	     { //mean over 6 samples
	      value[acq] += (int)(Event16->DataChannel[ch][i]);
         }
	     value[acq] = (value[acq] / 6);

	     }//for acq

        ///check for clean baselines
        int max = 0;
        int mpp = 0;
        int size = (int)pow(2, (double)BoardInfo.ADC_NBits);
        int *freq = (int*)calloc(size, sizeof(int));

        for (k = 0; k < NACQS; k++)
        {
	      if (value[k] > 0 && value[k] < size)
	      {
	       freq[value[k]]++;
	       if (freq[value[k]] > max) { max = freq[value[k]]; mpp = value[k]; }
	      }
        }

        free(freq);
        int ok = 0;
        for (k = 0; k < NACQS; k++)
        {
	      if (value[k] == mpp || value[k] == (mpp + 1) || value[k] == (mpp - 1))
	      {
	        avg_value[p] = avg_value[p] + (float)value[k]; ok++;
	      }
        }

         avg_value[p] = (avg_value[p] / (float)ok)*100. / (float)size;

        }//close for p

        cal = ((float)(avg_value[1] - avg_value[0]) / (float)(dc[1] - dc[0]));
        offset = (float)(dc[1] * avg_value[0] - dc[0] * avg_value[1]) / (float)(dc[1] - dc[0]);
        //printf("Cal %f   offset %f\n", cal, offset);
      }///close if calibration is possible


      if (PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive)
      {
        DCoffset[ch] = (uint32_t)((float)(fabs(( ((float)dc_file[ch] - offset )/ cal ) - 100.))*(655.35));
        if (DCoffset[ch] > 65535) DCoffset[ch] = 65535;
        if (DCoffset[ch] < 0) DCoffset[ch] = 0;
      } else if (PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityNegative)
            {
             DCoffset[ch] = (uint32_t)((float)(fabs(( (fabs(dc_file[ch] - 100.) - offset) / cal ) - 100.))*(655.35));
             if (DCoffset[ch] < 0) DCoffset[ch] = 0;
             if (DCoffset[ch] > 65535) DCoffset[ch] = 65535;
            }

      if(verbose){cout<<"Calibrated DC offset: "<<DCoffset[ch]<<endl;}

      ret = CAEN_DGTZ_SetChannelDCOffset(handle, (uint32_t)ch, DCoffset[ch]);
      if (ret) printf("Warning: error setting ch %d offset\n", ch);

      usleep(200000);
    }//if ch enabled
  }//loop ch

  //printf("DAC Calibration ready\n");

  CAEN_DGTZ_SWStopAcquisition(handle);

  ///free events e buffer
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);

  SetCorrectThreshold();

  CAEN_DGTZ_SetPostTriggerSize(handle, PostTrigger);
  CAEN_DGTZ_SetAcquisitionMode(handle, mem_mode);
  CAEN_DGTZ_SetExtTriggerInputMode(handle, ExtTriggerMode);
  if (ret) printf("Warning: error setting recorded parameters\n");

  CalibComplete=true;

  return CAEN_DGTZ_Success;
}


CAEN_DGTZ_ErrorCode Digitizer::SetCorrectThreshold(){

  CAEN_DGTZ_UINT16_EVENT_t    *Event16;
  int i = 0,ch=0;
  CAEN_DGTZ_ErrorCode ret;
  uint32_t  AllocatedSize;

  uint32_t BufferSize;
  CAEN_DGTZ_EventInfo_t       EventInfo;
  char *buffer = NULL;
  char *EventPtr = NULL;

  ///malloc
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
  if (ret) { return ret; }

  ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
  if (ret != CAEN_DGTZ_Success) { return ret;}

  uint32_t mask;
  CAEN_DGTZ_GetChannelEnableMask(handle, &mask);

  CAEN_DGTZ_SWStartAcquisition(handle);
  for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++)
  {
    if (EnableMask & (1 << ch) && Version_used[ch] == 1)
    {
     if (cal_ok[ch])
     {
      int baseline = 0;
	  CAEN_DGTZ_SendSWtrigger(handle);

      ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
	  if (ret) {cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
                exit(0);
               }

      ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, 0, &EventInfo, &EventPtr);
      if (ret) {cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
                return ret;
                }
	  // decode the event //
	  ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);
	  if (ret) {cout<<errors[abs((int)ret)]<<" (code "<<ret<<")"<<endl;
                return ret;
                }

	      for (i = 1; i < 11; i++) //mean over 10 samples
		{
		  baseline += (int)(Event16->DataChannel[ch][i]);

		}
	      baseline = (baseline / 10);

	      if (PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive)
		Threshold[ch] = (uint32_t)baseline + thr_file[ch];
	      else 	if (PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityNegative)
		Threshold[ch] = (uint32_t)baseline - thr_file[ch];

	      if (Threshold[ch] < 0) Threshold[ch] = 0;
	      int size = (int)pow(2, (double)BoardInfo.ADC_NBits);
	      if (Threshold[ch] > (uint32_t)size) Threshold[ch] = size;
	      if(verbose){
		cout<<"Initial threshold: "<<thr_file[ch]<<endl;
		cout<<"Calibrated threshold: "<<Threshold[ch]<<endl;
	      }

	    }//if cal ok ch
	  else
	    Threshold[ch] = thr_file[ch];


	  ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, ch, Threshold[ch]);
	  if (ret)
	    printf("Warning: error setting ch %d corrected threshold\n", ch);

	  CAEN_DGTZ_SetChannelSelfTrigger(handle, ChannelTriggerMode[ch], (1 << ch));

	}
    }
  CAEN_DGTZ_SWStopAcquisition(handle);

  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);

  return CAEN_DGTZ_Success;
}


void Digitizer::CheckKeyboardCommands(){
  int c = 0;

  if(!kbhit())
    return;

  c = getch();

  switch(c) {
  case 'q' :
    Quit = 1;
    manualStop=true;
    break;
  case 'R' :
    Restart = 1;
    break;
  case 't' :
    if (!ContinuousTrigger) {
      CAEN_DGTZ_SendSWtrigger(handle);
      printf("Single Software Trigger issued\n");
    }
    break;
  case 'T' :
    ContinuousTrigger ^= 1;
    if (ContinuousTrigger)
      printf("Continuous trigger is enabled\n");
    else
      printf("Continuous trigger is disabled\n");
    break;
  case 'w' :
    SingleWrite = 1;
    break;
  case 'W' :
    ContinuousWrite ^= 1;
    if (ContinuousWrite)
      printf("Continuous writing is enabled\n");
    else
      printf("Continuous writing is disabled\n");
    break;
  case 's' :
    if (AcqRun == 0) {
        printf("Acquisition started\n");
        startAcq();
    }
    else {
        printf("Acquisition stopped\n");
        CAEN_DGTZ_SWStopAcquisition(handle);
        AcqRun = 0;
        //Restart = 1;
        }
    break;
  case 'm' :
  case ' ' :
    printf("\n                            Bindkey help                                \n");
    printf("--------------------------------------------------------------------------\n");;
    printf("  [q]   Quit\n");
    printf("  [R]   Reload configuration file and restart\n");
    printf("  [s]   Start/Stop acquisition\n");
    printf("  [t]   Send a software trigger (single shot)\n");
    printf("  [T]   Enable/Disable continuous software trigger\n");
    printf("  [w]   Write one event to output file\n");
    printf("  [W]   Enable/Disable continuous writing to output file\n");
    printf("[SPACE] This help\n");
    printf("--------------------------------------------------------------------------\n");
    printf("Press a key to continue\n");
    getch();
    break;
  default :   break;
  }
}


void Digitizer::startAcq(){

  if(!CalibComplete) Calibrate_DC_Offset();

  if(verbose){
    cout<<"================================================="<<endl;
    printf("Digitizer: Acquisition started\n");
  }

  CAEN_DGTZ_SWStartAcquisition(handle);

  RunStartTime = util::markTime();

  cout.precision(15);
  if(verbose) cout<<"Digitizer: Start time: "<<RunStartTime<<endl;

  fman.setRunStartTime(RunStartTime);

  AcqRun = 1;
}


void Digitizer::printOn(ostream & out) const{
//  out<<"filename \t"<<fname<<endl;
//  out<<"filename \tYYYY_MM_DD_HH_MMtanca.root"<<endl;

  out<<"Duration of run \t";
  if(eventContinuous) out<<" Countinuous data taking\n";
  else if(eventLimit) out<<numOfEvents<<" events\n";
  else if(timeLimit)  out<<DurationOfRun<<" seconds\n";
  else out<<"manual\n";

  out<<"================================================="<<endl;
  out<<"Digitizer Settings:"<<endl;
  if(LinkType==CAEN_DGTZ_USB)
    out<<"LinkType:\t\tUSB"<<endl;

  out<<"LinkNum \t\t"<< LinkNum <<endl;
  out<<"ConetNode \t\t"<<ConetNode  <<endl;
  out<<"BaseAddress \t\t"<<std::hex<< BaseAddress<<std::dec <<endl;
  out<<"Nch \t\t\t"<< Nch <<endl;
  out<<"Nbit \t\t\t"<< Nbit <<endl;
  out<<"Ts \t\t\t"<<Ts  <<endl;
  out<<"RecordLength \t\t"<< RecordLength <<endl;
  out<<"PostTrigger \t\t"<< PostTrigger <<endl;
  out<<"InterruptNumEvents \t"<<InterruptNumEvents  <<endl;
  out<<"TestPattern \t\t"<<TestPattern  <<endl;
  out<<"FPIOtype \t\t"<<FPIOtype  <<endl;
  out<<"ExtTriggerMode \t\t";//<< ExtTriggerMode <<endl;

  if(ExtTriggerMode==CAEN_DGTZ_TRGMODE_DISABLED)
    out<<"Disabled\n";
  else if(ExtTriggerMode==CAEN_DGTZ_TRGMODE_EXTOUT_ONLY)
    out<<"Ext Out Only\n";
  else if(ExtTriggerMode==CAEN_DGTZ_TRGMODE_ACQ_ONLY)
    out<<"Acq only\n";
  else if(ExtTriggerMode==CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT)
    out<<"Acq and Extout\n";

  out<<"Trigger Mode \t\t";
  if(!triggerMaj1) out<<"OR\n";
  else{ out<<"Maj1\n";
    out<<"\tWindow\t\t"<<c_window<<endl;
  }

  bitset<4> mask(EnableMask); //ACF: DT5720B <4>

  out<<"EnableMask \t\t"<<mask <<endl;
  out<<"================================================="<<endl;

  for(int i=0; i<Nch; i++){
    out<<"Channel\t"<<i;
    if(mask[i]==0){
      out<<" Disabled\n";
      continue;
    }
    out<<endl;
    out<<"\tTriggerMode:\t ";
    if(ChannelTriggerMode[i]==CAEN_DGTZ_TRGMODE_DISABLED)
      out<<"Disabled\n";
    else if(ChannelTriggerMode[i]==CAEN_DGTZ_TRGMODE_EXTOUT_ONLY)
      out<<"Ext Out Only\n";
    else if(ChannelTriggerMode[i]==CAEN_DGTZ_TRGMODE_ACQ_ONLY)
      out<<"Acq only\n";
    else if(ChannelTriggerMode[i]==CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT)
      out<<"Acq and Extout\n";

    out<<"\tPulsePolarity\t ";
    if(PulsePolarity[i]==CAEN_DGTZ_PulsePolarityPositive)
      out<<"Positive\n";
    else if(PulsePolarity[i]==CAEN_DGTZ_PulsePolarityNegative)
      out<<"Negative\n";

    out<<"\tTrigger Polarity ";
    if(TrigPolarity[i]==CAEN_DGTZ_TriggerOnRisingEdge)
      out<<"Positive\n";
    else if(TrigPolarity[i]==CAEN_DGTZ_TriggerOnFallingEdge)
      out<<"Negative\n";

    out<<"\tDCoffset \t "<<DCoffset[i]<<endl;
    out<<"\tThreshold \t "<<Threshold[i]<<endl;

  }


}

//overload of << operator
ostream& operator<<(ostream& os, const Digitizer& r) {
  r.printOn(os);
  return os;
}
