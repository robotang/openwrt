/*
 *  Records wifi signal strength with GPS position and time stamps
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "gps.h"

int main(void)
{
    int result, i;    
    gps_t gps;
    
    //gps_init("data/mish_gps.txt", GPS_LOGFILE);
    gps_init("/dev/ttyUSB0", 9600);
    
    for(i = 0; i < 100; i++)
    {
        result = gps_update(&gps);
        usleep(100000);
    }
    
    gps_close();
    
    return 0;
}

