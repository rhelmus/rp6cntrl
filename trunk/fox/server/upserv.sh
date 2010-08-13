#!/bin/sh
scp -P 22 server main.lua rick@192.168.1.50:rp6serv/
ssh rick@192.168.1.50 /home/rick/rp6serv/runrp6serv.sh

