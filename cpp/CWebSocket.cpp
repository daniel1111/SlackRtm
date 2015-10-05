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

static int ws_static_callback(struct libwebsocket_context *wsc, struct libwebsocket *wsi, 
  enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len);

char *CWebSocket::store_string(string s)
{
  //Note: need to free() any strings created in destructor
  char *c;
  c = (char*)malloc(s.length()+1);
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
  memset(protocols, 0, sizeof(protocols)); 
  port = -1;
  context = NULL;
  status = NOT_CONNECTED;
  t_service = -1;
  iface = NULL;
  pthread_mutex_init (&ws_mutex, NULL);  
  set_protocol("");
}


int CWebSocket::set_address(string address)
{
  dbg("websocket target address set to [" + address + "]");
  if (CWebSocket::address != NULL)
    free(CWebSocket::address);
  
  CWebSocket::address = store_string(address);
  return 0; 
}

int CWebSocket::set_iface(string iface)
{
  dbg("websocket network interface set to [" + iface + "]");
  if (CWebSocket::iface != NULL)
    free(CWebSocket::iface);
  
  CWebSocket::iface = store_string(iface);
  return 0; 
}

int CWebSocket::set_path(string path)
{
  dbg("websocket path set to [" + path + "]");
  
  if (CWebSocket::path != NULL)
    free(CWebSocket::path);
  
  CWebSocket::path = store_string(path);
  return 0; 
}

int CWebSocket::set_protocol(string protocol)
{
  dbg("websocket protocol set to [" + protocol + "]");
  
  if (CWebSocket::protocol != NULL)
    free(CWebSocket::protocol);  
  
  protocols[0].name = store_string(protocol);
  protocols[0].callback = ws_static_callback;
  CWebSocket::protocol = store_string(protocol);
  return 0; 
}

int CWebSocket::set_port(int port)
{
  dbg("websocket port set to [" + itos(port) + "]");
  CWebSocket::port = port;
  return 0; 
}

int CWebSocket::ws_open()
{
  struct lws_context_creation_info info;
  int use_ssl;
  char *proto;
  memset(&info, 0, sizeof info);

  if ((protocol == NULL) || (path == NULL) || (address == NULL) || (port == -1))
  {
    dbg("CWebSocket::open(): NULL required property!");
    return -1;
  }

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;  
  info.iface = iface;

  context = libwebsocket_create_context(&info);
  if (context == NULL) 
  {
    dbg("libwebsocket_create_context failed");
    return -1;
  }
  
  use_ssl = 1;
  if (strlen(protocol) == 0)
    proto = 0;
  else
    proto = protocol;
  
  ws = libwebsocket_client_connect_extended(context, address, port, use_ssl, path, address, address, proto, -1, this);
  if (ws == NULL) 
  {
    dbg("libwebsocket_client_connect_extended failed");
    return -1;
  }

  dbg("Websocket connection opened");
  status = CONNECTED;

  return 0;
}


int CWebSocket::ws_send(string s)
{
  unsigned char *buf;
  
  dbg("> " + s);
  if (status != CONNECTED)
  {
    dbg("Not connected!");
    return -1;
  }
  
  buf = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + s.length() + 1 + LWS_SEND_BUFFER_POST_PADDING);
  if (buf==NULL)
    return -1;
  
  strcpy((char*)(buf+LWS_SEND_BUFFER_PRE_PADDING), s.c_str());
  
  pthread_mutex_lock(&ws_mutex);  
  libwebsocket_write(ws, buf+LWS_SEND_BUFFER_PRE_PADDING, s.length()+1, LWS_WRITE_TEXT);
  pthread_mutex_unlock(&ws_mutex);
  
  free(buf);
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
    CWebSocket *ws;
    ws = (CWebSocket*) arg;  
    ws->service_thread();
    return 0;
}

void CWebSocket::service_thread()
{
  int n=0;
  
    while ((n==0) && (status == CONNECTED))
    {
      n = libwebsocket_service(context, 100);
    }
  
    status = NOT_CONNECTED;
    dbg("Websocket disconnected!");
    if (context != NULL)
    {
      libwebsocket_context_destroy(context);
      context = NULL;
    }
}

CWebSocket::~CWebSocket()
{
  status = NOT_CONNECTED;
  
  if (address != NULL)
    free(address);
  
  if (CWebSocket::iface != NULL)
    free(iface);  
 
  if (path != NULL)
    free(path);  
  
  if (protocol != NULL)
    free(protocol);  

  if (t_service != -1)
    pthread_join(t_service, NULL);
  dbg("~CWebSocket(): Done");
}

int CWebSocket::ws_callback(struct libwebsocket_context *wsc, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *in, size_t len)
{
// printf("ws_callback> reason=[%d], size=[%d] (%s)\n", reason, len, reason2text(reason).c_str());


  switch (reason) 
  {
    case LWS_CALLBACK_CLOSED:
      dbg("LWS_CALLBACK_CLOSED");
      status = NOT_CONNECTED;
      break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
      ((char *)in)[len] = '\0';
      
      dbg("< " + (string)((char*)in));
      got_data((char*)in);
      break;

    default:
      break;
  }

  return 0;
}


string CWebSocket::reason2text(int code)
{
  string reason = "<UNKNWON>";
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
  
  
  
static int ws_static_callback(struct libwebsocket_context *wsc, struct libwebsocket *wsi, 
  enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
  CWebSocket *ws; 
  ws = (CWebSocket*) user;
  return ws->ws_callback(wsc, wsi, reason, in, len);
}

void CWebSocket::dbg(string msg)
{
  _cb->cbi_debug_message("CWebSocket::" + msg);
}

string itos(int n)
{
  string s;
  stringstream out;
  out << n;
  return out.str();
}


  

