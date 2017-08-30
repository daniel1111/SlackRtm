# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
apt-get update
apt-get install -y g++ libjson-c-dev libcurl4-gnutls-dev libutfcpp-dev libboost-dev libmosquittopp-dev libmosquitto-dev mosquitto mosquitto-clients build-essential devscripts debhelper git libwebsockets-dev
echo ""
echo "Done"

SCRIPT

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "generic/debian9"
  config.vm.provision "shell", inline: $script
  config.vm.synced_folder "apt/", "/var/cache/apt"
  config.vm.synced_folder "./", "/vagrant"
end

