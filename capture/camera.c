/*
 *  Camera interface code modularised from 'V4L2 video capture example'
 *
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

#include "camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

/*
 * Private function prototypes
 */

static void errno_exit(const char *s);
static int xioctl(int fd, int request, void *arg);
static void open_camera(camera_t *camera);
static void init_camera(camera_t *camera);
static void start_capture(camera_t *camera);
static int read_frame(camera_t *camera);
static void stop_capture(camera_t *camera);
static void uninit_camera(camera_t *camera);
static void init_read(camera_t *camera, unsigned int buffer_size);
static void init_mmap(camera_t *camera);
static void init_userp(camera_t *camera, unsigned int buffer_size);

/*
 * Public functions
 */

int camera_init(camera_t *camera, const char *dev_name)
{
    camera->fd = -1;
    
    strcpy(camera->dev_name, dev_name);
    
    if(camera->width <= 0)
        camera->width = 640;
    if(camera->height <= 0)
        camera->height = 480;
    if((camera->io != IO_METHOD_READ) || (camera->io != IO_METHOD_MMAP) || (camera->io != IO_METHOD_USERPTR))
        camera->io = IO_METHOD_MMAP;

    camera->n_channels = 4;
    camera->pixel_format = V4L2_PIX_FMT_YUV420;
    
    open_camera(camera);
    init_camera(camera);
    start_capture(camera);
    return 0;
}

int camera_grab(camera_t *camera)
{
    read_frame(camera);
    return 0;
}

int camera_save(camera_t *camera, const char *output_dir)
{
    static int i = 0;
    char filename[1024];
    int fd;
    ssize_t written = 0;

    snprintf(filename, sizeof(filename), "%s/webcam-%5.5d.%s", output_dir, i++, camera->pixel_format == V4L2_PIX_FMT_YUV420 ? "yuv" : "raw");

    fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd < 0)
    {
        fputc('*', stdout);
        fflush(stdout);
        return -1;
    }
    do
    {
        int ret;
        ret = write(fd, camera->channel[0].start + written, camera->channel[0].length - written);
        if (ret < 0)
        {
            fputc('+', stdout);
            fflush(stdout);
            return -1;
        }
        written += ret;
    } while (written < camera->channel[0].length);
    close(fd);

    fputc('.', stdout);
    fflush(stdout);
    
    return 0;
}

int camera_close(camera_t *camera)
{
    stop_capture(camera);
    uninit_camera(camera);
    
    if(close(camera->fd) == -1)
        errno_exit("close");

    camera->fd = -1;
    
    return 0;
}

/*
 * Private functions
 */

static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
    int r;
    do
    {
        r = ioctl(fd, request, arg); 
    } while (-1 == r && EINTR == errno);
    return r;
}

