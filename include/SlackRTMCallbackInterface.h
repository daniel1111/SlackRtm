#pragma once

class SlackRTMCallbackInterface
{
  public:
    virtual int cbiGotSlackMessage(std::string, std::string, std::string) = 0;
};
