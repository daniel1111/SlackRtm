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

#include "CWebSocket.h"
#include <stdio.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <iostream>
#include "libwebsockets.h"

using namespace std;

 
static CWebSocket *g_CWebSocket;

char *CWebSocket::store_string(string s)
{
  //Note: need to free() any strings created in destructor
  char *c;

  c = (char*)malloc(s.length()+2);
  memset(c, 0, s.length()+1);
  strcpy(c, s.c_str());

  return c;
}

CWebSocket::CWebSocket(SlackRTMCallbackInterface *cb)
{
  _cb = cb;
  protocol = NULL;
  path = NULL;
  address = NULL;
  address_and_port = NULL;
  memset(protocols, 0, sizeof(protocols)); 
  port = -1;
  context = NULL;
  status = NOT_CONNECTED;
  t_service = -1;
  iface = NULL;
  pthread_mutex_init (&mut_connection_status, NULL);
  set_protocol("");
  g_CWebSocket = this;
  lws_set_log_level(0xFF, &CWebSocket::s_web_socket_debug);
}

void CWebSocket::s_web_socket_debug(int level, const char *line)
/* Static function - Dirty nasty hack to get debugging output from libwebsockets */
{
  if (g_CWebSocket)
  {
    string msg = line;
    msg.erase(msg.find_last_not_of(" \n\r\t")+1);
    g_CWebSocket->dbg(LOG_DEBUG, "[LWS] " + msg);
  }
}

int CWebSocket::set_address(string address)
{
  dbg(LOG_INFO, "websocket target address set to [" + address + "]");
  if (CWebSocket::address != NULL)
    free(CWebSocket::address);
  
  CWebSocket::address = store_string(address);  
  return 0; 
}

int CWebSocket::set_iface(string iface)
{
  dbg(LOG_INFO, "websocket network interface set to [" + iface + "]");
  if (CWebSocket::iface != NULL)
    free(CWebSocket::iface);
  
  CWebSocket::iface = store_string(iface);
  return 0; 
}

int CWebSocket::set_path(string path)
{
  dbg(LOG_INFO, "websocket path set to [" + path + "]");
  
  if (CWebSocket::path != NULL)
    free(CWebSocket::path);
  
  CWebSocket::path = store_string(path);
  return 0; 
}

int CWebSocket::set_protocol(string protocol)
{
  dbg(LOG_INFO, "websocket protocol set to [" + protocol + "]");
  
  if (CWebSocket::protocol != NULL)
    free(CWebSocket::protocol);  
  
  protocols[0].name = store_string(protocol);
  protocols[0].callback = &CWebSocket::ws_static_callback;
//protocols[0].rx_buffer_size = (1024*16); // 16k buffer
  CWebSocket::protocol = store_string(protocol);
  return 0; 
}

int CWebSocket::set_port(int port)
{
  dbg(LOG_INFO, "websocket port set to [" + itos(port) + "]");
  CWebSocket::port = port;
  return 0; 
}

int CWebSocket::ws_open()
{
  struct lws_context_creation_info info;
  struct lws_client_connect_info connect_info;
  int use_ssl;
  char *proto;
  memset(&info, 0, sizeof info);

  if ((protocol == NULL) || (path == NULL) || (address == NULL) || (port == -1))
  {
    dbg(LOG_ERR, "CWebSocket::open(): NULL required property!");
    return -1;
  }
  
  if (CWebSocket::address_and_port != NULL)
      free(CWebSocket::address_and_port);    
  CWebSocket::address_and_port = store_string((string)address + ":" + itos(port));
  dbg(LOG_INFO, "address_and_port=[" + (string)address_and_port + "]");
  dbg(LOG_INFO, "address=[" + (string)address + "]");
  dbg(LOG_INFO, "port=[" + itos(port) + "]");
  
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;  
  info.iface = iface;
  info.options = 0;
  info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;

  context = lws_create_context(&info);
  if (context == NULL) 
  {
    dbg(LOG_ERR, "lws_create_context failed");
    return -1;
  }
  
  use_ssl = 2;
  if (strlen(protocol) == 0)
    proto = 0;
  else
    proto = protocol;
  
  memset(&connect_info, 0, sizeof(connect_info));
  
  
  connect_info.port = port;
  connect_info.address = address;
  connect_info.path = path;
  connect_info.context = context;
  connect_info.ssl_connection = use_ssl;
  connect_info.host = address; //address_and_port; libwebsockets fails with certificate verify errors if this includes the port
  connect_info.origin = address_and_port;
  connect_info.ietf_version_or_minus_one = -1;
  connect_info.protocol = proto;
  connect_info.userdata = this;

  dbg(LOG_ERR, "About to call lws_client_connect_via_info");
  ws = lws_client_connect_via_info(&connect_info);

  if (ws == NULL)
  {
    dbg(LOG_ERR, "lws_client_connect_via_info failed");
    return -1;
  }

  dbg(LOG_NOTICE, "Websocket connection opened");
  get_mutex(MUT_CONNECTION_STATUS);
  status = CONNECTED;
  release_mutex(MUT_CONNECTION_STATUS);

  return 0;
}


