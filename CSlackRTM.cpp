#include "CSlackRTM.h"

  
using namespace std;

CSlackRTM::CSlackRTM(string token, string api_url, CLogging *log)
{
  
  _token = token;
  _api_url = api_url;
  _log = log;
  _sws = new CSlackWS(_log, &CSlackRTM::s_slack_callback, this);
  
}

CSlackRTM::~CSlackRTM()
{
  delete _sws;
}


void CSlackRTM::dbg(string msg)
{
  _log->dbg(msg);
}



void CSlackRTM::go()
{
  // Use the web API to get a web service URL for the RTM interface
  string URL="";
  if (get_ws_url(URL))
  {
    dbg("CSlackRTM::go(): Failed to get web service URL");
    return;
  }

  // Get server + path from URL
  string server;
  string path;
  if (split_url(URL, server, path))
    return;

  // Connect to the websocket
  _sws->set_iface("eth0");
  _sws->set_port(443);
  _sws->set_address(server);
  _sws->set_path(server + path);
  _sws->start();

 
}


int CSlackRTM::s_slack_callback(string message, void *obj) /* Static */
{
  CSlackRTM *s;

  s = (CSlackRTM*)obj;
  return s->slack_callback(message);

}

int CSlackRTM::slack_callback(string message)
{
  dbg("slack_callback> " + message);

  return 0;
}


int CSlackRTM::get_ws_url(string &URL)
{
  CSlackWeb sw(_log, _api_url, _token);
  
  return sw.get_ws_url(URL);
}

int CSlackRTM::split_url(string url, string &server, string &path)
/* Take the wss URL, e.g.:
 * wss://ms404.slack-msgs.com/websocket/yIKU...
 * 
 * and return:
 *  server = ms404.slack-msgs.com
 *  path   = /websocket/yIKU.....
 */
{
  // sanity check length
  if (url.length() < 10)
    return -1;
  
  unsigned int addr_start = url.find_first_of("://");
  if (addr_start == string::npos)
    return -1;
  addr_start += 3; // "://"
  
  // Sanity check, this should never be true with a valid url
  if ((addr_start+2) > url.length())
    return -1;
  
  unsigned int path_start = url.find_first_of("/", addr_start);
  if (path_start == string::npos)
    return -1;

  server = url.substr(addr_start, path_start-addr_start);
  path   = url.substr(path_start, string::npos);
  
  return 0;
}

