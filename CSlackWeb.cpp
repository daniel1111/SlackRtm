
#include "CSlackWeb.h"

#include <stdio.h>
#include <string>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

using namespace std;



CSlackWeb::CSlackWeb(CLogging *log, string ApiUrl, string token)
{
  _log = log;
  _ApiUrl = ApiUrl;
  _token = token;
}

void CSlackWeb::dbg(string msg)
{
  if (_log)
    _log->dbg(msg);
}

int CSlackWeb::get_ws_url(std::string &url)
{
  int ret;
  string payload;

  // Get data from slack
  ret = slack_rtm_start(payload);
  if (ret)
    return -1;

  url = extract_value(payload, "url");
  
  
  string users = extract_users(payload);
  // dbg("payload = " + payload);

  return 0;
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

  dbg("slack_rtm_start> get fields: [" + get_fields + "]");

  string ApiUrl = _ApiUrl + "rtm.start?" + get_fields;

  dbg("slack_rtm_start> Using URL: [" + ApiUrl + "]");
  payload = "";
  curl_easy_setopt(curl, CURLOPT_URL, ApiUrl.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg("slack_rtm_start> cURL perform failed: " + (string)_errorBuffer);
    curl_easy_cleanup(curl);
    return -1;
  } else
  {
    dbg("slack_rtm_start> Got data");
  }

//dbg("Got: " + payload);
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
    dbg("extract_value> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return "";
  }

  json_object *obj_param = json_object_object_get(jobj, param.c_str());
  if (obj_param == NULL)
  {
    dbg("extract_value> json_object_object_get failed. JSON data: [" + json_in + "]");
    json_object_put(jobj);
    return "";
  }

  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    dbg("extract_value> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return "";
  }

  dbg("extract_value> got [" + param + "] = [" + (string)param_val + "]");
  string result = (string)param_val;

  json_object_put(obj_param);
  json_object_put(jobj);

  return result;
}

string CSlackWeb::extract_users(string json_in)
/*
 */
{
  json_object *jobj = json_tokener_parse(json_in.c_str());
  if (jobj == NULL)
  {
    dbg("extract_users> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return "";
  }

  json_object *obj_param = json_object_object_get(jobj, "users");
  if (obj_param == NULL)
  {
    dbg("extract_users> json_object_object_get failed. JSON data: [" + json_in + "]");
    json_object_put(jobj);
    return "";
  }
  
  // Expecting "users" to be an array, check it is.
  enum json_type type;
  if (json_object_get_type(obj_param) != json_type_array)
  {
    dbg("extract_users> Error: \"users\" isn't an array");
    json_object_put(jobj);
    return "";
  }
  
  json_object *jarray = obj_param;
  unsigned int user_array_length = json_object_array_length(jarray);
  
  // Loop through array of user objects
  for (unsigned int idx = 0; idx < user_array_length; idx++)
  {
    const char *user_obj_id_str;
    const char *user_obj_name_str;

    json_object *user_obj = json_object_array_get_idx(jarray, idx); 

    // get id
    json_object *user_obj_id = json_object_object_get(user_obj, "id");
    if (user_obj_id == NULL)
      continue;
    user_obj_id_str = json_object_get_string(user_obj_id);

    // get name
    json_object *user_obj_name = json_object_object_get(user_obj, "name");
    if (user_obj_name == NULL)
      continue;
    user_obj_name_str = json_object_get_string(user_obj_name);

    dbg("id: " + (string)user_obj_id_str + ", name: " + (string)user_obj_name_str);
  }

  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    dbg("extract_users> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return "";
  }


  string result = (string)param_val;

  json_object_put(obj_param);
  json_object_put(jobj);

  return result;
}
