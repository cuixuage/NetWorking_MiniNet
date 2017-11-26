#!/usr/bin/python
# -*- coding: UTF-8 -*-
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI

# Mininet will assign an IP address for each interface of a node 
# automatically, but hub or switch does not need IP address.
def clearIP(n):
    for iface in n.intfList():
        n.cmd('ifconfig %s 0.0.0.0' % (iface))

class MyTopo(Topo):
    def build(self):
		m1 = self.addHost('m1')					#master
		w1 = self.addHost('w1')
		w2 = self.addHost('w2')
		s1 = self.addSwitch('s1')

		self.addLink(m1, s1, bw=20)     		#bw是带宽
		self.addLink(w1, s1, bw=10)
		self.addLink(w2, s1, bw=10)

if __name__ == '__main__':
	topo = MyTopo()
	net = Mininet(topo = topo, link = TCLink)   #controller = None不能加上

	m1, w1, w2, s1 = net.get('m1', 'w1','w2','s1')
	m1.cmd('ifconfig m1-eth0 10.0.0.1/24')  			#前24位是子网掩码
	w1.cmd('ifconfig w1-eth0 10.0.0.2/24')
	w2.cmd('ifconfig w2-eth0 10.0.0.3/24')

    # clearIP(s1)   #hub or switch don't need IP address

	net.start()
	CLI(net)
	net.stop()
