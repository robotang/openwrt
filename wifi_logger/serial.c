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

#include "serial.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

#define BUFFER_LENGTH   200

static int fd;
static char rx[BUFFER_LENGTH];

int serial_init(const char *dev, int baud, bool blocking)
{
    struct termios options;
    int flags;
    
    flags = O_RDWR;
    if(!blocking)
        flags |= O_NDELAY;

    fd = open(dev, flags);
    if(fd == -1)
    {
        printf("Opening %s port failed\n", dev);
        return -1; 
    }
    
    if(!blocking)
        fcntl(fd, F_SETFL, FNDELAY);

    tcflush(fd, TCIFLUSH);
	tcgetattr(fd, &options);
	
	switch(baud)
	{
        case 4800:
        {
            cfsetispeed(&options, B4800);
            cfsetospeed(&options, B4800);
        } break;
        
        case 9600:	
        {
            cfsetispeed(&options, B9600); 
            cfsetospeed(&options, B9600); 		
        } break;
        
        case 38400:	
        {
            cfsetispeed(&options, B38400); 
            cfsetospeed(&options, B38400); 				
        } break;
        
        case 57600:	
        {
            cfsetispeed(&options, B57600); 
            cfsetospeed(&options, B57600); 				
        } break;
        
        case 115200:	
        {
            cfsetispeed(&options, B115200); 
            cfsetospeed(&options, B115200); 				
        } break;
        
        default:	
        {
            cfsetispeed(&options, B9600); 
            cfsetospeed(&options, B9600); 				
        } break;
    }
        
    options.c_cflag &= ~CRTSCTS;
    options.c_lflag = 0;
    options.c_iflag = 0;
    options.c_oflag = 0;

    tcsetattr(fd, TCSANOW, &options);
    
    rx[0] = '\0';
    
    return 0;
}

void serial_close(void)
{
    if(fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

void serial_flush(void)
{
    if(fd != -1) 
        ioctl(fd, TCFLSH, 2);
}

int serial_readline(char *data)
{
    int result;
    
    if(fd != -1)
    {
        int length, i;
        bool complete;
        
        complete = false;
                
        do
        {
            length = strlen(rx);
            /* Append read characters onto end of buffer */
            result = read(fd, rx + length, BUFFER_LENGTH - length);
            if(result > 0)
            {
                char *pch;
                
                rx[length + result] = '\0';
                length += result;
                
                /* Look for new line character */
                pch = strchr(rx, '\n');
                i = pch - rx + 1; 
                if(i > 0)
                {
                    char tmp[BUFFER_LENGTH];
                    /* Copy complete message to data and remove this from rx buffer */
                    strcpy(tmp, rx);                    
                    strncpy(data, rx, i);
                    data[i] = '\0';
                    strcpy(rx, tmp + i);
                    rx[length - i] = '\0';
                    complete = true;
                }
            }            
            usleep(10000);    
        } while(!complete);
    }
    else
        result = -1;
    
    return result;
}

int serial_writeline(char *data) 
{
    int result;
    
    if(fd != -1)
    {
        result = write(fd, data, sizeof(data));
        tcdrain(fd);
    }
    else
        result = -1;
        
    return result;
}

