#pragma once

class SlackRTMCallbackInterface
{
  public:
    virtual int  cbi_got_slack_message(std::string, std::string, std::string) = 0;
    virtual void cbi_debug_message(int level, std::string msg) = 0;
};
