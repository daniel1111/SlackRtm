#include "CSlackRTM.h"

  
using namespace std;

CSlackRTM::CSlackRTM(string token, string api_url, CLogging *log, SlackRTMCallbackInterface *cb)
{
  
  _token = token;
  _api_url = api_url;
  _log = log;
  _cb = cb;
  
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
  //_sws->set_iface("eth0");
  _sws->set_port(443);
  _sws->set_address(server);
  _sws->set_path(server + path);
  _sws->start();
}


int CSlackRTM::send(string channel, string message)
{
  string json_message = json_encode_slack_message(channel, message);

  _sws->ws_send(json_message);

  return 0;
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

  // For now, only really interested in the "message" ("A message was sent to a channel") event
  if (type == "message")
  {
    string user_id   = CSlackWeb::extract_value(message, "user");
    string user_name = _sweb->get_username_from_id(user_id);

    string channel_id   = CSlackWeb::extract_value(message, "channel");
    string channel_name = _sweb->get_channel_from_id(channel_id);

    string text = CSlackWeb::extract_value(message, "text");

//    dbg("slack_callback> #" + channel_name + "/<" + user_name + "> " + text);
    _cb->cbiGotSlackMessage(channel_name, user_name, text);
  }

  return 0;
}

string CSlackRTM::json_encode_slack_message(string channel_name, string text)
/* TODO: JSON encode the post data used to add a notification channel. should look something like:
 *  { "id": "cf32051e-4135-11e5-97d6-3c970e884252", "type": "web_hook", "address": "https:\/\/lspace.nottinghack.org.uk\/temp\/google.php", "token": "1" }
 */
{
  int    msg_id     = get_next_msg_id();
  string channel_id = _sweb->get_id_from_channel(channel_name);

  json_object *j_obj_root   = json_object_new_object();
  json_object *jstr_id      = json_object_new_int(msg_id);
  json_object *jstr_type    = json_object_new_string("message");
  json_object *jstr_channel = json_object_new_string(channel_id.c_str());
  json_object *jstr_text    = json_object_new_string(text.c_str());

  json_object_object_add(j_obj_root, "id"     , jstr_id);
  json_object_object_add(j_obj_root, "type"   , jstr_type);
  json_object_object_add(j_obj_root, "channel", jstr_channel);
  json_object_object_add(j_obj_root, "text"   , jstr_text);

  string json_encoded = json_object_to_json_string(j_obj_root);

  json_object_put(j_obj_root);  

  return json_encoded;
}

int CSlackRTM::get_next_msg_id()
{
  static int msg_id = 1;
  return msg_id++;
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
