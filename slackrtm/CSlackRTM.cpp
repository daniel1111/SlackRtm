/*
 * Copyright (c) 2015, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "CSlackRTM.h"

  
using namespace std;

CSlackRTM::CSlackRTM(string token, string api_url, SlackRTMCallbackInterface *cb)
{
  
  _token = token;
  _api_url = api_url;
  _cb = cb;
  
  // Stuff for activity monitoring thread
  pthread_mutex_init(&_act_mutex, NULL);
  pthread_cond_init(&_act_condition_var, NULL);
  
  _last_msg_received = -1;
  _act_thread = 0;
  _act_thread_msg = ACT_MSG_NOTHING;
  
  _sweb = NULL;
  _sws  = NULL;
  
  dbg(LOG_DEBUG, "Created");
}

CSlackRTM::~CSlackRTM()
{
  // Signal act_thread to stop
  if (_act_thread)
  {
    dbg(LOG_DEBUG, "Signal activity act_thread to stop");
    pthread_mutex_lock(&_act_mutex);
    _act_thread_msg = ACT_MSG_EXIT;
    pthread_cond_signal(&_act_condition_var);
    pthread_mutex_unlock(&_act_mutex);

    dbg(LOG_DEBUG, "Join act_thread");
    pthread_join(_act_thread, NULL);
  }

  if (_sws != NULL)
    delete _sws;
  
  if (_sweb != NULL)
    delete _sweb;

  dbg(LOG_DEBUG, "Done.");
}

void CSlackRTM::dbg(int dbglvl, string msg)
{
  _cb->cbi_debug_message(dbglvl, "CSlackRTM::" + msg);
}

void CSlackRTM::go()
{
  connect_to_slack();

  // Create thead to monitor connection
  pthread_create(&_act_thread, NULL, &CSlackRTM::activity_thread, this);

}

int CSlackRTM::connect_to_slack()
{
  string URL="";
  
  // For web api
  _sweb = new CSlackWeb(_api_url, _token, _cb);

  // For real time message / websocket
  _sws  = new CSlackWS(&CSlackRTM::s_slack_callback, this, _cb);  

  // Use the web API to get a web service URL for the RTM interface
  _sweb->init();
  URL = _sweb->get_ws_url();
  if (URL == "")
  {
    dbg(LOG_ERR, "connect_to_slack(): Failed to get web service URL");
    return -1;
  }

  // Get server + path from URL
  string server;
  string path;
  if (split_url(URL, server, path))
    return -1;

  // Connect to the websocket
//_sws->set_iface("eth0");
  _sws->set_port(443);
  _sws->set_address(server);
  _sws->set_path(path);
  _sws->start();
  
  _last_msg_received = time(NULL);
  return 0;
}


int CSlackRTM::send(string channel, string message)
{
  
  if (_sws != NULL)
  {
    
    message = escape_for_slack(message);
    
    string channel_id = _sweb->get_id_from_channel(channel);
    string json_message = json_encode_slack_message(channel_id, message);

    _sws->ws_send(json_message);
    return 0;
  }
  else
    return -1;
}

int CSlackRTM::send_dm(std::string username, std::string message)
/* Send a direct message to <username> */
{
  
  if (_sws != NULL)
  {
    message = escape_for_slack(message);

    // get user id
    string user_id = _sweb->get_id_from_username(username);
    if (user_id == "")
      return -1;

    // Lookup DM channel for that user
    string channel_id = _sweb->get_id_from_channel(user_id);
    if (channel_id == "")
    {
      // We don't know the DM channel ID for the user (probably not DM'ed them before). So use the web API
      // to request one. It will be created for us, if required, otherwise the existing one will be returned.
      _sweb->slack_im_open(user_id, channel_id);
    }

    string json_message = json_encode_slack_message(channel_id, message);

    _sws->ws_send(json_message);
    return 0;
  }
  else
    return -1;
}

int CSlackRTM::s_slack_callback(string message, void *obj) /* Static */
{
  CSlackRTM *s;

  s = (CSlackRTM*)obj;
  return s->slack_callback(message);
}

