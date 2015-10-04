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
  CSlackRTM *_rtm;

public:

  SlackTest(string token, string apiurl)
  {
    _log = new CLogging();
    _rtm = new CSlackRTM(token, apiurl, _log, this);
  }

  ~SlackTest()
  {
    delete _rtm;
    delete _log;
  }

  void start()
  {

    _rtm->go();
    sleep(2);
    _rtm->send("general", "test message");
    sleep(1000);

  }
  
  int cbiGotSlackMessage(string channel, string username, string message)
  {
    _log->dbg("cbiGotSlackMessage> #" + channel + "/<" + username + "> " + message);

    if (message == "ping")
      _rtm->send(channel, "PONG!");

    return 0;
  }
};


int main()
{
  string token = "xoxb-11832276226-HiO5bl2qSGxdqAhqM5tca90E";
  string apiurl = "https://slack.com/api/";

  SlackTest test(token, apiurl);

  test.start();

  return 0;
}
  
