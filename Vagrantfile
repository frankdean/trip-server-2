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
  myEnv = { :TRIP_DEV => "n",
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
    debian.vm.box_version = "11.20231211.1"

    # Create a forwarded port mapping which allows access to a specific port
    # within the machine from a port on the host machine. In the example below,
    # accessing "localhost:8080" will access port 80 on the guest machine.
    #config.vm.network "forwarded_port", guest: 80, host: 80
    debian.vm.network "forwarded_port", guest: 8080, host: 8080
  end

  config.vm.define "debian12", autostart: false do |debian12|
    debian12.vm.box = "debian/bookworm64"
    debian12.vm.box_version = "12.20231211.1"
    debian12.vm.network "forwarded_port", guest: 8080, host: 8090
  end

  config.vm.define "fedora", autostart: false do |fedora|
    # boxes at https://app.vagrantup.com/fedora/
    fedora.vm.box = "fedora/39-cloud-base"
    fedora.vm.box_version = "39.20231031.1"

    # If the VirtualBox guest additions fail to install, first try:
    #
    #   vagrant vbguest --do install fedora
    #   vagrant reload fedora --provision
    #
    # The initial provisioning script installs packages needed to install the
    # VirtualBox guest additions and a restart is required to enable the guest
    # additions.

    fedora.vm.network "forwarded_port", guest: 8080, host: 8082
  end

  config.vm.define "freebsd", autostart: false do |freebsd|
    # https://app.vagrantup.com/freebsd
    freebsd.vm.box = "freebsd/FreeBSD-14.0-STABLE"
    freebsd.vm.box_version = "2024.01.18"

    # Bento box does not have bash shell:
    #freebsd.vm.box = "bento/freebsd-13"
    # VirtualBox 7.0.6
    #freebsd.vm.box_version = "202303.13.0"
    # VirtualBox 6.1.40
    #freebsd.vm.box_version = "202212.12.0"

    # https://superuser.com/questions/764069/freebsd-with-vagrant-dont-know-how-to-check-guest-additions-version
    # Needs host root privileges though to alter host network settings!
    #freebsd.vm.network "private_network", type: "dhcp"
    #freebsd.vm.synced_folder ".", "/vagrant", type: "nfs"

    # https://developer.hashicorp.com/vagrant/docs/cli/rsync
    freebsd.vm.synced_folder ".", "/vagrant", type: "rsync"

    freebsd.vm.network "forwarded_port", guest: 8080, host: 8084

    # Export the following environment variable to enable specifying the disk
    # size:
    #
    #     $ export VAGRANT_EXPERIMENTAL="disks"
    #
    # FreeBSD needs more disk space than the default.  Suggest a minimum of
    # 13GB or 28GB if installing textlive-full, which is only needed for
    # building the PDF documentation.  This will only grow the disk space on a
    # fresh VM creation.  It will grow the volume on an existing VM but not
    # resize the underlying filesystem.
    freebsd.vm.disk :disk, size: "28GB", primary: true
  end

  config.vm.define "rockylinux", autostart: false do |rockylinux|
    # https://app.vagrantup.com/rockylinux
    rockylinux.vm.box = "rockylinux/9"
    rockylinux.vm.box_version = "3.0.0"
    #
    # Using Bento box:
    #rockylinux.vm.box = "bento/rockylinux-9"
    # VirtualBox 7.0.6
    #rockylinux.vm.box_version = "202303.13.0"
    # if myEnv[:ROCKY_INIT] == "y"
    #   puts "Rocky Linux init\r"
    #   rockylinux.vbguest.auto_update = false
    #   #rockylinux.vm.synced_folder ".", "/vagrant", type: "rsync"
    #   rockylinux.vm.synced_folder ".", "/vagrant", type: "virtualbox", automount: false
    # end

    rockylinux.vm.synced_folder ".", "/vagrant", type: "rsync"

    if myEnv[:TRIP_DEV] == "y"
      # Export the following environment variable to enable specifying the
      # disk size:
      #
      #     $ export VAGRANT_EXPERIMENTAL="disks"
      #
      rockylinux.vm.disk :disk, size: "28GB", primary: true
      #
      # Vagrant will expand the size of the underlying disk, but not the
      # partition or filesystem.  Use `sudo fdisk -l` to see which partition
      # needs expanding.  It's probably easiest to use `parted` to resize the
      # partition, e.g. expanding `/dev/sda5` (in the Vagrant guest):
      #
      #     $ sudo parted /dev/sda resizepart 5
      #
      # `parted` will prompt for the `END` position.  Accept the default.
      # Then resize the file system on the newly expanded root partition with:
      #
      #     $ sudo xfs_grow /
      #
      # Force a recheck of the file system on next boot with:
      #
      #     $ sudo touch /forcefsck
      #
    end
    rockylinux.vm.network "forwarded_port", guest: 8080, host: 8086
  end

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

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # View the documentation for the provider you are using (VirtualBox)
  # for more information on available options.
  config.vm.provider "virtualbox" do |v|
    #v.name = "Trip2—Vagrant"
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
      v.memory = "2048"
    end

    # Whether to use a master VM and linked clones
    v.linked_clone = false
  end

  config.vm.provision :shell, path: "provisioning/bootstrap.sh", env: myEnv
  config.vm.provision :shell, path: "provisioning/bootconfig.sh", run: "always", env: myEnv

end
