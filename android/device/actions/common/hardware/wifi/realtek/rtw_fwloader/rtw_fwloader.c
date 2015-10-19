/******************************************************************************
 *
 * Copyright(c) 2014 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <net/if.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#define LOG_TAG "rtw_fwloader"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif

static const char DRIVER_STATUS_PROP[] = "wlan.driver.status";
static const char INTERFACE_NAME_PROP[] = "wifi.interface";
#define WIFI_TEST_INTERFACE "sta"

static char primary_iface[PROPERTY_VALUE_MAX];

int get_wifi_ifname_from_prop(char *ifname)
{
	ifname[0] = '\0';
	if (property_get(INTERFACE_NAME_PROP, ifname, WIFI_TEST_INTERFACE)
		&& strcmp(ifname, WIFI_TEST_INTERFACE) != 0)
		return 0;
	
	ALOGE("Can't get wifi ifname from property \"%s\"", INTERFACE_NAME_PROP);
	return -1;
}

int check_wifi_ifname_from_proc(char *ifname)
{
#define PROC_NET_DEV_PATH "/proc/net/dev"
#define MAX_WIFI_IFACE_NUM 20

	char linebuf[1024];
	char wifi_iface[IFNAMSIZ+1] = {0};
	int i, ret = -1;
	int match = -1; /* if matched, this means the index*/
	FILE *f;

	if (!ifname) {
		ALOGE("ifname is NULL\n");
		goto exit;
	}
	ALOGE("%s ifname:%s\n", __func__, ifname);

	f = fopen(PROC_NET_DEV_PATH, "r");
	if (!f) {
		ALOGE("Unable to read %s: %s\n", PROC_NET_DEV_PATH, strerror(errno));
		goto exit;
	}

	/* check wifi interfaces form PROC_NET_DEV_PATH */
	while(fgets(linebuf, sizeof(linebuf)-1, f)) {

		if (strchr(linebuf, ':')) {
			char *dest = wifi_iface;
			char *p = linebuf;

			while(*p && isspace(*p))
				++p;
			while ((*p && *p != ':')) {
				if (dest - wifi_iface < IFNAMSIZ)
					*dest++ = *p++;
			}
			*dest = '\0';

			match++;

			if (strcmp(ifname, wifi_iface) == 0)  {
				ALOGD("%s: find %s\n", __func__, wifi_iface);
				ret = 0;
				break;
			}
		}
	}
	fclose(f);

exit:
	return ret;
}

int main(int argc, char ** argv)
{
    int count = 50; /* wait at most 5 seconds for completion */

	if (get_wifi_ifname_from_prop(primary_iface) != 0)
		goto exit;

	while (check_wifi_ifname_from_proc(primary_iface) != 0 && count-- > 0) {
		usleep(100000);
	}
	if (check_wifi_ifname_from_proc(primary_iface) != 0) {
		ALOGE("check_wifi_ifname_from_proc(%s) fail\n", primary_iface);
		goto exit;
	}

	property_set(DRIVER_STATUS_PROP, "ok");

exit:
	return 0;
}
