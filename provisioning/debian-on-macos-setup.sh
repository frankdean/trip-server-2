#!/bin/sh

# User and group names to create on the guest VM
USERNAME=${USERNAME:-vagrant}
GROUPNAME=${GROUPNAME:-${USERNAME}}
# Set to match the UID and GID of the user on the host system: id -u && id -g
KUID=${KUID:=501}
KGID=${KGID:=20}
# The target path on the guest to mount the shared folder
TRIP_SOURCE=${TRIP_SOURCE:-/vagrant}

sudo apt-get remove systemd-timesyncd
grep $USERNAME /etc/passwd >/dev/null 2>&1
if [ $? -ne 0 ]; then
    sudo adduser --uid $KUID --gid $KGID $USERNAME
    sudo adduser $USERNAME sudo
    sudo chmod go+rx /home/${USERNAME}
fi
if [ ! -d $TRIP_SOURCE ]; then
    sudo mkdir -p $TRIP_SOURCE
    sudo chown "$USERNAME:$GROUPNAME" $TRIP_SOURCE
fi
grep $TRIP_SOURCE /etc/fstab
if [ $? -ne 0 ]; then
    echo "trip2 $TRIP_SOURCE 9p trans=virtio,msize=104857600,ro 0 0" | sudo tee -a /etc/fstab
    sudo systemctl daemon-reload
    sudo mount $TRIP_SOURCE
fi
mount -l -t 9p | grep /vagrant
if [ $? -ne 0 ]; then
    sudo mount $TRIP_SOURCE
fi

sudo TRIP_SOURCE=$TRIP_SOURCE USERNAME=$USERNAME GROUPNAME=$USERNAME USERHOME="/home/${USERNAME}" bash -x "${TRIP_SOURCE}/provisioning/bootstrap.sh"
sudo TRIP_SOURCE=$TRIP_SOURCE USERNAME=$USERNAME GROUPNAME=$USERNAME USERHOME="/home/${USERNAME}" bash -x "${TRIP_SOURCE}/provisioning/bootconfig.sh"
