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

#include "wifi_scan.h"

#include <iwlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct iwscan_state
{
    int ap_num;
    int val_index;
} iwscan_state;

static int skfd;
static const char *interface, *target;

int wifi_scan_init(const char *ifname, const char *target_essid)
{
    skfd = iw_sockets_open();
    if(skfd < 0)
    {
        printf("Error opening iw socket\n");
        return -1;
    }
    
    interface = ifname;
    target = target_essid;
    
    return 0;
}

void wifi_scan_close(void)
{
    if(skfd != -1)
    {
        iw_sockets_close(skfd);
        skfd = -1;
    }
}

/* Hacked from print_scanning_info function from iwlist.c */
int wifi_scan(wifi_scan_t *scan)
{
    struct iwreq            wrq;
    struct iw_scan_req      scanopt;                      /* Options for 'set' */
    unsigned char *         buffer = NULL;                /* Results */
    int                     buflen = IW_SCAN_MAX_DATA;    /* Min for compat WE<17 */
    struct iw_range         range;
    int                     has_range;
    struct timeval          tv;                           /* Select timeout */
    int                     timeout = 5000000;            /* 5s */
    int                     result = 0;

    /* Get range stuff */
    has_range = (iw_get_range_info(skfd, interface, &range) >= 0);
    /* Check if the interface could support scanning. */
    if((!has_range) || (range.we_version_compiled < 14))
    {
        printf("%-8.16s  Interface doesn't support scanning.\n\n", interface);
        return -1;
    }

    /* Init timeout value -> 250ms between set and first get */
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    /* Clean up set args */
    memset(&scanopt, 0, sizeof(scanopt));

    /* Initiate Scanning */
    if(iw_set_ext(skfd, interface, SIOCSIWSCAN, &wrq) < 0)
    {
        if((errno != EPERM))
        {
            printf("%-8.16s  Interface doesn't support scanning : %s\n\n", interface, strerror(errno));
            return -1;
        }
        tv.tv_usec = 0;
    }
    timeout -= tv.tv_usec;

    /* Forever */
    while(1)
    {
        fd_set            rfds;           /* File descriptors for select */
        int               last_fd;        /* Last fd */
        int               ret;

        /* Guess what ? We must re-generate rfds each time */
        FD_ZERO(&rfds);
        last_fd = -1;

        /* Wait until something happens */
        ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);

        /* Check if there was an error */
        if(ret < 0)
        {
            if(errno == EAGAIN || errno == EINTR)
                continue;
            printf("Unhandled signal - exiting...\n");
            return -1;
        }

        /* Check if there was a timeout */
        if(ret == 0)
        {
            unsigned char *newbuf;

        realloc:
            /* (Re)allocate the buffer - realloc(NULL, len) == malloc(len) */
            newbuf = realloc(buffer, buflen);
            if(newbuf == NULL)
            {
                if(buffer)
                    free(buffer);
                printf("%s: Allocation failed\n", __FUNCTION__);
                return -1;
            }
            buffer = newbuf;

            /* Try to read the results */
            wrq.u.data.pointer = buffer;
            wrq.u.data.flags = 0;
            wrq.u.data.length = buflen;
            if(iw_get_ext(skfd, interface, SIOCGIWSCAN, &wrq) < 0)
            {
                /* Check if buffer was too small (WE-17 only) */
                if((errno == E2BIG) && (range.we_version_compiled > 16))
                {
                    /* Some driver may return very large scan results, either
                    * because there are many cells, or because they have many
                    * large elements in cells (like IWEVCUSTOM). Most will
                    * only need the regular sized buffer. We now use a dynamic
                    * allocation of the buffer to satisfy everybody. Of course,
                    * as we don't know in advance the size of the array, we try
                    * various increasing sizes. Jean II */

                    /* Check if the driver gave us any hints. */
                    if(wrq.u.data.length > buflen)
                        buflen = wrq.u.data.length;
                    else
                        buflen *= 2;

                    /* Try again */
                    goto realloc;
                }

                /* Check if results not available yet */
                if(errno == EAGAIN)
                {
                    /* Restart timer for only 100ms*/
                    tv.tv_sec = 0;
                    tv.tv_usec = 100000;
                    timeout -= tv.tv_usec;
                    if(timeout > 0)
                        continue;   /* Try again later */
                }

                /* Bad error */
                free(buffer);
                printf("%-8.16s  Failed to read scan data : %s\n\n", interface, strerror(errno));
                return -2;
            }
            else
                /* We have the results, go to process them */
                break;
        }
    }

    if(wrq.u.data.length)
    {
        struct iw_event         iwe;
        struct stream_descr     stream;
        int                     ret;
        bool                    found_target = false;    

        iw_init_event_stream(&stream, (char *) buffer, wrq.u.data.length);
        do
        {
            /* Extract an event and process it */
            ret = iw_extract_event_stream(&stream, &iwe, range.we_version_compiled);
            if(ret > 0)
            {
                switch(iwe.cmd)
                {
                    case SIOCGIWESSID:
                    {
                        char essid[IW_ESSID_MAX_SIZE+1];
                        memset(essid, '\0', sizeof(essid));
                        if((iwe.u.essid.pointer) && (iwe.u.essid.length))
                            memcpy(essid, iwe.u.essid.pointer, iwe.u.essid.length);
                        if(strcmp(target, essid) == 0)
                            found_target = true;
                    } break;
                    
                    case IWEVQUAL:
                    {
                        if(found_target)
                        {   
                            /* If the statistics are in dBm */
                            if(has_range && (iwe.u.qual.level != 0))
                            {
                                /* Statistics are in dBm (absolute power measurement) */
                                if(iwe.u.qual.level > range.max_qual.level)
                                {
                                    scan->quality = (100*iwe.u.qual.qual) / range.max_qual.qual;
                                    scan->signal = iwe.u.qual.level - 0x100;
                                    scan->noise = iwe.u.qual.noise - 0x100;
                                }
                                /* Statistics are relative values (0 -> max) */
                                else
                                {
                                    scan->quality = (100*iwe.u.qual.qual) / range.max_qual.qual;
                                    scan->signal = (100*iwe.u.qual.level) / range.max_qual.level;
                                    scan->noise = (100*iwe.u.qual.noise) / range.max_qual.noise;                                    
                                }
                            }
                            /* We can't read the range, so we don't know... */
                            else
                            {
                                scan->quality = iwe.u.qual.qual;
                                scan->signal = iwe.u.qual.level;
                                scan->noise = iwe.u.qual.noise;
                            }
                            goto exit;
                        }
                    } break;
                    
                    default:
                    {
                        ; //Not interested in other fields
                    } break;                    
                }
            }
        } while(ret > 0);
    }
    else
    {
        printf("%-8.16s  No scan results\n\n", interface);
        result = -1;
    }
    
exit:   
    free(buffer);
    
    return result;
}

