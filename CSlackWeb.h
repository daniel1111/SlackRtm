
#include <stdio.h>
#include <string>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

#include "CLogging.h"

class CSlackWeb
{
public:
  CSlackWeb(CLogging *log);
  bool slack_rtm_start(std::string url, std::string token);
  
private:
  static size_t s_curl_write(char *data, size_t size, size_t nmemb, void *p);
  void dbg(std::string msg);
  
  CLogging *_log;
  char _errorBuffer[CURL_ERROR_SIZE];



};

