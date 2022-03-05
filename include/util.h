#include <sys/time.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

#ifndef UTIL_h
#define UTIL_h

class util{

 public:

 static int hour(){
  time_t time_now;
  struct tm * time_str;
  time (&time_now);
  time_str = gmtime(&time_now);
  int hora = time_str->tm_hour;
  return hora;
 }

  static int minut(){
  time_t time_now;
  struct tm * time_str;
  time (&time_now);
  time_str = gmtime(&time_now);
  int minuto = time_str->tm_min;
  return minuto;
 }
// static string dirName1(){
//  stringstream ssY, ssM, ssD;
//  string dateS;
//  time_t time_now;
//  struct tm * time_str;
//  time (&time_now);
//  time_str = gmtime(&time_now);
//  ssY<< int(time_str->tm_year + 1900);
//  ssM<<int(time_str->tm_mon + 1);
//  ssD<<int(time_str->tm_mday);
//  dateS =ssY.str()+ssM.str()+ssD.str();
//  return dateS;
//}
 static string dirName(){
  string dateS,strY,strM,strD;
  time_t time_now;
  struct tm * time_str;
  time (&time_now);
  time_str = gmtime(&time_now);
  strY = to_string(int(time_str->tm_year + 1900));
  strM = to_string(int(time_str->tm_mon + 1));
  if(strM.length()==1)strM="0"+strM;
  strD = to_string(int(time_str->tm_mday));
  if(strD.length()==1)strD="0"+strD;
  dateS =strY+"_"+strM+"_"+strD;
  return dateS;
}

 static string HoraMin(){
  string dateS,strH,strM;
  time_t time_now;
  struct tm * time_str;
  time (&time_now);
  time_str = gmtime(&time_now);
  strH = to_string(int(time_str->tm_hour));
  if(strH.length()==1)strH="0"+strH;
  strM = to_string(int(time_str->tm_min));
  if(strM.length()==1)strM="0"+strM;
  dateS =strH+"_"+strM;
  return dateS;
}

  static long get_time(){
    long time_ms;
    struct timeval t1;
    gettimeofday(&t1, NULL);
    time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
    return time_ms;
  }

  static double markTime() {
    struct timeval timeMark;
    gettimeofday(&timeMark,NULL);
    return (double)timeMark.tv_sec + (double)timeMark.tv_usec/1000000.;
  }

  static vector<string> split(const char *str, char c = ' ')
    {
      vector<string> result;

      do
	{
	  const char *begin = str;

	  while(*str != c && *str)
            str++;

	  result.push_back(string(begin, str));
	} while (0 != *str++);

      return result;
    }

};

#endif
