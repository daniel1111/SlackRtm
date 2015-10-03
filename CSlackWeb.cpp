
#include "CSlackWeb.h"

#include <stdio.h>
#include <string>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

using namespace std;



CSlackWeb::CSlackWeb(CLogging *log)
{
  _log = log;
}

void CSlackWeb::dbg(string msg)
{
  _log->dbg(msg);
}

size_t CSlackWeb::s_curl_write(char *data, size_t size, size_t nmemb, void *p)
/* static callback used by cURL when data is recieved. */
{
  ((string*)p)->append(data, size * nmemb);
  return size*nmemb;
}

bool CSlackWeb::slack_rtm_start(std::string url, std::string token)
{
  string curl_read_buffer;
  string get_fields;
  string refresh_token;
  string identity;
  int res;
  CURL *curl;


  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up

  get_fields =  "token="     +  token
              + "&simple_latest=0" 
              + "&no_unreads=1";


  dbg("slack_rtm_start> get fields: [" + get_fields + "]");


  string api_url = url + "?" + get_fields;
  

  dbg("slack_rtm_start> Using URL: [" + api_url + "]");
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg("slack_rtm_start> cURL perform failed: " + (string)_errorBuffer);
    curl_easy_cleanup(curl);
    return false;
  } else
  {
    dbg("slack_rtm_start> Got data");
  }

  dbg("Got: " + curl_read_buffer);
  curl_easy_cleanup(curl);
 
    return false;
}

/*
int main()
{


  slack_rtm_start("https://slack.com/api/rtm.start", "xoxb-11832276226-HiO5bl2qSGxdqAhqM5tca90E");
  return 0;
}
*/