int CSlackRTM::slack_callback(string message)
{
  if (_sweb == NULL)
  {
    dbg(LOG_ERR, "slack_callback> _sweb is NULL!");
    return -1;
  }

  string type = CSlackWeb::extract_value(message, "type");

  pthread_mutex_lock(&_act_mutex); 
  _last_msg_received = time(NULL);
  pthread_mutex_unlock(&_act_mutex); 
  
  // For now, only really interested in the "message" ("A message was sent to a channel") event
  if (type == "message")
  {
    // For now, ignore replies to our messages from slack
    string reply_to = CSlackWeb::extract_value(message, "reply_to");

    if (reply_to.length() > 0)
    {
      dbg(LOG_DEBUG, "Ignoring reply messge");
      return 0;
    }

    string user_id   = CSlackWeb::extract_value(message, "user");
    string user_name = _sweb->get_username_from_id(user_id);

    string channel_id   = CSlackWeb::extract_value(message, "channel");

    // if the first character of the channel name is "D", then it was direct message
    string channel_name;
    if (channel_id.substr(0,1) == "D")
      channel_name = ""; // DM, set channel name to blank
    else
      channel_name = _sweb->get_channel_from_id(channel_id);

    string text = CSlackWeb::extract_value(message, "text");

    // Pass the message on to the callback we were passed on construction
    _cb->cbi_got_slack_message(channel_name, user_name, escape_from_slack(text));
  }
  
  // New user has joined the slack team. Need to add their user id / name mapping to the cache
  else if (type == "team_join")
  {
    string user_name="";
    string user_id="";
    if (!extract_user_details(message, user_name, user_id))
    {
      // add to cache
      dbg(LOG_INFO, "New user joined the team: id=[" + user_id + "], name=[" + user_name + "]");
      if (_sweb != NULL)
        _sweb->add_user(user_id, user_name);
    }
  }

  // Channel created - add channel id -> channel mappping
  else if (type == "channel_created")
  {
    // extract channel name / ID from message
    string channel_name="";
    string channel_id="";
    if (!extract_channel_details(message, channel_name, channel_id))
    {
      // add to cache
      dbg(LOG_INFO, "Channel created: id=[" + channel_id + "], name=[" + channel_name + "]");
      if (_sweb != NULL)
        _sweb->add_channel(channel_id, channel_name);
    }
  }

  return 0;
}

int CSlackRTM::extract_channel_details(string message, string &channel_name, string &channel_id)
{
  json_object *jobj = json_tokener_parse(message.c_str());
  if (jobj == NULL)
  {
    dbg(LOG_ERR, "extract_channel_details> json_tokener_parse failed. JSON data: [" + message + "]");
    return -1;
  }

  json_object *obj_param = NULL;
  if (!json_object_object_get_ex(jobj, "channel", &obj_param))
  {
    dbg(LOG_ERR, "extract_channel_details> json_object_object_get_ex failed. JSON data: [" + message + "]");
    json_object_put(jobj);
    return -1;
  }

  json_object *channel_obj_name = NULL;
  if (json_object_object_get_ex(obj_param, "name", &channel_obj_name))
    channel_name = json_object_get_string(channel_obj_name);

  json_object *channel_obj_id   = NULL;
  if (json_object_object_get_ex(obj_param, "id", &channel_obj_id))
    channel_id   = json_object_get_string(channel_obj_id);

  json_object_put(obj_param);
  json_object_put(jobj);

  return 0;
}

int CSlackRTM::extract_user_details(string message, string &user_name, string &user_id)
{
  json_object *jobj = json_tokener_parse(message.c_str());
  if (jobj == NULL)
  {
    dbg(LOG_ERR, "extract_user_details> json_tokener_parse failed. JSON data: [" + message + "]");
    return -1;
  }

  json_object *obj_param = NULL;
  if (!json_object_object_get_ex(jobj, "user", &obj_param))
  {
    dbg(LOG_ERR, "extract_user_details> json_object_object_get_ex failed. JSON data: [" + message + "]");
    json_object_put(jobj);
    return -1;
  }

  json_object *user_obj_name = NULL;
  json_object *user_obj_id   = NULL;

  if (json_object_object_get_ex(obj_param, "name", &user_obj_name))
    user_name = json_object_get_string(user_obj_name);

  if (json_object_object_get_ex(obj_param, "id", &user_obj_id))
    user_id   = json_object_get_string(user_obj_id);

  json_object_put(obj_param);
  json_object_put(jobj);

  return 0;
}

