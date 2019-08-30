#!/bin/busybox sh
#分配固定IP地址



ifconfig rndis0 172.168.2.70 netmask 255.255.255.0 up
route add default gw 172.168.2.1 rndis0

# 启动IP地址分配
if busybox ps | grep dnsmasq | grep -v grep; then
	echo "dnsmasq already exist, restart ..."
	busybox killall dnsmasq 
	dnsmasq -C /data/bin/dnsmasq.conf &
else
	echo "dnsmasq start ..."
	dnsmasq -C /data/bin/dnsmasq.conf &
fi

#启动PCBA测试中的被搜索服务
if busybox ps | grep echo_discovery | grep -v grep; then
	echo "echo_discovery already exist, restart ..."
	busybox killall echo_discovery
	echo_discovery &
else
	echo "echo_discovery start..."
	echo_discovery &
fi