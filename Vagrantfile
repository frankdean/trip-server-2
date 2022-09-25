# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|

  # Set :TRIP_DEV to 'y' (lower-case) to setup and run the VM as a
  # development environment.  If the VM already exists, run or reload
  # with the '--provision' option, e.g. 'vagrant reload --provision'.
  #
  # Set :VB_GUI to 'y' (lower-case) to run a GUI environment.  If the
  # VM already exists, run or reload with the '--provision' option,
  # e.g. 'vagrant reload --provision'.
  #
  # Set :WIPE_DB to 'y' (lower-case) to WIPE the database each time
  # the VM is started.
  myEnv = { :TRIP_DEV => "y",
            :VB_GUI => "n",
            :WIPE_DB => "n" }

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  # Bento are one of the Vagrant recommended boxes see
  # https://www.vagrantup.com/docs/boxes.html#official-boxes
  # https://app.vagrantup.com/bento
  #config.vm.box = "bento/debian-11"
  #config.vm.box_version = "202112.19.0"
  # https://wiki.debian.org/Teams/Cloud/VagrantBaseBoxes
  # Boxes: https://app.vagrantup.com/debian
  config.vm.box = "debian/bullseye64"
  #config.vm.box_version = "11.20211230.1"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  #config.vm.network "forwarded_port", guest: 80, host: 80
  config.vm.network "forwarded_port", guest: 8080, host: 8080
  config.vm.network "forwarded_port", guest: 8081, host: 8081

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  if myEnv[:TRIP_DEV] == "y"
    config.vm.synced_folder "../trip-web-client", "/vagrant-trip-web-client"
    config.vm.synced_folder "../trip-server", "/vagrant-trip-server"
  end

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # View the documentation for the provider you are using (VirtualBox)
  # for more information on available options.
  config.vm.provider "virtualbox" do |v|
    v.name = "Trip2—Vagrant"
    # Display the VirtualBox GUI when booting the machine
    if myEnv[:VB_GUI] == "y"
      v.gui = true
    end

    # The amount of video ram in MB
    # https://www.virtualbox.org/manual/ch08.html#vboxmanage-cmd-overview
    if v.gui
      v.customize [ "modifyvm", :id, "--vram", "32" ]
    end

    # Customize the amount of memory on the VM:
    if myEnv[:TRIP_DEV] == "y"
      v.memory = "4196"
    else
      v.memory = "1024"
    end

    # Whether to use a master VM and linked clones
    v.linked_clone = false
  end

  config.vm.provision :shell, path: "provisioning/bootstrap.sh", env: myEnv
  config.vm.provision :shell, path: "provisioning/bootconfig.sh", run: "always", env: myEnv

end
