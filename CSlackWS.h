#include "CWebSocket.h"

class CSlackWS : public CWebSocket
{

public:
  CSlackWS(CLogging *log, int (*slack_callback)(string, void*), void *cb_param);
  
  
  int got_data(std::string data);
  int start();
  
private:
  CLogging *_log;
  int (*_slack_callback)(string, void*);
  void *_cb_param;
};