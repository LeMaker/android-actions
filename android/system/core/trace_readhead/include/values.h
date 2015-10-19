/* ureadahead
 *
 * Copyright © 2009 Canonical Ltd.
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

#ifndef UREADAHEAD_VALUES_H
#define UREADAHEAD_VALUES_H

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE
#endif

#include <sys/types.h>

#include <limits.h>

#include <fcntl.h>

int get_value (int dfd, const char *path, int *value);
int set_value (int dfd, const char *path, int value, int *oldvalue);
char *fgets_line (const void *parent,FILE * stream);

#endif /* UREADAHEAD_VALUES_H */

