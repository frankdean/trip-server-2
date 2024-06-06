#!/usr/bin/env bash

# This file is part of Trip Server 2, a program to support trip recording and
# itinerary planning.
#
# Copyright (C) 2022-2024 Frank Dean <frank.dean@fdsd.co.uk>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Uncomment the following to debug the script
#set -x

# The user and group name for building Trip Server
USERNAME=${USERNAME:-vagrant}

export DEBIAN_FRONTEND=noninteractive
DEB_OPTIONS="--yes --allow-change-held-packages"
SU_CMD="su ${USERNAME} -c"

append_name_to_file() {
    if [ -n "$1" ]; then
	next_start=100000
	next_length=65536
	while IFS=: read name start length
	do
	    echo "$name - $start - $length"
	    next_length=$length
	    next_start=$(($start+$next_length))
	done <$1
	echo "$USERNAME:$next_start:$next_length" >>$1
    fi
}

apt-get install $DEB_OPTIONS podman podman-compose qemu-system-x86
fuse-overlayfs

if [ ! -d "/home/$USERNAME/.config/containers" ]; then
    $SU_CMD "mkdir -p /home/$USERNAME/.config/containers"
fi
if [ ! -e "/home/$USERNAME/.config/containers/storage.conf" ]; then
    cat <<EOF >> "/home/$USERNAME/.config/containers/storage.conf"
[storage]
  driver = "overlay"

[storage.options.overlay]

  mount_program = "/usr/bin/fuse-overlayfs"
EOF
fi
if [ ! -e "/home/$USERNAME/.config/containers/registries.conf" ]; then
    cat <<EOF >> "/home/$USERNAME/.config/containers/registries.conf"
unqualified-search-registries = ["docker.io"]
EOF
fi

grep "$USERNAME" /etc/subuid >/dev/null 2>&1
if [ $? -ne 0 ]; then
    append_name_to_file "/etc/subuid"
fi
grep "$USERNAME" /etc/subgid >/dev/null 2>&1
if [ $? -ne 0 ]; then
    append_name_to_file "/etc/subgid" test.txt
fi

$SU_CMD "podman machine init"
$SU_CMD "podman info | grep -E 'graphDriverName:\s+overlay'"
if [ $? -ne 0 ]; then
    echo "The overlay filesystem is not configured correctly"
fi

if [ ! -f "/home/$USERNAME/.bash_aliases" ]; then
    touch "/home/$USERNAME/.bash_aliases"
fi
grep docker "/home/$USERNAME/.bash_aliases"
if [ $? -ne 0 ]; then
    echo "alias docker='podman'" >>"/home/$USERNAME/.bash_aliases"
fi
