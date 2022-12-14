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

  config.vm.define "debian", primary: true do |debian|
    #debian.vm.box = "bento/debian-11"
    #debian.vm.box_version = "202112.19.0"
    # https://wiki.debian.org/Teams/Cloud/VagrantBaseBoxes
    # Boxes: https://app.vagrantup.com/debian
    debian.vm.box = "debian/bullseye64"
    debian.vm.box_version = "11.20220912.1"

    # Create a forwarded port mapping which allows access to a specific port
    # within the machine from a port on the host machine. In the example below,
    # accessing "localhost:8080" will access port 80 on the guest machine.
    #config.vm.network "forwarded_port", guest: 80, host: 80
    debian.vm.network "forwarded_port", guest: 8080, host: 8080
    debian.vm.network "forwarded_port", guest: 8081, host: 8081
  end

  config.vm.define "fedora", autostart: false do |fedora|
    # boxes at https://app.vagrantup.com/fedora/
    fedora.vm.box = "fedora/36-cloud-base"
    fedora.vm.box_version = "36-20220504.1"

    # If the VirtualBox guest additions fail to install, first try:
    #
    #   vagrant reload fedora --provision
    #
    # The initial provisioning installs packages needed to install the
    # VirtualBox guest additions.  Failing that, uncomment the following
    # definition to use the rsync method to synchronise the current folder:

    #fedora.vm.synced_folder ".", "/vagrant", type: "rsync"
    fedora.vm.network "forwarded_port", guest: 8080, host: 8082
    fedora.vm.network "forwarded_port", guest: 8081, host: 8083
  end

  # The FreeBSD configuration it not recommended for development as it doesn't
  # reliably synchronise the source directory structure.  However, it can be
  # useful to manually test a distribution tarball before release.
  config.vm.define "freebsd", autostart: false do |freebsd|
    # boxes at https://app.vagrantup.com/freebsd

    freebsd.vm.box = "freebsd/FreeBSD-13.1-STABLE"
    freebsd.vm.box_version = "2022.10.14"
    # When using FreeBSD, the following may need to be defined
    freebsd.vm.synced_folder ".", "/vagrant", type: "rsync"

    freebsd.vm.network "forwarded_port", guest: 8080, host: 8084
    freebsd.vm.network "forwarded_port", guest: 8081, host: 8085

    # Export the following environment variable to enable specifying the disk size
    # export VAGRANT_EXPERIMENTAL="disks"
    # FreeBSD needs more disk space than the default.  Suggest a minimum of
    # 13GB or 28GB if installing textlive-full, which is only needed for
    # building the PDF documentation.  This will only grow the disk space on a
    # fresh VM creation.  It will grow the volume on an existing VM but not
    # resize the underlying filesystem.
    freebsd.vm.disk :disk, size: "28GB", primary: true
  end

  # Currently there are no boxes at https://app.vagrantup.com/rockylinux that
  # are easily configured.
  # config.vm.define "rockylinux", autostart: false do |rockylinux|
  #   rockylinux.vm.box = "rockylinux/8"
  #   rockylinux.vm.network "forwarded_port", guest: 8080, host: 8086
  #   rockylinux.vm.network "forwarded_port", guest: 8081, host: 8087
  # end

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

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

    # When using Fedora, the following may need to be defined
    #fedora.vm.synced_folder ".", "/vagrant", type: "virtualbox", automount: true
  end

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # View the documentation for the provider you are using (VirtualBox)
  # for more information on available options.
  config.vm.provider "virtualbox" do |v|
    #v.name = "Trip2???Vagrant"
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