string CSlackRTM::json_encode_slack_message(string channel_id, string text)
{
  if (_sweb == NULL)
  {
    dbg(LOG_ERR, "json_encode_slack_message> _sweb is NULL!");
    return "";
  }

  int    msg_id     = get_next_msg_id();

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

string CSlackRTM::json_encode_slack_ping()
{
  int msg_id = get_next_msg_id();

  json_object *j_obj_root   = json_object_new_object();
  json_object *jstr_id      = json_object_new_int(msg_id);
  json_object *jstr_type    = json_object_new_string("ping");

  json_object_object_add(j_obj_root, "id"     , jstr_id);
  json_object_object_add(j_obj_root, "type"   , jstr_type);

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

string CSlackRTM::escape_for_slack(string message)
/* Perform replacements:
 * 
 * from  to
 * <     &lt;
 * >     &gt; 
 * &     &amp;
 */
{
  string result = "";

  char* str = (char*)message.c_str(); // utf-8 string
  char* str_i = str;                  // string iterator
  char* end = str+strlen(str)+1;      // end iterator

  do
  {
    uint32_t code = utf8::next(str_i, end);
    if (code == 0)
      continue;

    if (code == '<')
      result += "&lt;";
    else if (code == '>')
      result += "&gt;";
    else if (code == '&')
      result += "&amp;";
    else
      utf8::append(code, std::back_inserter(result));
  }
  while (str_i < end);

  return result;
}

string CSlackRTM::escape_from_slack(string message)
{
  string ret = message;

  boost::replace_all(ret, "&lt;" , "<");
  boost::replace_all(ret, "&gt;" , ">");
  boost::replace_all(ret, "&amp;", "&");

  return ret;
}

void *CSlackRTM::activity_thread(void *arg)
{
  CSlackRTM *rtm;
  rtm = (CSlackRTM*) arg;
  rtm->activity_thread();
  return NULL;
}

void CSlackRTM::activity_thread()
/* Thread to monitor the state of the connection. If there has been no message of any kind received on the web socket in 
 * the last minute, send a PING message. If still nothing received after a further 30 seconds, trigger a reconnect.
 */
{
  int timeout = 10; // (15*60); 
  int ping_timout = 5;
  struct timespec to;
  int err;
  time_t wait_until;
  char buf[30] = "";
  act_msg msg;
  bool ping_sent = false;
  dbg(LOG_DEBUG, "Entered activity_thread");

  while (1)
  {
    pthread_mutex_lock(&_act_mutex); 

    if (ping_sent)
      wait_until = time(NULL) + ping_timout;
    else
      wait_until = _last_msg_received + timeout;

    to.tv_sec = wait_until+2;
    to.tv_nsec = 0; 

    if (_act_thread_msg == ACT_MSG_NOTHING)
    {
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&wait_until));
      //dbg("activity_thread> Waiting until: " + (string)buf);
    }

    while (_act_thread_msg == ACT_MSG_NOTHING) 
    {
      err = pthread_cond_timedwait(&_act_condition_var, &_act_mutex, &to);
      if (err == ETIMEDOUT) 
      {
        //dbg("activity_thread> Timeout reached");
        break;
      }
    }
    msg = _act_thread_msg;
    if (_act_thread_msg != ACT_MSG_EXIT)
      _act_thread_msg = ACT_MSG_NOTHING;

    pthread_mutex_unlock(&_act_mutex);

    if (msg == ACT_MSG_EXIT)
    {
      dbg(LOG_DEBUG, "exiting activity_thread");
      return;
    }
    
    if ((time(NULL) - _last_msg_received) < timeout)
    {
      // We've received a message within the last 60 seconds... so nothing to do.
      ping_sent = false;
      continue;
    }

    if (ping_sent)
    {
      // nothing's been received within the timeout period, a ping's been sent, and we've still not got anything back. 
      // Time to reconnect.
      dbg(LOG_NOTICE, "RECONNECT REQUIRED");
      
      // Taredown connection, and start again
      if (_sws != NULL)
        delete _sws;
      
      if (_sweb != NULL)
        delete _sweb;
      
      connect_to_slack();
      
    }
    else
    {
      // Nothing received within the timeout period. This is probably normal - i.e. just a case of no-one saying anything.
      // Send a ping to check the connection is alive (server should respond near-instantly)
      if (_sws != NULL)
      {
        _sws->ws_send(json_encode_slack_ping());
      } else
      {
        dbg(LOG_DEBUG, "activity_thread> _sws is NULL, can't send ping"); 
        // still mark ping as sent, so we timout the connection in the hope of recovering from this
      }
      ping_sent = true;
    }
  }
}
