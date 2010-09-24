#!/bin/sh
ssh rick@192.168.1.50 pkill server
scp -P 22 server main.lua utils.lua robot.lua nav.lua rick@192.168.1.50:rp6serv/
ssh rick@192.168.1.50 /home/rick/rp6serv/runrp6serv.sh

