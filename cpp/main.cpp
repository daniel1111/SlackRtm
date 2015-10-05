#include <stdio.h>
#include <string>
#include <cerrno>
#include <iostream>

#include "SlackRTMCallbackInterface.h"
#include "CSlackRTM.h"

using namespace std;


class SlackTest : public SlackRTMCallbackInterface
{
private:
  CSlackRTM *_rtm;

public:

  SlackTest(string token, string apiurl)
  {
    _rtm = new CSlackRTM(token, apiurl, this);
  }

  ~SlackTest()
  {
    delete _rtm;
  }

  void start()
  {

    _rtm->go();
    sleep(2);
    _rtm->send("general", "test message");
    sleep(1000);

  }

  int cbi_got_slack_message(string channel, string username, string message)
  {
    cbi_debug_message("cbi_got_slack_message> #" + channel + "/<" + username + "> " + message);

    if (message == "ping")
      _rtm->send(channel, "PONG!");

    return 0;
  }
  
  void cbi_debug_message(string msg)
  {
    cout << "[" + msg + "]" << endl;
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
