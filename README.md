SlackRtm
========

SlackRtm is a very minimal c++ library for the Slack Real Time Messaging protocol, with an example app (slackmqtt) that acts as a bridge between slack and an MQTT broker such as Mosquitto - this makes it particularly easy for Arduinos making use of the [pubsubclient library](https://github.com/knolleary/pubsubclient) to message a slack channel.

* http://slack.com/
* http://mosquitto.org/

## Features

* At present, only allows for messages to be sent to/from slack channels - i.e. no support for private messages, private channels, etc.

## Requirements

* Linux; current only tested on Debian Stretch.
* Required packages: g++ libjson-c-dev libcurl4-gnutls-dev libutfcpp-dev libboost-dev libmosquittopp-dev libmosquitto-dev mosquitto mosquitto-clients libwebsockets-dev
* A [Vagrantfile](https://www.vagrantup.com/) is included, so a "vagrant up" should be all that's needed to test it

## slackmqtt
### Installation from Source

1. Install the required packages: "apt-get install g++ libjson-c-dev libcurl4-gnutls-dev libutfcpp-dev libboost-dev libmosquittopp-dev libmosquitto-dev mosquitto mosquitto-clients libwebsockets-dev"
2. After cloning, run "make" in the root

### Installation from binary package
Download the binary package [slackmqtt_0.2-1_amd64.deb](https://github.com/daniel1111/SlackRtm/releases/download/v0.2/slackmqtt_0.2-1_amd64.deb), then run:
```
dpkg --install slackmqtt_0.2-1_amd64.deb
apt-get install -f
```
### Usage

1. Get an API token from Slack, goto:
  Configure Intergrations
  DIY Integrations & Customizations -> Bots
  Pick a username -> add Intergration
  Note down the "API Token"
  
2. Run SlackMqtt, and specifiy (at minimum) the API key:

```
./examples/mqtt/SlackMqtt -k "<token>"
or
/usr/bin/SlackMqtt -k "<token>"
```

By default, SlackMqtt expects to find an MQTT broker running on localhost, port 1883; this can be overridden by passing the "-p \<port\>" and/or "-h \<host\>" parameters.

Once running, messages in any of the Slack channels the bot is present in will be published to the MQTT topic "slack/rx/\<channel\>"; this can be seen by running something like "mosquitto_sub -t 'slack/rx/#'" in another terminal. The MQTT topic used can be changed by the "-r \<topic\>" parameter.

Messages published to the MQTT topic "slack/tx/\<channel\>" will be sent to the Slack channel \<channel\>, provided that the bot is already present in that channel (i.e. has been invited in). This MQTT topic can be changed by the "-t \<topic\>" parameter.
E.g. to send a message to #general, run "mosquitto_pub -t 'slack/tx/general' -m 'hello, world'".

### Glaring ommisions
* No ability to send PMs
* No systemd, upstart or init script included yet, so needs be ran directly from the console. It is able to redirect logging to syslog (parameter "-l SYSLOG"), so writing such a script probably isn't hard
* Only tested on - and for the moment, probably only works on - Debian (0.1 for Jessie, 0.2 for Stretch) for amd64
* Probably a lot more

## slackrtm
TODO: Some/more documentation on the slackrtm library.
