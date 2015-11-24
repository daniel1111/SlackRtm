#include <string>
#include <sstream>
#include <pthread.h>
#include <queue>
#include <syslog.h>
#include "libwebsockets.h"
#include "SlackRTMCallbackInterface.h"
 

std::string itos(int n);
std::string reason2text(int code);

class CWebSocket
{
  public:
    CWebSocket(SlackRTMCallbackInterface *cb);
    CWebSocket(std::string address, int port, std::string path, std::string protocol, SlackRTMCallbackInterface *cb);
    virtual ~CWebSocket();
    int set_address(std::string address);
    int set_iface(std::string iface);
    int set_path(std::string path);
    int set_protocol(std::string protocol);
    int set_port(int port);
    int ws_open();
    int service();
    int ws_send(std::string s);
    int ws_callback(struct libwebsocket_context *wsc, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *in, size_t len);
    virtual int got_data(std::string data) = 0;
    static void *service_thread(void *arg);
    static void s_web_socket_debug(int level, const char *line);
    
  protected:
    enum ws_status {NOT_CONNECTED, CONNECTED};
    ws_status status;
    std::string reason2text(int code);
    
  private:
    struct libwebsocket_context *context;
    struct libwebsocket_protocols protocols[2];
    char *store_string(std::string s);
    void dbg(int dbglvl, std::string msg);
    int ws_send_pending();
    SlackRTMCallbackInterface *_cb;
    
    std::queue<std::string> _ws_queue;
    char *protocol;
    char *path;
    char *address;
    int port;
    struct libwebsocket *ws;
    pthread_t t_service;
    void service_thread();
    pthread_mutex_t ws_mutex;
    char *iface;

    
};
