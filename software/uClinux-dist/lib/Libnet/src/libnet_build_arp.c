/*
 *  $Id: libnet_build_arp.c,v 1.1.1.1 2000/05/25 00:28:49 route Exp $
 *
 *  libnet
 *  libnet_build_arp.c - ARP packet assembler
 *
 *  Copyright (c) 1998 - 2001 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#include "../include/libnet.h"

int
libnet_build_arp(u_short hrd, u_short pro, u_char hln, u_char pln, u_short op,
            u_char *sha, u_char *spa, u_char *tha, u_char *tpa,
            const u_char *payload, int payload_s, u_char *buf)
{
    struct libnet_arp_hdr arp_hdr;

    if (!buf)
    {
        return (-1);
    }

    arp_hdr.ar_hrd = htons(hrd);       /* hardware address type */
    arp_hdr.ar_pro = htons(pro);       /* protocol address type */
    arp_hdr.ar_hln = hln;              /* hardware address length */
    arp_hdr.ar_pln = pln;              /* protocol address length */
    arp_hdr.ar_op  = htons(op);        /* opcode command */
    memcpy(arp_hdr.ar_sha, sha, hln);  /* sender hardware address */
    memcpy(arp_hdr.ar_spa, spa, pln);  /* sender protocol (IP) address */
    memcpy(arp_hdr.ar_tha, tha, hln);  /* target hardware address */
    memcpy(arp_hdr.ar_tpa, tpa, pln);  /* target protocol (IP) address */

    if (payload && payload_s) 
    {
        /*
         *  Unchecked runtime error for buf + LIBNET_ARP_H payload to be
         *  greater than the allocated heap memory.
         */
        memcpy(buf + LIBNET_ARP_H, payload, payload_s);
    }
    memcpy(buf, &arp_hdr, sizeof(arp_hdr));
    return (1);
}


/* EOF */
