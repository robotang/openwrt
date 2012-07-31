/*
 *  Records wifi signal strength with GPS position stamps
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
#include <time.h>

#include "gps.h"
#include "wifi_scan.h"
#include "ini.h"

#define MSLEEP(x)            usleep((x)*1000)

typedef struct
{
    const char *gps_dev;
    int gps_baud;
    const char *wifi_interface;
    const char *target_essid;
    int logging_delta;
    int logging_duration;
    const char *output;
    bool print_output;
} configuration;

static int handler(void *user, const char *section, const char *name, const char *value)
{
    configuration *pconfig = (configuration *)user;

    #define MATCH(s, n) strcasecmp(section, s) == 0 && strcasecmp(name, n) == 0
    if(MATCH("gps", "port")) 
        pconfig->gps_dev = strdup(value);
    else if(MATCH("gps", "baud"))
        pconfig->gps_baud = (atoi(value) > 0) ? atoi(value) : 0;
    else if(MATCH("wifi", "interface"))
        pconfig->wifi_interface = strdup(value);
    else if(MATCH("wifi", "target"))
        pconfig->target_essid = strdup(value);
    else if(MATCH("log", "delta"))
        pconfig->logging_delta = (atoi(value) > 0) ? atoi(value) : 0;
    else if(MATCH("log", "duration"))
        pconfig->logging_duration = (atoi(value) > 0) ? atoi(value) : 0;
    else if(MATCH("log", "output"))
        pconfig->output = strdup(value);
    else if(MATCH("debug", "printoutput"))
        pconfig->print_output = (atoi(value) > 0) ? true : false;    
    else
        return 0;  /* unknown section/name, error */

    return 1;
}

void write_log(FILE *output, gps_t gps, wifi_scan_t scan, bool display)
{
    /* Format: gps time, scan quality, signal level, noise level, gps latitude, gps longitude */
    fprintf(output, "%0.3f %d %d %d %0.4f %0.4f\n", gps.time, scan.quality, scan.signal, scan.noise, gps.latitude, gps.longitude);
    if(display)
        printf("%0.3f %d %d %d %0.4f %0.4f\n", gps.time, scan.quality, scan.signal, scan.noise, gps.latitude, gps.longitude);
}

int main(int argc, char* argv[])
{
    int result = 0;
    gps_t gps;
    wifi_scan_t scan;
    configuration config;
    FILE *output;
    bool log = true;
    time_t start;

    /* Parse configuration file */
    if(ini_parse("wifi_logger.ini", handler, &config) < 0) 
    {
        printf("Failed to load 'wifi_logger.ini'\n");
        return -1;
    }
    
    /* Configure GPS */
    result = gps_init(config.gps_dev, config.gps_baud);
    if(result < 0)
        goto exit;
        
    /* Configure wifi */
    result = wifi_scan_init(config.wifi_interface, config.target_essid);
    if(result < 0)
        goto exit;
    
    /* Setup output log file */
    output = fopen(config.output, "w");
    if(output == NULL)
    {
        printf("Failed to create %s logfile\n", config.output); 
        goto exit;
    }
    
    start = time(NULL);
    /* Log wifi statistics with GPS position stamps */
    while(log)
    {
        int gps_result, wifi_result;
        
        gps_result = gps_update(&gps);
        if((gps_result >= 0) && gps.valid)
        {
            wifi_result = wifi_scan(&scan);       
        
            if(wifi_result >= 0)
                write_log(output, gps, scan, config.print_output);
        }
        
        if((int)(time(NULL) - start) >= config.logging_duration)
            log = false;

        MSLEEP(config.logging_delta);
    }
   
exit:
    printf("wifi logger exitting\n");
    gps_close();
    wifi_scan_close();
    if(output)
        fclose(output);
    
    return result;
}

