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

#include "gps.h"

#include "serial.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static uint8_t nmea_checksum(const char *string);
static bool nmea_validate(char *string);

static FILE *input;

static bool readlog;

void gps_init(char *dev, int baud)
{
    readlog = (baud == GPS_LOGFILE) ? true : false;
    
    if(readlog)
    {       
        input = fopen(dev, "r");
        if(input == NULL)
        {
	        printf("loading %s logfile failed\n", dev); 
	        exit(0);
        }
    }
    else
    {
        serial_init(dev, baud, false);
    }
    
}

void gps_close(void)
{
    if(readlog)
        fclose(input);
    else
        serial_close();        
}

int gps_update(gps_t *gps)
{
    char data[1000];
    int result;
    
    if(readlog)
    {
        result = fgets(data, sizeof(data) - 1, input);
	}
	else
	{        
        result = serial_readline(data);
    }
    
    if(result > 0)
        printf("%d %s\n", result, data);    
    
    return result;
}

/*
 * Private functions
 */
 
static uint8_t nmea_checksum(const char *string)
{
	uint8_t checksum = 0;
	
	while(*string)
		checksum ^= *string++;
		
	return checksum;
} 

static bool nmea_validate(char *string)
{
	uint8_t checksum1 = strtol(string + (strlen(string) - (2+2)), NULL, 16); //Check - is checksum always 2 chars? +2 for '\r\n' chars
	
	char *snew = strtok(string, "$*"); //This modifies the 'string'!
	uint8_t checksum2 = nmea_checksum((const char*) snew);
	if(checksum1 == checksum2)
		return true;
	else
		return false;
}

