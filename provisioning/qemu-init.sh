#!/usr/bin/env bash

# This file is part of Trip Server 2, a program to support trip recording and
# itinerary planning.
#
# Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
# for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Uncomment the following to debug the script
#set -x

if [ -z "${USERNAME}" ]; then
    echo "Set USERNAME to the default user to be created"
    exit 1
fi
GROUPNAME=${GROUPNAME:-${USERNAME}}

echo "Username: '${USERNAME}'"
echo "Groupname: '${GROUPNAME}'"

#IFS=: read gname x USER_ID dump<<<$(getent passwd $USERNAME)
echo "UID: '${USER_ID}'"
#IFS=: read gname x GID <<<$(getent group $GROUPNAME)
echo "USER_GID: '${USER_GID}'"

if [ -z "${USERNAME}" ]; then
    echo "Set USERNAME to the default user to be created"
    exit 1
fi
if [ -z "${USER_GID}" ]; then
    echo "Set USER_GID to the default UID of the user to be created"
    exit 1
fi
if [ -z "${GROUP_GID}" ]; then
    echo "Set GROUP_GID to the default GID of the user to be created"
    exit 1
fi

## Non-vagrant configuration
if [ ${USERNAME} != vagrant ] && [ -f /etc/debian_version ]; then
    grep $USERNAME /etc/passwd >/dev/null 2>&1
    if [ $? -ne 0 ]; then
	adduser --uid $USER_ID --gid $USER_GID $USERNAME
	adduser $USERNAME sudo
	mkdir -p /home/${USERNAME}/trip-server-2
    fi
    grep -E "trip2.*${USERNAME}/trip-server-2 9p" /etc/fstab
    if [ $? -ne 0 ]; then
	cat << EOF >> /etc/fstab
trip2 /home/$USERNAME/trip-server-2 9p trans=virtio,msize=104857600,noauto 0 0
EOF
    fi
    systemctl daemon-reload
    mount trip2
    apt-get install anacron
fi
