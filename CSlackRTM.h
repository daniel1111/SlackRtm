#include "CSlackWS.h"
#include "CSlackWeb.h"
#include "SlackRTMCallbackInterface.h"
#include "CLogging.h"

#include <string>
  
class CSlackRTM
{


public:
  CSlackRTM(std::string token, std::string api_url, CLogging *log, SlackRTMCallbackInterface *cb);
  ~CSlackRTM();
  void go();
  int send(std::string channel, std::string message);

private:
  int split_url(std::string url, std::string &server, std::string &path);
  void dbg(std::string msg);
  
  static int s_slack_callback(std::string message, void *obj);
  int slack_callback(std::string message);
  std::string json_encode_slack_message(std::string channel_name, std::string text);
  int get_next_msg_id();
  
  
  std::string _api_url;
  std::string _token;
  CLogging *_log;
  CSlackWS *_sws;
  CSlackWeb *_sweb;
  SlackRTMCallbackInterface *_cb;
  
};



 