static void open_camera(camera_t *camera)
{
    struct stat st;

    if(stat(camera->dev_name, &st) == -1)
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", camera->dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", camera->dev_name);
        exit(EXIT_FAILURE);
    }
    
    camera->fd = open(camera->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
    if(camera->fd == -1)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", camera->dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void init_camera(camera_t *camera)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;

    if(xioctl(camera->fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        if(EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\n", camera->dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n", camera->dev_name);
        exit(EXIT_FAILURE);
    }

    switch(camera->io)
    {
        case IO_METHOD_READ:
        {
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
            {
                fprintf(stderr, "%s does not support read i/o\n", camera->dev_name);
                exit(EXIT_FAILURE);
            }
        } break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
        {
            if(!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                fprintf(stderr, "%s does not support streaming i/o\n", camera->dev_name);
                exit(EXIT_FAILURE);
            }
        } break;
    }

    /* Select video input, video standard and tune here. */

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(xioctl(camera->fd, VIDIOC_CROPCAP, &cropcap) == -1)
    {
        /* Errors ignored. */
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if(xioctl(camera->fd, VIDIOC_S_CROP, &crop) == -1)
    {
        switch(errno)
        {
            case EINVAL:
            {
                /* Cropping not supported. */
            } break;
            
            default:
            {
                /* Errors ignored. */
            } break;
        }
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = camera->width;
    fmt.fmt.pix.height = camera->height;
    fmt.fmt.pix.pixelformat = camera->pixel_format;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if(xioctl(camera->fd, VIDIOC_S_FMT, &fmt) == -1)
        errno_exit("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */
    camera->width = fmt.fmt.pix.width;
    camera->height = fmt.fmt.pix.height;

    switch(camera->io)
    {
        case IO_METHOD_READ:
        {
            init_read(camera, fmt.fmt.pix.sizeimage);
        } break;

        case IO_METHOD_MMAP:
        {
            init_mmap(camera);
        } break;

        case IO_METHOD_USERPTR:
        {
            init_userp(camera, fmt.fmt.pix.sizeimage);
        } break;
    }
}

static void start_capture(camera_t *camera)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch(camera->io)
    {
        case IO_METHOD_READ:
        {
            /* Nothing to do. */
        } break;

        case IO_METHOD_MMAP:
        {
            for(i = 0; i < camera->n_channels; ++i)
            {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if(xioctl(camera->fd, VIDIOC_QBUF, &buf) == -1)
                    errno_exit("VIDIOC_QBUF");
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if(xioctl(camera->fd, VIDIOC_STREAMON, &type) == -1)
                errno_exit("VIDIOC_STREAMON");
        } break;

        case IO_METHOD_USERPTR:
        {
            for(i = 0; i < camera->n_channels; ++i)
            {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.m.userptr = (unsigned long) camera->channel[i].start;
                buf.length = camera->channel[i].length;

                if(xioctl(camera->fd, VIDIOC_QBUF, &buf) == -1)
                    errno_exit("VIDIOC_QBUF");
            }
            
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if(xioctl(camera->fd, VIDIOC_STREAMON, &type) == -1)
                errno_exit("VIDIOC_STREAMON");
        } break;
    }
}

static int read_frame(camera_t *camera)
{
    struct v4l2_buffer buf;
    unsigned int i;
    ssize_t read_bytes;
    unsigned int total_read_bytes;

    switch(camera->io)
    {
        case IO_METHOD_READ:
        {
            total_read_bytes = 0;
            do
            {
                read_bytes = read(camera->fd, camera->channel[0].start, camera->channel[0].length);
                if(read_bytes < 0)
                {
                    switch(errno)
                    {
                        case EIO:
                        case EAGAIN:
                            continue;
                        default:
                            errno_exit("read");
                    }
                }
                total_read_bytes += read_bytes;

            } while(total_read_bytes < camera->channel[0].length);
        } break;

        case IO_METHOD_MMAP:
        {
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if(xioctl(camera->fd, VIDIOC_DQBUF, &buf) == -1)
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("VIDIOC_DQBUF");
                }
            }

            assert(buf.index < camera->n_channels);

            if(xioctl(camera->fd, VIDIOC_QBUF, &buf) == -1)
                errno_exit("VIDIOC_QBUF");
        } break;

        case IO_METHOD_USERPTR:
        {
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;

            if(xioctl(camera->fd, VIDIOC_DQBUF, &buf) == -1)
            {
                switch(errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("VIDIOC_DQBUF");
                }
            }

            for(i = 0; i < camera->n_channels; ++i)
            {
                if(buf.m.userptr == (unsigned long) camera->channel[i].start && buf.length == camera->channel[i].length)
                    break;
            }
            
            assert(i < camera->n_channels);

            if(xioctl(camera->fd, VIDIOC_QBUF, &buf) == -1)
                errno_exit("VIDIOC_QBUF");
        } break;
    }

    return 1;
}

static void stop_capture(camera_t *camera)
{
    enum v4l2_buf_type type;

    switch(camera->io)
    {
        case IO_METHOD_READ:
        {
            /* Nothing to do. */
        } break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
        {
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if(xioctl(camera->fd, VIDIOC_STREAMOFF, &type) == -1)
                errno_exit("VIDIOC_STREAMOFF");
        } break;
    }
}

static void uninit_camera(camera_t *camera)
{
    unsigned int i;

    switch(camera->io)
    {
        case IO_METHOD_READ:
        {
            free(camera->channel[0].start);
        } break;

        case IO_METHOD_MMAP:
        {
            for(i = 0; i < camera->n_channels; ++i)
            {    
                if(munmap(camera->channel[i].start, camera->channel[i].length) == -1)
                    errno_exit("munmap");
            }
        } break;

        case IO_METHOD_USERPTR:
        {
            for (i = 0; i < camera->n_channels; ++i)
                free(camera->channel[i].start);
        } break;
    }

    free(camera->channel);
}

static void init_read(camera_t *camera, unsigned int buffer_size)
{
    camera->channel = calloc(1, sizeof(camera->channel));

    if(!camera->channel)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    camera->channel[0].length = buffer_size;
    camera->channel[0].start = malloc(buffer_size);

    if(!camera->channel[0].start)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
}

static void init_mmap(camera_t *camera)
{
    struct v4l2_requestbuffers req;
    int i;

    CLEAR(req);

    req.count = camera->n_channels;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(xioctl(camera->fd, VIDIOC_REQBUFS, &req) == -1)
    {
        if(errno == EINVAL)
        {
            fprintf(stderr, "%s does not support memory mapping\n", camera->dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", camera->dev_name);
        exit(EXIT_FAILURE);
    }

    camera->channel = calloc(req.count, sizeof(camera->channel));

    if(!camera->channel)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < camera->n_channels; ++i)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(xioctl(camera->fd, VIDIOC_QUERYBUF, &buf) == -1)
            errno_exit("VIDIOC_QUERYBUF");

        camera->channel[i].length = buf.length;
        camera->channel[i].start = mmap(NULL /* start anywhere */,
                buf.length,
                PROT_READ | PROT_WRITE /* required */,
                MAP_SHARED /* recommended */,
                camera->fd, buf.m.offset);

        if(camera->channel[i].start == MAP_FAILED)
            errno_exit("mmap");
    }
}

static void init_userp(camera_t *camera, unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
    int i;

    CLEAR(req);

    req.count = camera->n_channels;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if(xioctl(camera->fd, VIDIOC_REQBUFS, &req) == -1)
    {
        if(errno == EINVAL)
        {
            fprintf(stderr, "%s does not support user pointer i/o\n", camera->dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    camera->channel = calloc(4, sizeof(camera->channel));

    if(!camera->channel)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < camera->n_channels; ++i)
    {
        camera->channel[i].length = buffer_size;
        camera->channel[i].start = malloc(buffer_size);

        if (!camera->channel[i].start)
        {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
    }
}

