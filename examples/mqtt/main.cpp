#include <string>
#include <iostream>
#include <mosquittopp.h> //   libmosquittopp-dev

#include "CSlackRTM.h"


using namespace std;


class SlackMQTT : public SlackRTMCallbackInterface, mosqpp::mosquittopp
{
private:
  CSlackRTM *_rtm;
  string _slack_rx_topic;
  string _slack_tx_topic;
  string _mqtt_host;
  int    _mqtt_port;
  bool   _debug;

public:
  SlackMQTT(string token, string apiurl, const char *id, string tx_topic, string rx_topic, string mqtt_host, int mqtt_port, bool debug) : mosquittopp(id)
  {
     _slack_rx_topic = rx_topic;
     _slack_tx_topic = tx_topic;
     _mqtt_host      = mqtt_host;
     _debug          = debug;
     _mqtt_port      = mqtt_port;
    _rtm = new CSlackRTM(token, apiurl, this);
  }

  ~SlackMQTT()
  {
    delete _rtm;
  }

  void start()
  {
    int rc = -1;

    // Connect to mosquitto
    rc = connect(_mqtt_host.c_str(), _mqtt_port, 60);

    // Connect to Slack
    _rtm->go();

    cbi_debug_message("start> Entering mqtt loop");
    while(1)
    {
      rc = loop();
      if(rc)
        reconnect(); // Reconnect to mosquitto
    }
  }

  int cbi_got_slack_message(string channel, string username, string message)
  {
    cbi_debug_message("cbi_got_slack_message> #" + channel + "/<" + username + "> " + message);
    // When publishing to MQTT, we're not using the slack username for anything.

    string topic = _slack_rx_topic + "/" + channel;
    publish(NULL, topic.c_str(), message.length(), message.c_str());

    return 0;
  }

  void cbi_debug_message(string msg)
  {
    if (_debug)
      cout << "[" + msg + "]" << endl;
  }

  void on_connect(int rc)
  {
    cbi_debug_message("on_connect> Connected to mosquitto");
    if(rc == 0)
    {
      // Only attempt to subscribe on a successful connect.
      subscribe(NULL, string(_slack_tx_topic + "/+").c_str());
    }
  }

  void on_message(const struct mosquitto_message *message)
  /* Message received from MQTT */
  {
    char buf[51];

    memset(buf, 0, 51*sizeof(char));
    memcpy(buf, message->payload, 50*sizeof(char));

    if(!strncmp(message->topic, string(_slack_tx_topic + "/").c_str(), _slack_tx_topic.length()))
    {
      string channel =  ((string)(message->topic)).substr (_slack_tx_topic.length()+1,string::npos);
      cbi_debug_message("got message: [" + ((string)(message->topic)) + "] " + (string)buf);
      _rtm->send(channel, buf);
    }
  }

  void on_subscribe(int mid, int qos_count, const int *granted_qos)
  {
    cbi_debug_message("on_subscribe> Subscription succeeded");
  }

};

void print_usage(const char *arg0)
{
  fprintf(stderr, "Usage: %s -k token [-d] [-t MQTT to Slack topic] [-r Slack to MQTT topic] [-m MQTT server] [-p MQTT port] [-i MQTT client ID]  \n\n", arg0);

  fprintf(stderr, "    -k : Slack API Token (slack -> Configure Intergrations -> DIY Integrations -> Bots -> Add -> API Token)\n");
  fprintf(stderr, "    -d : Enable debuging output \n");
  fprintf(stderr, "    -t : MQTT->Slack tx topic. Defaults to \"slack/tx\". With the default, publishing to \"slack/tx/general\" would push to the #general channel in slack \n");
  fprintf(stderr, "    -r : Slack->MQTT rx topic. Defaults to \"slack/rx\". With the default, messages to the #general channel in slack will be published to \"slack/rx/general\" \n");
  fprintf(stderr, "    -h : MQTT server to connect to. Defaults to localhost \n");
  fprintf(stderr, "    -p : MQTT server port. Defaults to 1883 \n");
  fprintf(stderr, "    -i : MQTT client ID to use. Defaults to \"SlackRtm\" \n");

  fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
  string apiurl = "https://slack.com/api/";

  string token = "";
  bool debug = false;
  string tx = "slack/tx";
  string rx = "slack/rx";
  string mqtt_host = "127.0.0.1";
  string mqtt_id = "SlackRtm";
  int    mqtt_port = 1883;

  int opt;

  while ((opt = getopt(argc, argv, "k:dt:r:h:p:i:")) != -1) 
  {
    switch (opt) 
    {
      case 'k': // token (Key)
        token = optarg;
        break;

      case 'd': // Debug
        debug = true;
        break;

      case 't': // Tx topic 
        tx = optarg;
        break;

      case 'r': // Rx topic
        rx = optarg;
        break;

      case 'm': // MQTT server hostname
        mqtt_host = optarg;
        break;

      case 'p': // mqtt port
        mqtt_port = atoi(optarg);
        break;

      case 'i': // mqtt ID
        mqtt_id = optarg;
        break;

      default:
        print_usage(argv[0]);
        exit (-1);
    }
  }

  // sanity check
  if (tx == rx)
  {
    fprintf(stderr, "ERROR: MQTT Tx and Rx topics MUST NOT be the same(!)\n\n");
    exit (-1);
  }

  if (token == "")
  {
    print_usage(argv[0]);
    exit (-1);
  }

  fprintf(stdout, "\nMessages from slack will be published to: %s/<channel>\n", rx.c_str());
  fprintf(stdout,   "Messages published to [%s/<channel>] will be sent to slack\n\n", tx.c_str());

  mosqpp::lib_init();

  SlackMQTT test(token, apiurl, mqtt_id.c_str(), tx, rx, mqtt_host, mqtt_port, debug);
  test.start();

  return 0;
}
