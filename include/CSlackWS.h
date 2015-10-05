#include "CWebSocket.h"
#include "SlackRTMCallbackInterface.h"

class CSlackWS : public CWebSocket
{

public:
  CSlackWS(int (*slack_callback)(std::string, void*), void *cb_param, SlackRTMCallbackInterface *cb);
  
  
  int got_data(std::string data);
  int start();
  
private:
  int (*_slack_callback)(std::string, void*);
  void *_cb_param;
  SlackRTMCallbackInterface *_cb;
};