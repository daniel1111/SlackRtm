#include <string>
#include <sstream>
#include <pthread.h>
#include <queue>
#include <syslog.h>
#include "libwebsockets.h"
#include "include/SlackRTMCallbackInterface.h"
 

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
    int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *in, size_t len);
    virtual int got_data(std::string data) = 0;
    static void *service_thread(void *arg);
    static void s_web_socket_debug(int level, const char *line);
    static int ws_static_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
    
  protected:
    enum ws_status {NOT_CONNECTED, CONNECTED};
    enum ws_mutex  {MUT_CONNECTION_STATUS};
    ws_status status;
    static std::string reason2text(int code);
    
  private:
    struct lws_context *context;
    struct lws_protocols protocols[2];
    char *store_string(std::string s);
    void dbg(int dbglvl, std::string msg);
    int ws_send_pending();
    void get_mutex(ws_mutex mutex);
    void release_mutex(ws_mutex mutex);
    bool get_mutex_from_enum(ws_mutex mutex, pthread_mutex_t **mutex_out, std::string &mutex_desription);
    
    SlackRTMCallbackInterface *_cb;
    
    std::queue<std::string> _ws_queue;
    char *protocol;
    char *path;
    char *address;
    char *address_and_port;
    int port;
    struct lws *ws;
    pthread_t t_service;
    void service_thread();
    pthread_mutex_t mut_connection_status;
    char *iface;

    
};