int CWebSocket::ws_send(string s)
{
  dbg(LOG_INFO, "> " + s);

  get_mutex(MUT_CONNECTION_STATUS);
  if (status != CONNECTED)
  {
    dbg(LOG_WARNING, "ws_send: Not connected!");
    release_mutex(MUT_CONNECTION_STATUS);
    return -1;
  }

  // Queue the message for transmission by the service routine
  _ws_queue.push(s);

  // request a callback to transmit it
  dbg(LOG_DEBUG, "lws_callback_on_writable(ws)=" + itos(lws_callback_on_writable(ws)));
  release_mutex(MUT_CONNECTION_STATUS);

  return 0;
}


// Start service thread, and return
int CWebSocket::service()
{
  if (pthread_create(&t_service, NULL, &CWebSocket::service_thread, this))
    return -1;
  else
    return 0;
}

void *CWebSocket::service_thread(void *arg)
{
    CWebSocket *cws;
    cws = (CWebSocket*) arg;
    cws->service_thread();
    return 0;
}

void CWebSocket::service_thread()
{
  int n=0;
  
    dbg(LOG_DEBUG, "Entered service_thread");
    while ((n==0) && (status == CONNECTED))
    {
        //dbg(LOG_DEBUG, "Call lws_service");
        n = lws_service(context, 100);

    }

    status = NOT_CONNECTED;
    dbg(LOG_NOTICE, "Websocket disconnected!");
    if (context != NULL)
    {
      lws_context_destroy(context);
      context = NULL;
    }
}

CWebSocket::~CWebSocket()
{
  g_CWebSocket = NULL;

  status = NOT_CONNECTED;

  if (address != NULL)
    free(address);

  if (address_and_port != NULL)
    free(address_and_port);

  if (CWebSocket::iface != NULL)
    free(iface);

  if (path != NULL)
    free(path);

  if (protocol != NULL)
    free(protocol);

  if (t_service != -1)
    pthread_join(t_service, NULL);
  dbg(LOG_DEBUG, "~CWebSocket(): Done");
}

int CWebSocket::ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *in, size_t len)
{

  switch (reason)
  {
    
    // *** When adding an extra case here, it must also be added to ws_static_callback ***

    case LWS_CALLBACK_CLOSED:
      dbg(LOG_DEBUG, "LWS_CALLBACK_CLOSED");

      get_mutex(MUT_CONNECTION_STATUS);
      status = NOT_CONNECTED;
      release_mutex(MUT_CONNECTION_STATUS);
      break;

    case LWS_CALLBACK_WSI_DESTROY:
      dbg(LOG_DEBUG, "LWS_CALLBACK_WSI_DESTROY");

      get_mutex(MUT_CONNECTION_STATUS);
      status = NOT_CONNECTED;
      release_mutex(MUT_CONNECTION_STATUS);

    case LWS_CALLBACK_CLIENT_RECEIVE:
      if (in)
      {
        ((char *)in)[len] = '\0';

        dbg(LOG_INFO, "< " + (string)((char*)in));
        got_data((char*)in);
      }
      break;

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
      ws_send_pending();
      break;

    case LWS_CALLBACK_GET_THREAD_ID:
      return pthread_self();

    default:
      break;
  }

  return 0;
}

