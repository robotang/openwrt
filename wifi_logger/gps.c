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

#define ASCIIHEX_TO_UINT(x)         ( ((x) > '9') ? (x-55):(x-48) )

static bool nmea_process(gps_t *gps, char *string);
static uint8_t nmea_checksum(const char *string);
static bool nmea_validate(char *string);

static FILE *input;
static bool readlog;
static int id;

int gps_init(const char *dev, int baud)
{
    int result = 0;
    readlog = (baud == GPS_LOGFILE) ? true : false;
    
    if(readlog)
    {       
        input = fopen(dev, "r");
        if(input == NULL)
        {
	        printf("loading %s logfile failed\n", dev); 
	        result = -1;
        }
    }
    else
    {
        result = serial_init(dev, baud, false);
    }
    
    id = 0;
    return result;
}

void gps_close(void)
{
    if(readlog)
    {
        if(input)
        {
            fclose(input);
            input = NULL;
        }
    }
    else
        serial_close();        
}

int gps_update(gps_t *gps)
{
    char data[200];
    int result;
    
    if(readlog)
    {
        char *ret;
        ret = fgets(data, sizeof(data) - 1, input);
        result = (ret) ? 0 : -1;
	}
	else
	{        
        result = serial_readline(data);
    }
    
    if(result >= 0)
    {
        nmea_process(gps, data);  
    }
    
    return result;
}

/*
 * Private functions
 */

static bool nmea_process(gps_t *gps, char *string)
{
    bool valid;
    
    valid = nmea_validate(string);
    gps->valid = false;
    
    /* Check for GPRMC message */
    if(valid && (strncmp(string, "$GPRMC", 6) == 0))
    {
        strcpy(gps->str, string);
        
        char a;
        sscanf(string, "$GPRMC,%f,%c,%f,%c,%f,%c", &gps->time, &a, &gps->latitude, &gps->latitude_dir, &gps->longitude, &gps->longitude_dir);
        gps->valid = (a == 'A') ? true : false;
        gps->id = id++;
    }
    
    return valid;
}

static uint8_t nmea_checksum(const char *string)
{
	uint8_t checksum = 0, tmp;
	
	while(*string)
	{
		if(*string == '*')
		    break;
		else if(*string == '$')
		    tmp = *string++;
		else
		    checksum ^= *string++;
	}	

	return checksum;
} 

static bool nmea_validate(char *string)
{
	uint8_t checksum1;
	int i;
	char *pch;
	
	pch = strchr(string, '*');
    i = pch - string + 1;
	
	checksum1 = (ASCIIHEX_TO_UINT(string[i]) << 4) + ASCIIHEX_TO_UINT(string[i+1]);

	uint8_t checksum2 = nmea_checksum(string);
	if(checksum1 == checksum2)
		return true;
	else
		return false;
}

