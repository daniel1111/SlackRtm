#include "CSlackWS.h"

using namespace std;


CSlackWS::CSlackWS(CLogging *log, int (*slack_callback)(string, void*), void *cb_param) : CWebSocket(log)
{
  _slack_callback = slack_callback;
  _cb_param = cb_param;
}

int CSlackWS::got_data(string data)
{
  _slack_callback(data, _cb_param);
}
  
int CSlackWS::start()
{
  // Connect to web service. 
  if (ws_open())
    return -1;

  // Start service thread
  return service();

    /*
    while (CWebSocket::status == CWebSocket::CONNECTED)
    {
      sleep(1);
    } 
    sleep (1);
  } // while(1)
  */

}