int CWebSocket::ws_send_pending()
{
  dbg(LOG_DEBUG, "ws_send_pending enter");

  if (_ws_queue.size() <= 0)
  {
    dbg(LOG_DEBUG, "ws_send_pending exit");
    return 0;
  }

  string s = _ws_queue.front();

  unsigned char *buf = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + s.length() + 1 + LWS_SEND_BUFFER_POST_PADDING);

  if (buf==NULL)
    return -1;

  strcpy((char*)(buf+LWS_SEND_BUFFER_PRE_PADDING), s.c_str());

  lws_write(ws, buf+LWS_SEND_BUFFER_PRE_PADDING, s.length()+1, LWS_WRITE_TEXT);

  free(buf);
  _ws_queue.pop();

  // If there's still stuff left to be sent, ask for a callback when we can send it.
  if (_ws_queue.size() > 0)
    lws_callback_on_writable(ws);

  dbg(LOG_DEBUG, "ws_send_pending exit");
  return 0;
}

void CWebSocket::get_mutex(ws_mutex enum_mutex)
{
  pthread_mutex_t *pthread_mutex;
  string mutex_desription="";

  if (!get_mutex_from_enum(enum_mutex, &pthread_mutex, mutex_desription))
  {
    dbg(LOG_ERR, "get_mutex(): get_mutex_from_enum failed! (enum_mutex = " + itos(enum_mutex) + ")");
  }
  else
  {
    dbg(LOG_DEBUG, "get_mutex(): getting [" + mutex_desription + "] lock...");
    pthread_mutex_lock(pthread_mutex);
    dbg(LOG_DEBUG, "get_mutex(): lock acquired");
  }
}

void CWebSocket::release_mutex(ws_mutex enum_mutex)
{
  pthread_mutex_t *pthread_mutex;
  string mutex_desription="";

  if (!get_mutex_from_enum(enum_mutex, &pthread_mutex, mutex_desription))
  {
    dbg(LOG_ERR, "release_mutex(): get_mutex_from_enum failed! (enum_mutex = " + itos(enum_mutex) + ")");
  }
  else
  {
    pthread_mutex_unlock(pthread_mutex);
    dbg(LOG_DEBUG, "release_mutex(): releasing [" + mutex_desription + "]");
  }
}

bool CWebSocket::get_mutex_from_enum(ws_mutex enum_mutex, pthread_mutex_t **pthread_mutex_out, string &mutex_desription_out)
{
  bool ret=true;

  switch (enum_mutex)
  {
    case MUT_CONNECTION_STATUS:
      *pthread_mutex_out = &mut_connection_status;
      mutex_desription_out = "MUT_CONNECTION_STATUS";
      break;

    default:
      ret = false;
      mutex_desription_out = "UNKNOWN!";
      break;
  }

  return ret;
}

