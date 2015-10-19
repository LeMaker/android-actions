/* ureadahead
 *
 * values.c - dealing with proc/sysfs values
 *
 * Copyright © 2009 Canonical Ltd.
 * Author: Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#define _ATFILE_SOURCE


#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "values.h"



unsigned short xstrtoshortint(char *str)
{
	if(str==NULL)
	{
		return 0;
	}
    int len = strlen(str);

    unsigned short usvalue = 0;
    for (int i = 0; i < len; i++)
    {
        if ((str[i] <= '9' && str[i] >= '0'))
        {
        	usvalue = usvalue * 16 + (str[i] - '0');
        }
        else if ((str[i] >= 'a' && str[i] <= 'f'))
        {
        	usvalue = usvalue * 16 + (str[i] - 'a') + 10;
        }
        else if ((str[i] >= 'A' && str[i] <= 'F'))
        {
        	usvalue = usvalue * 16 + (str[i] - 'A') + 10;
        }

    }
    return usvalue;
}


int bitcount(char *str)
{
	unsigned short usvalue=xstrtoshortint(str);


unsigned int uibitcount=0;

unsigned short usMask = 0x0001;

	while (usMask) {

		if ((usvalue & usMask) != 0) {
			uibitcount++;
		}

		usMask <<= 1;
	}

return uibitcount;

}

int get_value(int dfd, const char *path, int * value) {
	int     fd;
	char    buf[80];
	ssize_t len;

	assert (path != NULL);
	assert (value != NULL);

	fd = openat (dfd, path, O_RDONLY);
	if (fd < 0)
		return -1;

	len = read (fd, buf, sizeof buf);
	if (len < 0) {
		close (fd);
		return -1;
	}

	buf[len] = '\0';


	if (close (fd) < 0)
		return -1;

   if(value)
	{
	   *value = len ? atoi (buf) : 0;

	}else
	{

	/*get tracing_cpumask to set buffer size*/
		return bitcount(buf);

	}



	return 0;
}

int set_value(int dfd, const char *path, int value, int * oldvalue) {
	int     fd;
	char    buf[80];
	ssize_t len;

	assert (path != NULL);

	fd = openat (dfd, path, O_RDWR);
	if (fd < 0)
		return -1;

	if (oldvalue) {
		len = read (fd, buf, sizeof buf);
		if (len < 0) {
			close (fd);
			return -1;
		}

		buf[len] = '\0';
		*oldvalue = atoi (buf);

		assert (lseek (fd, 0, SEEK_SET) == 0);
	}

	snprintf (buf, sizeof buf, "%d", value);

	len = write (fd, buf, strlen (buf));
	if (len < 0) {
		close (fd);
		return -1;
	}

	assert (len > 0);

	if (close (fd) < 0)
		return -1;

	return 0;
}


char *fgets_line (const void *parent,FILE* stream)
{
        char * buf = NULL;
        size_t buf_sz = 0;
	size_t buf_len = 0;

	assert (stream != NULL);

	for (;;) {
		char *ret;
		char *pos;

		if (buf_sz <= (buf_len + 1)) {
			buf_sz += 4096;
			buf = malloc (buf_sz);
		}

		ret = fgets (buf + buf_len, buf_sz - buf_len, stream);
		if ((! ret) && (! buf_len)) {
			free(buf);
			return NULL;
		} else if (! ret) {
			return buf;
		}

		buf_len += strlen (ret);
		pos = strchr (ret, '\n');
		if (pos) {
			*pos = '\0';
			break;
		}
	}

	return buf;
}
