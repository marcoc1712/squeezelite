#!/sbin/openrc-run
# Copyright 1999-2016 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

description="A headless Squeezebox emulator using ALSA audio output"

user="squeezelite:squeezelite"
logfile="/var/log/squeezelite-R2.log"

start_stop_daemon_args="--user $user"

command="/usr/bin/squeezelite-R2"
command_args="
        -f $logfile
        ${SL_OPTS}
"

command_background=yes
pidfile=/run/squeezelite-R2.pid

depend() {
    need net
    use alsasound
    after bootmisc
}

start_pre() {
    checkpath --file --owner $user --mode 0644 $logfile
}

