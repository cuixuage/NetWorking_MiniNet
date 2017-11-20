#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI

class RouterTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        n1 = self.addHost('n1') # nat

        self.addLink(h1, n1)
        self.addLink(h2, n1)

if __name__ == '__main__':
    topo = RouterTopo()
    net = Mininet(topo = topo, controller = None) 

    h1, h2, n1 = net.get('h1', 'h2', 'n1')

    h1.cmd('ifconfig h1-eth0 10.21.0.1/16')
    h1.cmd('route add default gw 10.21.0.254')

    n1.cmd('ifconfig n1-eth0 10.21.0.254/16')	   #internal_ip ???
    n1.cmd('ifconfig n1-eth1 159.226.39.43/24')   #external_ip  ???

    h2.cmd('ifconfig h2-eth0 159.226.39.123/24')

    net.start()
    CLI(net)
    net.stop()
