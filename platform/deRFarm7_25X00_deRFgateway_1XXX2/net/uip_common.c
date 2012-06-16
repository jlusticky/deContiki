/*
 * Copyright (c) 2012, Swedish Institute of Computer Science and other contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
/**
 * \file
 *         This file holds variables and functions common to both the IPv4 and the
 *         IPv6 stacks.
 *         Its content is derived from uip6.c written by Adam Dunkels <adam@sics.se>,
 *         Julien Abeille <jabeille@cisco.com> and Mathilde Durvy.
 * \author
 *         Luis Maqueda Ara <luis@sen.se> (ND proxy gw related modifications)
 */

#define DEBUG_PRINTF(...) /*printf(__VA_ARGS__)*/

#include "contiki.h"
#include "contiki-net.h"

#include "./net/uipv4/uipv4.h"  // TODO @@@jwg: added due to UIPV4_IPH_LEN

#define UIP_IP_BUF		((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIPV4_IP_BUF	((struct uipv4_ip_hdr *)&uip_buf[UIP_LLH_LEN])

/* Temporary variables. */
u8_t uip_acc32[4];

/*
 *
 */
void
uip_add32(u8_t *op32, u16_t op16)
{
  uip_acc32[3] = op32[3] + (op16 & 0xff);
  uip_acc32[2] = op32[2] + (op16 >> 8);
  uip_acc32[1] = op32[1];
  uip_acc32[0] = op32[0];
  
  if(uip_acc32[2] < (op16 >> 8)) {
    ++uip_acc32[1];
    if(uip_acc32[1] == 0) {
      ++uip_acc32[0];
    }
  }
  
  if(uip_acc32[3] < (op16 & 0xff)) {
    ++uip_acc32[2];
    if(uip_acc32[2] == 0) {
      ++uip_acc32[1];
      if(uip_acc32[1] == 0) {
				++uip_acc32[0];
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
static u16_t
chksum(u16_t sum, const u8_t *data, u16_t len)
{
  u16_t t;
  const u8_t *dataptr;
  const u8_t *last_byte;

  dataptr = data;
  last_byte = data + len - 1;
  
  while(dataptr < last_byte) {   /* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;      /* carry */
    }
    dataptr += 2;
  }
  
  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;      /* carry */
    }
  }
//  printf("uip_common.chksum: sum 0x%04x%s", sum, "\n");
  /* Return sum in host byte order. */
  return sum;
}

/*---------------------------------------------------------------------------*/
u16_t
uip_chksum(u16_t *data, u16_t len)
{
  return uip_htons(chksum(0, (u8_t *)data, len));
}

/*---------------------------------------------------------------------------*/

u16_t
uip_ipchksum(void)
{
  u16_t sum;

  sum = chksum(0, &uip_buf[UIP_LLH_LEN], UIP_IPH_LEN);
//  PRINTF("uip_ipchksum: sum 0x%04x\n", sum);
  printf("uip_ipchksum: sum 0x%04x%s", sum,"\n");
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/

u16_t
uipv4_ipchksum(void)
{
  u16_t sum;

  sum = chksum(0, &uip_buf[UIP_LLH_LEN], UIPV4_IPH_LEN);
  DEBUG_PRINTF("uipv4_ipchksum: sum 0x%04x\n", sum);
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/

static u16_t
upper_layer_chksum(u8_t ip_version, u8_t proto)
{
	volatile u16_t upper_layer_len;
  u16_t sum;
  
  if (ip_version == 4) {
	  upper_layer_len = (((u16_t)(UIPV4_IP_BUF->len[0]) << 8) + UIPV4_IP_BUF->len[1]) - UIPV4_IPH_LEN;
  
	  /* First sum pseudoheader. */
	  
	  /* IP protocol and length fields. This addition cannot carry. */
	  sum = upper_layer_len + proto;
	  /* Sum IP source and destination addresses. */
	  sum = chksum(sum, (u8_t *)&UIPV4_IP_BUF->srcipaddr, 2 * sizeof(uip_ip4addr_t));
	
	  /* Sum TCP header and data. */
	  sum = chksum(sum, &uip_buf[UIPV4_IPH_LEN + UIP_LLH_LEN],
		       upper_layer_len);  
  } else {
	  upper_layer_len = (((u16_t)(UIP_IP_BUF->len[0]) << 8) + UIP_IP_BUF->len[1] - uip_ext_len) ;
	  
	  /* First sum pseudoheader. */
	  /* IP protocol and length fields. This addition cannot carry. */
	  sum = upper_layer_len + proto;
	  /* Sum IP source and destination addresses. */
	  sum = chksum(sum, (u8_t *)&UIP_IP_BUF->srcipaddr, 2 * sizeof(uip_ipaddr_t));
	
	  /* Sum TCP header and data. */
	  sum = chksum(sum, &uip_buf[UIP_IPH_LEN + UIP_LLH_LEN + uip_ext_len],
	               upper_layer_len);
  
  }
// printf("upper_layer_chksum: sum 0x%04x%s", uip_htons(sum), "\n");
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/

u16_t
uip_icmp6chksum(void)
{
  return upper_layer_chksum(6, UIP_PROTO_ICMP6);
}
/*---------------------------------------------------------------------------*/

#if UIP_TCP
u16_t
uip_tcpchksum(void)
{
  return upper_layer_chksum(6, UIP_PROTO_TCP);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/

#if UIP_UDP && UIP_UDP_CHECKSUMS
u16_t
uip_udpchksum(void)
{
  return upper_layer_chksum(6, UIP_PROTO_UDP);
}
#endif /* UIP_UDP && UIP_UDP_CHECKSUMS */
/*---------------------------------------------------------------------------*/

#if UIPV4_TCP
u16_t
uipv4_tcpchksum(void)
{
  return upper_layer_chksum(4, UIP_PROTO_TCP);
}
#endif /* UIPV4_TCP */
/*---------------------------------------------------------------------------*/

#if UIPV4_UDP && UIPV4_UDP_CHECKSUMS
u16_t
uipv4_udpchksum(void)
{
  return upper_layer_chksum(4, UIP_PROTO_UDP);
}
#endif /* UIPV4_UDP && UIPV4_UDP_CHECKSUMS */
