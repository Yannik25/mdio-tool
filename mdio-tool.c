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

#define DEBUG		0
#define MAX_ETH		8		/* Maximum # of interfaces */
#define MMD_CTRL 0xD
#define MMD_DATA 0xE

static int skfd = -1;		/* AF_INET socket for ioctl() calls. */
static struct ifreq ifr;

/*--------------------------------------------------------------------*/



static int mdio_read(int skfd, int location)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
#if DEBUG
	printf("R:%d\n", location);
#endif
    if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	return -1;
    }

    return mii->val_out;
}

static int mdio_write(int skfd, int location, int value)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    mii->val_in = value;
    
#if DEBUG
	printf("W:%d,0x%04x\n", location, value);
#endif
	if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	return -1;
    }
    return 0;
}

static int mdio_mmd_read(int skfd, int mmddev, int location)
{	
	int ret;
	
	ret = mdio_write(skfd, MMD_CTRL, mmddev);
	if(ret == -1)
		goto exit;
	
	ret = mdio_write(skfd, MMD_DATA, location);
	if(ret == -1)
		goto exit;
	
	ret = mdio_write(skfd, MMD_CTRL, 0x4000 | mmddev);
	if(ret == -1)
		goto exit;
	
	ret = mdio_read(skfd, MMD_DATA);

exit:
	return ret;	
}

static int mdio_mmd_write(int skfd, int mmddev, int location, int value)
{	
	int ret;
		
	ret = mdio_write(skfd, MMD_CTRL, mmddev);
	if(ret == -1)
		goto exit;

	ret = mdio_write(skfd, MMD_DATA, location);
	if(ret == -1)
		goto exit;

	ret = mdio_write(skfd, MMD_CTRL, 0x4000 | mmddev);
	if(ret == -1)
		goto exit;

	ret = mdio_write(skfd, MMD_DATA, value);
exit:
	return ret;
}

static void print_help(char *name, FILE *std)
{
	fprintf(std, "Usage %s -e[dev] [-r/-w] -l[len] -m[mmddev] -p[phyadd] -a[reg] -v[val]\n", name);
	fprintf(std, "               mmd mode requires mmd dev\n");        
	fprintf(std, "               -h for printing this help\n");
}

int main(int argc, char **argv)
{
	char devname[64] = {0};
	int opt, phyaddr = 0, reg = 0, val = 0, mmddev = 0;	
	int len = 1, o_devname = 0, o_phyaddr = 0;
	int o_r = 0, o_w = 0, o_reg = 0, o_val = 0;
	int o_mmd = 0;

	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;

	if(argc == 1)
	{
		print_help(argv[0], stdout);
		return 0;
	}

	while ((opt = getopt(argc, argv, "e:l:hrwm:p:a:v:")) != -1) {
                switch (opt) {
                case 'e':
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
		case 'm':
			o_mmd = 1;
			mmddev = strtol(optarg, NULL, 0);
			break;

		case 'h':
                   print_help(argv[0], stdout);
                   return 0;
			
                default: /* '?' */
                   print_help(argv[0], stderr);
                   exit(EXIT_FAILURE);
               }
           }

#if DEBUG
	printf("o_devname: %d\n", o_devname);
	printf("Device: %s\n", devname);
	printf("o_mmd: %d\n", o_mmd);
	printf("mmddev: %d\n", mmddev);
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
			if(o_mmd)
			{
				printf("(MMD)PHY: %d| DEV: %d| REG: %d ---> 0x%04x\n", mii->phy_id, mmddev, reg, mdio_mmd_read(skfd, mmddev, reg));
			}
			else
			{			
				printf("PHY: %d|REG: %d ---> 0x%04x\n", mii->phy_id, reg, mdio_read(skfd, reg));
			}
			len--;
			reg++;		
		}
	}
	else if(o_w) {
		if(o_mmd)
		{
			printf("(MMD)PHY: %d| DEV: %d| REG: %d <--- 0x%04x\n", mii->phy_id, mmddev, reg, val);		
			mdio_mmd_write(skfd, mmddev, reg, val);
		}
		else
		{
			printf("PHY: %d|REG: %d <--- 0x%04x\n", mii->phy_id, reg, val);		
			mdio_write(skfd, reg, val);
		}
	}
	else {
		printf("Fout!\n");
	}

	close(skfd);

	return 0;
}
