#include <stdio.h>
#include <string>
#include <cerrno>

#include "SlackRTMCallbackInterface.h"
#include "CSlackRTM.h"

using namespace std;


class SlackTest : public SlackRTMCallbackInterface
{
private:
  CLogging *_log;

public:

  SlackTest()
  {
    _log = new CLogging();
  }

  ~SlackTest()
  {
    delete _log;
  }  
  
  void start()
  {
    CLogging log;
    string token = "xoxb-11832276226-HiO5bl2qSGxdqAhqM5tca90E";
    string apiurl = "https://slack.com/api/";

    CSlackRTM SlackRtm(token, apiurl, _log, this);
    SlackRtm.go();
    sleep(10);
    SlackRtm.send("general", "test message");
    sleep(5);

  }
  
  int cbiGotSlackMessage(string channel, string username, string message)
  {
    _log->dbg("cbiGotSlackMessage> #" + channel + "/<" + username + "> " + message);

    return 0;
  }
  
};


int main()
{
  SlackTest test;
  
  test.start();
  
  return 0;
}
  
