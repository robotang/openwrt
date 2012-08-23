/*
 *  Captures frames from a webcamera device and writes them to disk  
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
#include <unistd.h>

#define OUTPUT_DIR      "/tmp"
#define MSLEEP(x)       usleep(1000*(x))

int main(int argc, char *argv[])
{
    int i = 20;
    camera_t camera;
    
    camera_init(&camera, "/dev/video0");
    
    while(i > 0)
    {
        camera_grab(&camera);
        camera_save(&camera, OUTPUT_DIR); 
        i--;
        
        MSLEEP(200);
    }

    camera_close(&camera);

    return 0;
}

