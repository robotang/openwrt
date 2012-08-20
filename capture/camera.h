/*
 *  Copyright (C) 2012, Robert Tang <opensource@robotang.co.nz>
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public Licence
 *  along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <string.h>

enum {IO_METHOD_READ = 1, IO_METHOD_MMAP, IO_METHOD_USERPTR};

typedef struct
{
    void *start;
    size_t length;
} channel_t;

typedef struct
{
    int width, height, io, fd, pixel_format, n_channels;
    char dev_name[100];
    channel_t *channel;
} camera_t;

int camera_init(camera_t *camera, const char *dev_name);
int camera_grab(camera_t *camera);
int camera_save(camera_t *camera, const char *output_dir);
int camera_close(camera_t *camera);

#endif

