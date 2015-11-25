# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
apt-get update
apt-get install -y cmake g++ libjson0-dev libcurl4-gnutls-dev libutfcpp-dev libboost-dev libmosquittopp-dev libmosquitto-dev mosquitto mosquitto-clients build-essential devscripts debhelper git

cd /vagrant
git submodule update --init --recursive

SCRIPT

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "debian/jessie64"
  config.vm.provision "shell", inline: $script
  config.vm.synced_folder "apt/", "/var/cache/apt"
end
