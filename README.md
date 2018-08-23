mdio-tool
=========
This is tool to read and write MII registers from ethernet physicals under linux.
It has been tested with Realtek and Marvell PHY's connected via PCIe and should work
with all drivers implementing the mdio ioctls.

mdio-tool comes with ABSOLUTELY NO WARRANTY; Use with care!

Syntax:
		Usage mii-tool -d[dev] [-r/-w] -l[len] -p[phyadd] -a[reg] -v[val]
                       	       -h for printing this help

		sudo mdio-tool -d eth0 -r -a 0 -l 5
		Probed phyaddr: 0
		PHY: 0|REG: 0 ---> 0x1140
		PHY: 0|REG: 1 ---> 0x796d
		PHY: 0|REG: 2 ---> 0x0141
		PHY: 0|REG: 3 ---> 0x0c24
		PHY: 0|REG: 4 ---> 0x0de1

		sudo mdio-tool -d eth0 -w -a 0 -v 0x1340
		Probed phyaddr: 0
		PHY: 0|REG: 0 <--- 0x1340

		