string CWebSocket::reason2text(int code)
{
  string reason = "<UNKNOWN>";
  switch (code)
  {
    case   LWS_CALLBACK_ESTABLISHED:  reason = "LWS_CALLBACK_ESTABLISHED";  break;
    case   LWS_CALLBACK_CLIENT_CONNECTION_ERROR: reason = "LWS_CALLBACK_CLIENT_CONNECTION_ERROR"; break;
    case   LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: reason = "LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH"; break;
    case   LWS_CALLBACK_CLIENT_ESTABLISHED: reason = "LWS_CALLBACK_CLIENT_ESTABLISHED"; break;
    case   LWS_CALLBACK_CLOSED: reason = "LWS_CALLBACK_CLOSED"; break;
    case   LWS_CALLBACK_CLOSED_HTTP: reason = "LWS_CALLBACK_CLOSED_HTTP"; break;
    case   LWS_CALLBACK_RECEIVE: reason = "LWS_CALLBACK_RECEIVE"; break;
    case   LWS_CALLBACK_CLIENT_RECEIVE: reason = "LWS_CALLBACK_CLIENT_RECEIVE"; break;
    case   LWS_CALLBACK_CLIENT_RECEIVE_PONG: reason = "LWS_CALLBACK_CLIENT_RECEIVE_PONG"; break;
    case   LWS_CALLBACK_CLIENT_WRITEABLE: reason = "LWS_CALLBACK_CLIENT_WRITEABLE"; break;
    case   LWS_CALLBACK_SERVER_WRITEABLE: reason = "LWS_CALLBACK_SERVER_WRITEABLE"; break;
    case   LWS_CALLBACK_HTTP: reason = "LWS_CALLBACK_HTTP"; break;
    case   LWS_CALLBACK_HTTP_BODY: reason = "LWS_CALLBACK_HTTP_BODY"; break;
    case   LWS_CALLBACK_HTTP_BODY_COMPLETION: reason = "LWS_CALLBACK_HTTP_BODY_COMPLETION"; break;
    case   LWS_CALLBACK_HTTP_FILE_COMPLETION: reason = "LWS_CALLBACK_HTTP_FILE_COMPLETION"; break;
    case   LWS_CALLBACK_HTTP_WRITEABLE: reason = "LWS_CALLBACK_HTTP_WRITEABLE"; break;
    case   LWS_CALLBACK_FILTER_NETWORK_CONNECTION: reason = "LWS_CALLBACK_FILTER_NETWORK_CONNECTION"; break;
    case   LWS_CALLBACK_FILTER_HTTP_CONNECTION: reason = "LWS_CALLBACK_FILTER_HTTP_CONNECTION"; break;
    case   LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: reason = "LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED"; break;
    case   LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: reason = "LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION"; break;
    case   LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS: reason = "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS"; break;
    case   LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS: reason = "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS"; break;
    case   LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION: reason = "LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION"; break;
    case   LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: reason = "LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER"; break;
    case   LWS_CALLBACK_CONFIRM_EXTENSION_OKAY: reason = "LWS_CALLBACK_CONFIRM_EXTENSION_OKAY"; break;
    case   LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED: reason = "LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED"; break;
    case   LWS_CALLBACK_PROTOCOL_INIT: reason = "LWS_CALLBACK_PROTOCOL_INIT"; break;
    case   LWS_CALLBACK_PROTOCOL_DESTROY: reason = "LWS_CALLBACK_PROTOCOL_DESTROY"; break;
    case   LWS_CALLBACK_WSI_CREATE: reason = "LWS_CALLBACK_WSI_CREATE"; break;  
    case   LWS_CALLBACK_WSI_DESTROY: reason = "LWS_CALLBACK_WSI_DESTROY"; break;  
    case   LWS_CALLBACK_GET_THREAD_ID: reason = "LWS_CALLBACK_GET_THREAD_ID"; break;
    case   LWS_CALLBACK_ADD_POLL_FD: reason = "LWS_CALLBACK_ADD_POLL_FD"; break;
    case   LWS_CALLBACK_DEL_POLL_FD: reason = "LWS_CALLBACK_DEL_POLL_FD"; break;
    case   LWS_CALLBACK_CHANGE_MODE_POLL_FD: reason = "LWS_CALLBACK_CHANGE_MODE_POLL_FD"; break;
    case   LWS_CALLBACK_LOCK_POLL: reason = "LWS_CALLBACK_LOCK_POLL"; break;
    case   LWS_CALLBACK_UNLOCK_POLL: reason = "LWS_CALLBACK_UNLOCK_POLL"; break;
    case   LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY: reason = "LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY"; break;
    case   1000: reason = "LWS_CALLBACK_USER"; break; 
  }
  return reason;
}

int CWebSocket::ws_static_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)  
{
  if (g_CWebSocket)
    if (reason != LWS_CALLBACK_GET_THREAD_ID) // LWS_CALLBACK_GET_THREAD_ID is called very frequently, so don't write it out
      g_CWebSocket->dbg(LOG_DEBUG,"ws_callback> reason=[" + itos(reason) + "] size=[" + itos(len) + "] (" + reason2text(reason) + ")");

  // Usually, user is the user data supplied when connecting. But depening on the reason, it might be something else.
  // So sadly we can't just assume if it's set, is a pointer to a CWebSocket object.
  switch (reason)
  {
    case LWS_CALLBACK_CLOSED:
    case LWS_CALLBACK_WSI_DESTROY:
    case LWS_CALLBACK_CLIENT_RECEIVE:
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    case LWS_CALLBACK_CLIENT_WRITEABLE:
    case LWS_CALLBACK_GET_THREAD_ID:
      if (user)
      {
        CWebSocket *ws; 
        ws = (CWebSocket*) user;
        return ws->ws_callback(wsi, reason, in, len);
      }
      break;
  
    default:
      break;
  }

  return 0;
}

void CWebSocket::dbg(int dbglvl, string msg)
{
  _cb->cbi_debug_message(dbglvl, "CWebSocket(" + itos(pthread_self()) + ")::" + msg);
}

string itos(int n)
{
  string s;
  stringstream out;
  out << n;
  return out.str();
}
