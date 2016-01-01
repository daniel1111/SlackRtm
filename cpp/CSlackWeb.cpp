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

#include "CSlackWeb.h"

#include <stdio.h>
#include <string>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

using namespace std;



CSlackWeb::CSlackWeb(string ApiUrl, string token, SlackRTMCallbackInterface *cb)
{
  _ApiUrl = ApiUrl;
  _token = token;
  _ws_url = "";
  _cb = cb;
}

void CSlackWeb::dbg(int dbglvl, string msg)
{
  _cb->cbi_debug_message(dbglvl, "CSlackWeb::" + msg);
}

int CSlackWeb::init()
{
  string payload;
  
  // Get data from slack
  if (slack_rtm_start(payload))
    return -1;

  _ws_url = extract_value(payload, "url");

  // populate _users
  extract_users(payload);

  // populate _channels
  _channels.clear();
  extract_channels(payload, false); // get "normal" channels
  
  extract_channels(payload, true);  // get Direct Message channels

  return 0;
}

string CSlackWeb::get_ws_url()
{
  return _ws_url;
}

string CSlackWeb::get_username_from_id(string user_id)
{
  return _users[user_id];
}

string CSlackWeb::get_channel_from_id(string channel_id)
{
  return _channels[channel_id];
}

string CSlackWeb::get_id_from_channel(string channel_name)
{
  for (map<string,string>::iterator ii=_channels.begin(); ii!=_channels.end(); ++ii)
  {
    if ((*ii).second == channel_name)
      return (*ii).first;
  }
  return "";
}

string CSlackWeb::get_id_from_username(string username)
{
  for (map<string,string>::iterator ii=_users.begin(); ii!=_users.end(); ++ii)
  {
    if ((*ii).second == username)
      return (*ii).first;
  }
  return "";
}

size_t CSlackWeb::s_curl_write(char *data, size_t size, size_t nmemb, void *p)
/* static callback used by cURL when data is recieved. */
{
  ((string*)p)->append(data, size * nmemb);
  return size*nmemb;
}

int CSlackWeb::slack_rtm_start(string &payload)
{
  string get_fields;
  int res;
  CURL *curl;

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &payload);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up

  get_fields =  "token="     +  _token
              + "&simple_latest=0" 
              + "&no_unreads=1";

  dbg(LOG_DEBUG, "slack_rtm_start> get fields: [" + get_fields + "]");

  string ApiUrl = _ApiUrl + "rtm.start?" + get_fields;

  dbg(LOG_DEBUG, "slack_rtm_start> Using URL: [" + ApiUrl + "]");
  payload = "";
  curl_easy_setopt(curl, CURLOPT_URL, ApiUrl.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg(LOG_ERR, "slack_rtm_start> cURL perform failed: " + (string)_errorBuffer);
    curl_easy_cleanup(curl);
    return -1;
  } else
  {
    dbg(LOG_DEBUG, "slack_rtm_start> Got data");
  }

// dbg(LOG_DEBUG, "Got: " + payload);
  curl_easy_cleanup(curl);

  return 0;
}

int CSlackWeb::slack_im_open(string user_id, string &dm_channel)
/* Open a DM channel to <user_id>. Either a new DM channel is created, or the existing one is returned */
{
  string get_fields;
  string payload = "";
  int res;
  CURL *curl;

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &payload);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up

  get_fields =  "token="     + _token
              + "&user="     + user_id;

  dbg(LOG_DEBUG, "slack_im_open> get fields: [" + get_fields + "]");

  string ApiUrl = _ApiUrl + "im.open?" + get_fields;

  dbg(LOG_DEBUG, "slack_im_open> Using URL: [" + ApiUrl + "]");
  payload = "";
  curl_easy_setopt(curl, CURLOPT_URL, ApiUrl.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg(LOG_ERR, "slack_im_open> cURL perform failed: " + (string)_errorBuffer);
    curl_easy_cleanup(curl);
    return -1;
  } else
  {
    dbg(LOG_DEBUG, "slack_im_open> Got data");
  }

  dbg(LOG_DEBUG, "slack_im_open> Got: " + payload);

  string channel_data = extract_value(payload, "channel");
  dm_channel          = extract_value(channel_data, "id");
  add_channel(dm_channel, user_id);

  curl_easy_cleanup(curl);

  return 0;
}

string CSlackWeb::extract_value(string json_in, string param)
/* Extract a value/string from a JSON formatting string. e.g. for JSON string:
 *
 * {"access_token" : "yxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxA","token_type" : "Bearer","expires_in" : 3600}
 * 
 * param = "token_type" would return "Bearer"
 */
{
  json_object *jobj = json_tokener_parse(json_in.c_str());
  if (jobj == NULL)
  {
    // dbg("extract_value> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return "";
  }

  json_object *obj_param = NULL;
  if (!json_object_object_get_ex(jobj, param.c_str(), &obj_param))
  {
    json_object_put(jobj);
    return "";
  }

  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    // dbg("extract_value> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return "";
  }

  // dbg("extract_value> got [" + param + "] = [" + (string)param_val + "]");
  string result = (string)param_val;

  json_object_put(obj_param);
  json_object_put(jobj);

  return result;
}

