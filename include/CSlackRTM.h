#include "CSlackWS.h"
#include "CSlackWeb.h"
#include "SlackRTMCallbackInterface.h"

#include <string>
  
class CSlackRTM
{


public:
  CSlackRTM(std::string token, std::string api_url, SlackRTMCallbackInterface *cb);
  ~CSlackRTM();
  void go();
  int send(std::string channel, std::string message);

private:
  int split_url(std::string url, std::string &server, std::string &path);
  void dbg(std::string msg);
  
  static int s_slack_callback(std::string message, void *obj);
  int slack_callback(std::string message);
  std::string json_encode_slack_message(std::string channel_name, std::string text);
  std::string json_encode_slack_ping();
  int get_next_msg_id();
  int connect_to_slack();

  time_t _last_msg_received;
  std::string _api_url;
  std::string _token;
  CSlackWS *_sws;
  CSlackWeb *_sweb;
  SlackRTMCallbackInterface *_cb;
  
  
  enum act_msg 
  {
    ACT_MSG_NOTHING,
    ACT_MSG_EXIT
  };  
  static void *activity_thread(void *arg);
  void activity_thread();
  pthread_t _act_thread;
  pthread_mutex_t _act_mutex;
  pthread_cond_t  _act_condition_var;
  act_msg _act_thread_msg;
  
};



 