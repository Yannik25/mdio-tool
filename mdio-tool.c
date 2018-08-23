/*
mdio-tool allow for direct access to mdio registers in a network phy.

Routines are based on mii-tool: http://freecode.com/projects/mii-tool

mdio-tool comes with ABSOLUTELY NO WARRANTY; Use with care!

Copyright (C) 2013 Pieter Voorthuijsen

mdio-tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

mdio-tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with mdio-tool.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/sockios.h>

#ifndef __GLIBC__
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif
#include "mii.h"

#define MAX_ETH		8		/* Maximum # of interfaces */

static int skfd = -1;		/* AF_INET socket for ioctl() calls. */
static struct ifreq ifr;

/*--------------------------------------------------------------------*/

static int mdio_read(int skfd, int location)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	return -1;
    }
    return mii->val_out;
}

static void mdio_write(int skfd, int location, int value)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    mii->val_in = value;
    if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
    }
}

static void print_help(FILE *std)
{
	fprintf(std, "Usage mii-tool -d[dev] [-r/-w] -l[len] -p[phyadd] -a[reg] -v[val]\n");
        fprintf(std, "               -h for printing this help\n");
}

int main(int argc, char **argv)
{
	char devname[64] = {0};
	int opt, phyaddr = 0, reg = 0, val = 0;	
	int len = 1, o_devname = 0, o_phyaddr = 0;
	int o_r = 0, o_w = 0, o_reg = 0, o_val = 0;
	
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;

	if(argc == 1)
	{
		print_help(stdout);
		return 0;
	}

	while ((opt = getopt(argc, argv, "l:hd:rwp:a:v:")) != -1) {
                switch (opt) {
                case 'd':
                    strncpy(devname, optarg, sizeof(devname));
		    o_devname = 1;
                    break;
                case 'r':
                    o_r = 1;
                    break;
		case 'l':
			len = strtol(optarg, NULL, 0);
			break;
		case 'w':
			o_w = 1;
			break;
		case 'p':
			phyaddr = strtol(optarg, NULL, 0);
			o_phyaddr = 1;
			break;
		case 'a':
			reg = strtol(optarg, NULL, 0);
			o_reg = 1;
			break;
		case 'v':
			val = strtol(optarg, NULL, 0);
			o_val = 1;
			break;
		case 'h':
                   print_help(stdout);
                   return 0;
			
                default: /* '?' */
                   print_help(stderr);
                   exit(EXIT_FAILURE);
               }
           }

#if 0
	printf("o_devname: %d\n", o_devname);
	printf("Device: %s\n", devname);
	printf("o_r: %d\n", o_r);
	printf("o_w: %d\n", o_w);
	printf("len: %d\n", len);	
	printf("o_phyaddr: %d\n", o_phyaddr);
	printf("phyaddr: %d\n", phyaddr);
	printf("o_reg: %d\n", o_reg);
	printf("reg: %d\n", reg);
	printf("o_val: %d\n", o_val);
	printf("val: 0x%04x\n", val);
#endif
	if(o_devname == 0)
	{
		fprintf(stderr, "Missing ifname, specify -d option\n");
		exit(EXIT_FAILURE);
	}

	if(o_r && o_w)
	{
		fprintf(stderr, "You cannot specify -r and -w ad the same time\n");
		exit(EXIT_FAILURE);
	}	
	
	if(o_r && !o_reg)
	{
		fprintf(stderr, "-r operation require -a[reg] parameter\n");
		exit(EXIT_FAILURE);
	}

	if(o_w && (!o_reg || !o_val ))
	{
		fprintf(stderr, "-w operation require -a[reg] and -v[val] parameters\n");
		exit(EXIT_FAILURE);
	}

	/* Open a basic socket. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("socket");
		return -1;
	}

	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, devname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
		if (errno != ENODEV)
		fprintf(stderr, "SIOCGMIIPHY on '%s' failed: %s\n",
			devname, strerror(errno));
		return -1;
	}

	printf("Probed phyaddr: %d\n", mii->phy_id);
	
	if(o_phyaddr)
	{
		mii->phy_id = phyaddr;
		printf("Override phy address to: %d\n", mii->phy_id);
	}

	if(o_r) {
		while(len)
		{		
			printf("PHY: %d|REG: %d ---> 0x%04x\n", mii->phy_id, reg, mdio_read(skfd, reg));
			len--;
			reg++;		
		}
	}
	else if(o_w) {
		printf("PHY: %d|REG: %d <--- 0x%04x\n", mii->phy_id, reg, val);		
		mdio_write(skfd, reg, val);
	}
	else {
		printf("Fout!\n");
	}

	close(skfd);
}
