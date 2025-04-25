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

#set -x

# Script to simplify creating a new Vagrant environment, as Vagrant version
# 2.2.19 always fails to install the guest additions with the Debian Bento
# boxes.

vagrant status --machine-readable | grep 'The VM is running' >/dev/null
if [ $? -ne 0 ]; then
    vagrant up
fi
vagrant vbguest --status | egrep -E '\[default\] GuestAdditions [.[:digit:]]+ running --- OK\.' >/dev/null
if [ $? -eq 0 ]; then
    echo 'Vagrant appears to be running with the guest additions correctly installed'
    exit 0
fi

vagrant plugin list --machine-readable | grep 'plugin-name,vagrant-vbguest' >/dev/null
if [ $? -ne 0 ]; then
    vagrant plugin install vagrant-vbguest
fi

## Typical error message if the plugin is not up-to-date on the host

# Got different reports about installed GuestAdditions version:
# Virtualbox on your host claims:   5.2.0
# VBoxService inside the vm claims: 6.1.30
# Going on, assuming VBoxService is correct...
# [default] GuestAdditions seems to be installed (6.1.30) correctly, but not running.

vagrant vbguest --status | egrep -E '\[default\] GuestAdditions seems to be installed \([.[:digit:]]+\) correctly, but not running\.' >/dev/null
if [ $? -eq 0 ]; then
    vagrant vbguest --do install
    vagrant halt
fi
vagrant status --machine-readable | grep 'The VM is running'
if [ $? -ne 0 ]; then
    vagrant up
fi
