#include "CSlackRTM.h"

  
using namespace std;

CSlackRTM::CSlackRTM(string token, string api_url, CLogging *log)
{
  
  _token = token;
  _api_url = api_url;
  _log = log;
  
  // For web api
  _sweb = new CSlackWeb(_log, _api_url, _token);

  // For real time message / websocket
  _sws  = new CSlackWS(_log, &CSlackRTM::s_slack_callback, this);
}

CSlackRTM::~CSlackRTM()
{
  delete _sws;
  delete _sweb;
}


void CSlackRTM::dbg(string msg)
{
  _log->dbg(msg);
}



void CSlackRTM::go()
{
  string URL="";

  // Use the web API to get a web service URL for the RTM interface
  _sweb->init();
  URL = _sweb->get_ws_url();
  if (URL == "")
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
  string type = CSlackWeb::extract_value(message, "type");
  dbg("slack_callback> type = [" + type + "]");

  if (type == "message")
  {
    string user_id   = CSlackWeb::extract_value(message, "user");
    string user_name = _sweb->get_username_from_id(user_id);

    string channel_id   = CSlackWeb::extract_value(message, "channel");
    string channel_name = _sweb->get_channel_from_id(channel_id);

    string text = CSlackWeb::extract_value(message, "text");

    dbg("slack_callback> #" + channel_name + "/<" + user_name + "> " + text);
  }

  return 0;
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

