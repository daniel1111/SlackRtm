#include <stdio.h>
#include <string>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

//#include "CSlackWS.h"
#include "SlackRTMCallbackInterface.h"
#include "CSlackRTM.h"

using namespace std;


class SlackTest : public SlackRTMCallbackInterface
{
public:
  void start()
  {
    CLogging log;
    string token = "xoxb-11832276226-HiO5bl2qSGxdqAhqM5tca90E";
    string ApiUrl = "https://slack.com/api/";

    CSlackRTM SlackRtm(token, ApiUrl, &log);
    SlackRtm.go();
    sleep(10);

  }
  
  int cbiGotSlackMessage(string topic, string message)
  {
    return 0;
  }
  
};


int main()
{
  SlackTest test;
  
  test.start();
  
  return 0;
}
  
