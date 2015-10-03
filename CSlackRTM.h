#include "CSlackWS.h"
#include "CSlackWeb.h"
#include "CLogging.h"

#include <string>
  
class CSlackRTM
{


public:
  CSlackRTM(std::string token, std::string api_url, CLogging *log);
  ~CSlackRTM();
  void go();

private:
  int get_ws_url(std::string &URL);
  int split_url(std::string url, std::string &server, std::string &path);
  void dbg(std::string msg);
  
  static int s_slack_callback(std::string message, void *obj);
  int slack_callback(std::string message);
  
  std::string _api_url;
  std::string _token;
  CLogging *_log;
  CSlackWS *_sws;
  
};



 