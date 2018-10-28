#ifndef CSLACKRTM_H
#define CSLACKRTM_H

#include "SlackRTMCallbackInterface.h"

class CSlackRTM
{
  public:
    CSlackRTM(std::string token, std::string api_url, SlackRTMCallbackInterface *cb);
    ~CSlackRTM();
    void go();
    int send(std::string channel, std::string message);
    int send_dm(std::string username, std::string message);
};

#endif