int CSlackWeb::extract_users(string json_in)
/*
 */
{
  json_object *jobj = json_tokener_parse(json_in.c_str());
  if (jobj == NULL)
  {
    dbg(LOG_ERR, "extract_users> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return -1;
  }

  json_object *obj_param = NULL;
  if (!json_object_object_get_ex(jobj, "users", &obj_param))
  {
    dbg(LOG_ERR, "extract_users> json_object_object_get_ex failed. JSON data: [" + json_in + "]");
    json_object_put(jobj);
    return -1;
  }
  
  // Expecting "users" to be an array, check it is.
  enum json_type type;
  if (json_object_get_type(obj_param) != json_type_array)
  {
    dbg(LOG_ERR, "extract_users> Error: \"users\" isn't an array");
    json_object_put(jobj);
    return -1;
  }
  
  json_object *jarray = obj_param;
  unsigned int user_array_length = json_object_array_length(jarray);
  
  
  _users.clear();
  // Loop through array of user objects
  for (unsigned int idx = 0; idx < user_array_length; idx++)
  {
    const char *user_obj_id_str;
    const char *user_obj_name_str;

    json_object *user_obj = json_object_array_get_idx(jarray, idx); 

    // get id
    json_object *user_obj_id = NULL;
    if (!json_object_object_get_ex(user_obj, "id", &user_obj_id))
      continue;
    user_obj_id_str = json_object_get_string(user_obj_id);

    // get name
    json_object *user_obj_name = NULL;
    if (!json_object_object_get_ex(user_obj, "name", &user_obj_name))
      continue;
    user_obj_name_str = json_object_get_string(user_obj_name);

    dbg(LOG_DEBUG, "id: " + (string)user_obj_id_str + ", name: " + (string)user_obj_name_str);
    _users[(string)user_obj_id_str] = (string)user_obj_name_str;
  }

  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    dbg(LOG_ERR, "extract_users> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return -1;
  }

  json_object_put(obj_param);
  json_object_put(jobj);

  return 0;
}

int CSlackWeb::extract_channels(string json_in, bool dm)
/*
 */
{
  string obj_name;
  if (dm)
    obj_name = "ims";
  else
    obj_name = "channels";
  
  
  json_object *jobj = json_tokener_parse(json_in.c_str());
  if (jobj == NULL)
  {
    dbg(LOG_ERR, "extract_channels> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return -1;
  }

  json_object *obj_param = NULL;
  if (!json_object_object_get_ex(jobj, obj_name.c_str(), &obj_param))
  {
    dbg(LOG_ERR, "extract_channels> json_object_object_get_ex failed for [" + obj_name + "] JSON data: [" + json_in + "]");
    json_object_put(jobj);
    return -1;
  }
  
  // Expecting "channels" to be an array, check it is.
  enum json_type type;
  if (json_object_get_type(obj_param) != json_type_array)
  {
    dbg(LOG_ERR, "extract_channels> Error: \"" + obj_name + "\" isn't an array");
    json_object_put(jobj);
    return -1;
  }
  
  json_object *jarray = obj_param;
  unsigned int channel_array_length = json_object_array_length(jarray);

  
  // Loop through array of channel objects
  for (unsigned int idx = 0; idx < channel_array_length; idx++)
  {
    const char *channel_obj_id_str;
    const char *channel_obj_name_str;

    json_object *channel_obj = json_object_array_get_idx(jarray, idx); 

    // get id
    json_object *channel_obj_id = NULL;
    if (!json_object_object_get_ex(channel_obj, "id", &channel_obj_id))
      continue;
    channel_obj_id_str = json_object_get_string(channel_obj_id);

    // get name (or user for DMs)
    json_object *channel_obj_name = NULL;
    if (!json_object_object_get_ex(channel_obj, (dm ? "user" : "name"), &channel_obj_name))
      continue;
    channel_obj_name_str = json_object_get_string(channel_obj_name);

    dbg(LOG_DEBUG, "id: " + (string)channel_obj_id_str + ", name: " + (string)channel_obj_name_str);
    _channels[(string)channel_obj_id_str] = (string)channel_obj_name_str;
  }

  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    dbg(LOG_ERR, "extract_channels> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return -1;
  }

  json_object_put(obj_param);
  json_object_put(jobj);

  return 0;
}

int CSlackWeb::add_channel(string channel_id, string channel_name)
{
  _channels[channel_id] = channel_name;
  return 0;
}

int CSlackWeb::add_user(string user_id, string user_name)
{
  _users[user_id] = user_name;
  return 0;
}