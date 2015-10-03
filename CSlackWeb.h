
#include <stdio.h>
#include <string>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

#include "CLogging.h"

class CSlackWeb
{
public:
  CSlackWeb(CLogging *log, std::string _ApiUrl, std::string _token);
  int get_ws_url(std::string &url);
  

private:
  int slack_rtm_start(std::string &payload);
  static size_t s_curl_write(char *data, size_t size, size_t nmemb, void *p);
  std::string extract_value(string json_in, string param);
  
  std::string extract_users(std::string json_in);
  
  void dbg(std::string msg);

  CLogging *_log;
  char _errorBuffer[CURL_ERROR_SIZE];
  
  std::string _ApiUrl;
  std::string _token;
};

