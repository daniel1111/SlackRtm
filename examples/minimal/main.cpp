#include <string>
#include <iostream>

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
    cbi_debug_message(LOG_DEBUG, "cbi_got_slack_message> #" + channel + "/<" + username + "> " + message);

    if (message == "ping")
      _rtm->send(channel, "PONG!");

    return 0;
  }
  
  void cbi_debug_message(int log_level, string msg)
  {
    cout << "[" + msg + "]" << endl;
  }
};


int main(int argc, char *argv[])
{
  string apiurl = "https://slack.com/api/";
  
  if (argc != 2)
  {
    cout << "Invalid number of parameters supplied!\n";
    cout << "Expected one parameter: token\n\n";
    return -1;
  }

  string token(argv[1]);

  SlackTest test(token, apiurl);
  test.start();

  return 0;
}
