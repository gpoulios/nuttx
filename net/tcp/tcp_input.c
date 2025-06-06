/****************************************************************************
 * net/tcp/tcp_input.c
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 *   Copyright (C) 2007-2014, 2017-2019, 2020 Gregory Nutt. All rights
 *     reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for NuttX from logic in uIP which also has a BSD-like license:
 *
 *   Original author Adam Dunkels <adam@dunkels.com>
 *   Copyright () 2001-2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_NET) && defined(CONFIG_NET_TCP)

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/net/netconfig.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/netstats.h>
#include <nuttx/net/ip.h>
#include <nuttx/net/tcp.h>

#include "devif/devif.h"
#include "utils/utils.h"
#include "tcp/tcp.h"

#define IPDATA(hl) (*(FAR uint8_t *)IPBUF(hl))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_trim_head
 *
 * Description:
 *   Trim the head of the TCP segment.
 *
 * Input Parameters:
 *   dev     - The device driver structure containing the received TCP
 *             packet.
 *   tcp     - The TCP header.
 *   trimlen - The length to trim in bytes.
 *
 * Returned Value:
 *   True if nothing was left.
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

static bool tcp_trim_head(FAR struct net_driver_s *dev,
                          FAR struct tcp_hdr_s *tcp,
                          uint32_t trimlen)
{
  uint32_t seq = tcp_getsequence(tcp->seqno);
  uint16_t urg_ptr = (tcp->urgp[0] << 8) | tcp->urgp[1];
  uint32_t urg_trimlen = 0;
  uint8_t th_flags = tcp->flags;

  DEBUGASSERT(trimlen > 0);
  ninfo("Dropping %" PRIu32 " bytes: "
        "seq=%" PRIu32 ", "
        "tcp flags=%" PRIx8 ", "
        "d_len=%" PRIu16 ", "
        "urg_ptr=%" PRIu16 "\n",
        trimlen,
        seq,
        th_flags,
        dev->d_len,
        urg_ptr);

  if ((th_flags & TCP_SYN) != 0)
    {
      ninfo("Dropping SYN\n");
      seq = TCP_SEQ_ADD(seq, 1);
      urg_trimlen++;
      trimlen--;
      th_flags &= ~TCP_SYN;
    }

  if (trimlen > 0)
    {
      uint32_t len = trimlen;

      if (len > dev->d_len)
        {
          len = dev->d_len;
        }

      ninfo("Dropping %" PRIu32 " bytes app data\n", len);
      seq = TCP_SEQ_ADD(seq, len);
      urg_trimlen += len;
      dev->d_appdata += len;
      dev->d_len -= len;
      trimlen -= len;
    }

  if (trimlen > 0)
    {
      if ((th_flags & TCP_FIN) != 0)
        {
          ninfo("Dropping FIN\n");
          seq = TCP_SEQ_ADD(seq, 1);
          urg_trimlen++;
          trimlen--;
          th_flags &= ~TCP_FIN;
        }
    }

  /* Update the header */

  if ((th_flags & TCP_URG) != 0)
    {
      /* Adjust URG pointer */

      if (urg_trimlen >= urg_ptr)
        {
          th_flags &= ~TCP_URG;
          urg_ptr = 0;
        }
      else
        {
          urg_ptr -= urg_trimlen;
        }

      ninfo("Adjusting URG pointer by %" PRIu32 ", "
            "new urg_ptr=%" PRIu16 "\n",
            urg_trimlen, urg_ptr);

      tcp->urgp[0] = (uint8_t)(urg_ptr >> 8);
      tcp->urgp[1] = (uint8_t)urg_ptr;
    }

  tcp->flags = th_flags;
  tcp_setsequence(tcp->seqno, seq);

  if ((th_flags & (TCP_SYN | TCP_FIN)) == 0 && dev->d_len == 0)
    {
      ninfo("Dropped the entire segment\n");
      return true;
    }

  DEBUGASSERT(trimlen == 0);
  ninfo("Dropped the segment partially\n");
  return false;
}

static void tcp_snd_wnd_init(FAR struct tcp_conn_s *conn,
                             FAR struct tcp_hdr_s *tcp)
{
  /* Just ensure that the next tcp_update_snd_wnd will be accepted. */

  DEBUGASSERT((tcp->flags & TCP_ACK) != 0);
  conn->snd_wl1 = TCP_SEQ_SUB(tcp_getsequence(tcp->seqno), 1);
  conn->snd_wl2 = tcp_getsequence(tcp->ackno);
  conn->snd_wnd = 0;
  ninfo("snd_wnd init: wl1 %" PRIu32 "\n", conn->snd_wl1);
}

static bool tcp_snd_wnd_update(FAR struct tcp_conn_s *conn,
                               FAR struct tcp_hdr_s *tcp)
{
  uint32_t ackseq = tcp_getsequence(tcp->ackno);
  uint32_t seq = tcp_getsequence(tcp->seqno);
  uint16_t unscaled_wnd = ((uint16_t)tcp->wnd[0] << 8) + tcp->wnd[1];
#ifdef CONFIG_NET_TCP_WINDOW_SCALE
  uint32_t wnd = (uint32_t)unscaled_wnd << conn->snd_scale;
#else
  uint16_t wnd = unscaled_wnd;
#endif
  uint32_t wl2 = conn->snd_wl2;

  DEBUGASSERT((tcp->flags & TCP_ACK) != 0);

  if (TCP_SEQ_LT(wl2, ackseq))
    {
      uint32_t nacked = TCP_SEQ_SUB(ackseq, wl2);

      ninfo("snd_wnd acked: "
            "wl2 %" PRIu32 " -> %" PRIu32 " subtracting wnd %" PRIu32
            " by %" PRIu32 "\n",
            wl2,
            ackseq,
            (uint32_t)conn->snd_wnd,
            nacked);

      if (nacked > conn->snd_wnd)
        {
          conn->snd_wnd = 0;
        }
      else
        {
          conn->snd_wnd -= nacked;
        }

      conn->snd_wl2 = ackseq;
    }

  if (TCP_SEQ_LT(conn->snd_wl1, seq) ||
      (conn->snd_wl1 == seq && TCP_SEQ_LT(wl2, ackseq)) ||
      (wl2 == ackseq && conn->snd_wnd < wnd))
    {
      ninfo("snd_wnd update: "
            "wl1 %" PRIu32 " wl2 %" PRIu32 " wnd %" PRIu32 " -> "
            "wl1 %" PRIu32 " wl2 %" PRIu32 " wnd %" PRIu32 "\n",
            conn->snd_wl1,
            wl2,
            (uint32_t)conn->snd_wnd,
            seq,
            ackseq,
            (uint32_t)wnd);

      conn->snd_wl1 = seq;
      conn->snd_wl2 = ackseq;
      if (conn->snd_wnd != wnd)
        {
          conn->snd_wnd = wnd;
          return true;
        }
    }

  return false;
}

#ifdef CONFIG_NET_TCP_OUT_OF_ORDER

/****************************************************************************
 * Name: tcp_rebuild_ofosegs
 *
 * Description:
 *   Re-build out-of-order pool from incoming segment
 *
 * Input Parameters:
 *   conn   - The TCP connection of interest
 *   ofoseg - Pointer to incoming out-of-order segment
 *   start  - Index of start position of segment pool
 *
 * Returned Value:
 *   True if incoming data has been consumed
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

static bool tcp_rebuild_ofosegs(FAR struct tcp_conn_s *conn,
                                FAR struct tcp_ofoseg_s *ofoseg,
                                int start)
{
  struct tcp_ofoseg_s *seg;
  int i;

  for (i = start; i < conn->nofosegs && ofoseg->data != NULL; i++)
    {
      seg = &conn->ofosegs[i];

      /* ofoseg    |~~~
       * segpool |---|
       */

      if (TCP_SEQ_GTE(ofoseg->left, seg->left))
        {
          /* ofoseg        |---|
           * segpool |---|
           */

          if (TCP_SEQ_GT(ofoseg->left, seg->right))
            {
              continue;
            }

          /* ofoseg      |---|
           * segpool |---|
           */

          else if (ofoseg->left == seg->right)
            {
              net_iob_concat(&seg->data, &ofoseg->data);
              seg->right = ofoseg->right;
            }

          /* ofoseg   |--|
           * segpool |---|
           */

          else if (TCP_SEQ_LTE(ofoseg->right, seg->right))
            {
              iob_free_chain(ofoseg->data);
              ofoseg->data = NULL;
            }

          /* ofoseg    |---|
           * segpool |---|
           */

          else if (TCP_SEQ_GT(ofoseg->right, seg->right))
            {
              ofoseg->data =
                iob_trimhead(ofoseg->data,
                             TCP_SEQ_SUB(seg->right, ofoseg->left));
              net_iob_concat(&seg->data, &ofoseg->data);
              seg->right = ofoseg->right;
            }
        }

      /* ofoseg  |~~~
       * segpool   |---|
       */

      else
        {
          /* ofoseg  |---|
           * segpool     |---|
           */

          if (ofoseg->right == seg->left)
            {
              net_iob_concat(&ofoseg->data, &seg->data);
              seg->data = ofoseg->data;
              seg->left = ofoseg->left;
              ofoseg->data = NULL;
            }

          /* ofoseg  |---|
           * segpool       |---|
           */

          else if (TCP_SEQ_LT(ofoseg->right, seg->left))
            {
              continue;
            }

          /* ofoseg  |---|~|
           * segpool  |--|
           */

          else if (TCP_SEQ_GTE(ofoseg->right, seg->right))
            {
              iob_free_chain(seg->data);
              *seg = *ofoseg;
              ofoseg->data = NULL;
            }

          /* ofoseg  |---|
           * segpool   |---|
           */

          else if (TCP_SEQ_GT(ofoseg->right, seg->left))
            {
              ofoseg->data =
                iob_trimtail(ofoseg->data,
                             ofoseg->right - seg->left);
              net_iob_concat(&ofoseg->data, &seg->data);
              seg->data = ofoseg->data;
              seg->left = ofoseg->left;
              ofoseg->data = NULL;
            }
        }
    }

  return (ofoseg->data == NULL);
}

/****************************************************************************
 * Name: tcp_input_ofosegs
 *
 * Description:
 *   Handle incoming TCP data to out-of-order pool
 *
 * Input Parameters:
 *   dev    - The device driver structure containing the received TCP packet.
 *   conn   - The TCP connection of interest
 *   iplen  - Length of the IP header (IPv4_HDRLEN or IPv6_HDRLEN).
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

static void tcp_input_ofosegs(FAR struct net_driver_s *dev,
                              FAR struct tcp_conn_s *conn,
                              unsigned int iplen)
{
  struct tcp_ofoseg_s ofoseg;
  bool rebuild;
  int i = 0;
  int len;

  ofoseg.left =
    tcp_getsequence(((FAR struct tcp_hdr_s *)IPBUF(iplen))->seqno);

  /* Calculate the pending size of out-of-order cache, if the input edge can
   * not fill the adjacent segments, drop it
   */

  if (tcp_ofoseg_bufsize(conn) > CONFIG_NET_TCP_OUT_OF_ORDER_BUFSIZE &&
      ofoseg.left >= conn->ofosegs[0].left)
    {
      return;
    }

  /* Get left/right edge from incoming data */

  len = (dev->d_appdata - dev->d_iob->io_data) - dev->d_iob->io_offset;
  ofoseg.right = TCP_SEQ_ADD(ofoseg.left, dev->d_iob->io_pktlen - len);

  ninfo("TCP OFOSEG out-of-order "
        "[%" PRIu32 " : %" PRIu32 " : %" PRIu32 "]\n",
        ofoseg.left, ofoseg.right, TCP_SEQ_SUB(ofoseg.right, ofoseg.left));

  /* Trim l3/l4 header to reserve appdata */

  dev->d_iob = iob_trimhead(dev->d_iob, len);
  if (dev->d_iob == NULL || dev->d_iob->io_pktlen == 0)
    {
      /* No available data, prepare device iob */

      goto prepare;
    }

  ofoseg.data = dev->d_iob;

  /* Build out-of-order pool */

  rebuild = tcp_rebuild_ofosegs(conn, &ofoseg, 0);

  /* Incoming segment out of order from existing pool, add to new segment */

  if (!rebuild && conn->nofosegs != TCP_SACK_RANGES_MAX)
    {
      conn->ofosegs[conn->nofosegs] = ofoseg;
      conn->nofosegs++;
      rebuild = true;
    }

  /* Try Re-order ofosegs */

  if (rebuild &&
      tcp_reorder_ofosegs(conn->nofosegs, (FAR void *)conn->ofosegs))
    {
      /* Re-build out-of-order pool after re-order */

      while (i < conn->nofosegs - 1)
        {
          if (tcp_rebuild_ofosegs(conn, &conn->ofosegs[i], i + 1))
            {
              for (; i < conn->nofosegs - 1; i++)
                {
                  conn->ofosegs[i] = conn->ofosegs[i + 1];
                }

              conn->nofosegs--;

              i = 0;
            }
          else
            {
              i++;
            }
        }
    }

  for (i = 0; i < conn->nofosegs; i++)
    {
      ninfo("TCP OFOSEG [%d][%" PRIu32 " : %" PRIu32 " : %" PRIu32 "]\n", i,
            conn->ofosegs[i].left, conn->ofosegs[i].right,
            TCP_SEQ_SUB(conn->ofosegs[i].right, conn->ofosegs[i].left));
    }

  /* Incoming data has been consumed, re-prepare device buffer to send
   * response.
   */

  if (rebuild)
    {
      netdev_iob_clear(dev);
    }

prepare:
  netdev_iob_prepare(dev, false, 0);
}
#endif /* CONFIG_NET_TCP_OUT_OF_ORDER */

/****************************************************************************
 * Name: tcp_parse_option
 *
 * Description:
 *   Parse incoming TCP options
 *
 * Input Parameters:
 *   dev    - The device driver structure containing the received TCP packet.
 *   conn   - The TCP connection of interest
 *   iplen  - Length of the IP header (IPv4_HDRLEN or IPv6_HDRLEN).
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

static void tcp_parse_option(FAR struct net_driver_s *dev,
                             FAR struct tcp_conn_s *conn,
                             unsigned int iplen)
{
  FAR struct tcp_hdr_s *tcp;
  unsigned int tcpiplen;
  uint16_t tmp16;
  uint8_t  opt;
  int i;

  tcp = IPBUF(iplen);

  if ((tcp->tcpoffset & 0xf0) <= 0x50)
    {
      return;
    }

  tcpiplen = iplen + TCP_HDRLEN;

  for (i = 0; i < ((tcp->tcpoffset >> 4) - 5) << 2 ; )
    {
      opt = IPDATA(tcpiplen + i);
      if (opt == TCP_OPT_END)
        {
          /* End of options. */

          break;
        }
      else if (opt == TCP_OPT_NOOP)
        {
          /* NOP option. */

          ++i;
          continue;
        }
      else if (opt == TCP_OPT_MSS &&
               IPDATA(tcpiplen + 1 + i) == TCP_OPT_MSS_LEN)
        {
          uint16_t tcp_mss = TCP_MSS(dev, iplen);

          /* An MSS option with the right option length. */

          tmp16 = ((uint16_t)IPDATA(tcpiplen + 2 + i) << 8) |
                   (uint16_t)IPDATA(tcpiplen + 3 + i);
#ifdef CONFIG_NET_TCPPROTO_OPTIONS
          if (conn->user_mss > 0 && conn->user_mss < tcp_mss)
            {
              tcp_mss = conn->user_mss;
            }
#endif

          conn->mss = tmp16 > tcp_mss ? tcp_mss : tmp16;
        }
#ifdef CONFIG_NET_TCP_WINDOW_SCALE
      else if (opt == TCP_OPT_WS &&
               IPDATA(tcpiplen + 1 + i) == TCP_OPT_WS_LEN)
        {
          conn->snd_scale = IPDATA(tcpiplen + 2 + i);
          conn->rcv_scale = CONFIG_NET_TCP_WINDOW_SCALE_FACTOR;
          conn->flags    |= TCP_WSCALE;
        }
#endif
#ifdef CONFIG_NET_TCP_SELECTIVE_ACK
      else if (opt == TCP_OPT_SACK_PERM &&
               IPDATA(tcpiplen + 1 + i) ==
               TCP_OPT_SACK_PERM_LEN)
        {
          conn->flags    |= TCP_SACK;
        }
#endif
      else
        {
          /* All other options have a length field, so that we
           * easily can skip past them.
           */

          if (IPDATA(tcpiplen + 1 + i) == 0)
            {
              /* If the length field is zero, the options are
               * malformed and we don't process them further.
               */

              break;
            }
        }

      i += IPDATA(tcpiplen + 1 + i);
    }
}

/****************************************************************************
 * Name: tcp_clear_zero_probe
 *
 * Description:
 *   clear the TCP zero window probe
 *
 * Input Parameters:
 *   conn   - The TCP connection of interest
 *   tcp    - Header of TCP structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

static void tcp_clear_zero_probe(FAR struct tcp_conn_s *conn,
                                 FAR struct tcp_hdr_s *tcp)
{
  /* If the receive window is not 0,
   * the zero window probe timer needs to be cleared
   */

  if ((tcp->wnd[0] || tcp->wnd[1]) && conn->zero_probe &&
      (tcp->flags & TCP_ACK) != 0)
    {
      conn->zero_probe = false;
      conn->nrtx = 0;
      conn->timer = 0;
    }
}

/****************************************************************************
 * Name: tcp_input
 *
 * Description:
 *   Handle incoming TCP input
 *
 * Input Parameters:
 *   dev    - The device driver structure containing the received TCP packet.
 *   domain - IP domain (PF_INET or PF_INET6)
 *   iplen  - Length of the IP header (IPv4_HDRLEN or IPv6_HDRLEN).
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

static void tcp_input(FAR struct net_driver_s *dev, uint8_t domain,
                      unsigned int iplen)
{
  FAR struct tcp_conn_s *conn = NULL;
  FAR struct tcp_hdr_s *tcp;
  union ip_binding_u uaddr;
  unsigned int tcpiplen;
  uint16_t tmp16;
  uint16_t flags;
  uint16_t result;
  int      len;

#ifdef CONFIG_NET_STATISTICS
  /* Bump up the count of TCP packets received */

  g_netstats.tcp.recv++;
#endif

  /* Get a pointer to the TCP header.  The TCP header lies just after the
   * the link layer header and the IP header.
   */

  tcp = IPBUF(iplen);

  /* Get the size of the IP header and the TCP header.
   *
   * REVISIT:  TCP header is *not* a constant!  It can be larger if the
   * TCP header includes options.  The constant TCP_HDRLEN should be
   * replaced with the macro TCP_OPT_HDRLEN(n) which will calculate the
   * correct header length in all cases.
   */

  tcpiplen = iplen + TCP_HDRLEN;

#ifdef CONFIG_NET_TCP_CHECKSUMS
  /* Start of TCP input header processing code. */

  if (tcp_chksum(dev) != 0xffff)
    {
      /* Compute and check the TCP checksum. */

#ifdef CONFIG_NET_STATISTICS
      g_netstats.tcp.drop++;
      g_netstats.tcp.chkerr++;
#endif
      nwarn("WARNING: Bad TCP checksum\n");
      goto drop;
    }
#endif

  /* Demultiplex this segment. First check any active connections. */

  conn = tcp_active(dev, tcp);
  if (conn)
    {
      /* We found an active connection.. Check for the subsequent SYN
       * arriving in TCP_SYN_RCVD state after the SYNACK packet was
       * lost.  To avoid other issues,  reset any active connection
       * where a SYN arrives in a state != TCP_SYN_RCVD.
       */

      if ((conn->tcpstateflags & TCP_STATE_MASK) != TCP_SYN_RCVD &&
          (tcp->flags & TCP_CTL) == TCP_SYN)
        {
          nwarn("WARNING: SYN in TCP_SYN_RCVD\n");
          goto reset;
        }
      else
        {
          goto found;
        }
    }

  /* If we didn't find an active connection that expected the packet,
   * either (1) this packet is an old duplicate, or (2) this is a SYN packet
   * destined for a connection in LISTEN.  If the SYN flag isn't set,
   * it is an old packet and we send a RST.
   */

  if ((tcp->flags & TCP_CTL) == TCP_SYN)
    {
      /* This is a SYN packet for a connection.  Find the connection
       * listening on this port.
       */

      tmp16 = tcp->destport;
#ifdef CONFIG_NET_IPv6
#  ifdef CONFIG_NET_IPv4
      if (domain == PF_INET6)
#  endif
        {
          net_ipv6addr_copy(&uaddr.ipv6.laddr, IPv6BUF->destipaddr);
        }
#endif

#ifdef CONFIG_NET_IPv4
#  ifdef CONFIG_NET_IPv6
      if (domain == PF_INET)
#  endif
        {
          net_ipv4addr_copy(uaddr.ipv4.laddr,
                            net_ip4addr_conv32(IPv4BUF->destipaddr));
        }
#endif

#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
      if ((conn = tcp_findlistener(&uaddr, tmp16, domain)) != NULL)
#else
      if ((conn = tcp_findlistener(&uaddr, tmp16)) != NULL)
#endif
        {
          if (!tcp_backlogavailable(conn))
            {
              nerr("ERROR: no free containers for TCP BACKLOG!\n");
              goto drop;
            }

          /* We matched the incoming packet with a connection in LISTEN.
           * We now need to create a new connection and send a SYNACK in
           * response.
           */

          /* First allocate a new connection structure and see if there is
           * any user application to accept it.
           */

          conn = tcp_alloc_accept(dev, tcp, conn);
          if (conn)
            {
              /* The connection structure was successfully allocated and has
               * been initialized in the TCP_SYN_RECVD state.  The expected
               * sequence of events is then the rest of the 3-way handshake:
               *
               *  1. We just received a TCP SYN packet from a remote host.
               *  2. We will send the SYN-ACK response below (perhaps
               *     repeatedly in the event of a timeout)
               *  3. Then we expect to receive an ACK from the remote host
               *     indicated the TCP socket connection is ESTABLISHED.
               *
               * Possible failure:
               *
               *  1. The ACK is never received.  This will be handled by
               *     a timeout managed by tcp_timer().
               *  2. The listener "unlistens()".  This will be handled by
               *     the failure of tcp_accept_connection() when the ACK is
               *     received.
               */

              conn->crefs = 1;
            }

          if (!conn)
            {
              /* Either (1) all available connections are in use, or (2)
               * there is no application in place to accept the connection.
               * We drop packet and hope that the remote end will retransmit
               * the packet at a time when we have more spare connections
               * or someone waiting to accept the connection.
               */

#ifdef CONFIG_NET_STATISTICS
              g_netstats.tcp.syndrop++;
#endif
              nerr("ERROR: No free TCP connections\n");
              goto drop;
            }

          net_incr32(conn->rcvseq, 1); /* ack SYN */

          /* Parse the TCP MSS option, if present. */

          tcp_parse_option(dev, conn, iplen);

          /* Our response will be a SYNACK. */

          tcp_synack(dev, conn, TCP_ACK | TCP_SYN);
          return;
        }
    }

  nwarn("WARNING: SYN with no listener (or old packet) .. reset\n");

  /* This is (1) an old duplicate packet or (2) a SYN packet but with
   * no matching listener found.  Send RST packet in either case.
   */

reset:

  /* We do not send resets in response to resets. */

  if ((tcp->flags & TCP_RST) != 0)
    {
      goto drop;
    }

#ifdef CONFIG_NET_STATISTICS
  g_netstats.tcp.synrst++;
#endif
  tcp_reset(dev, conn);
  return;

found:
  flags = 0;

  /* We do a very naive form of TCP reset processing; we just accept
   * any RST and kill our connection. We should in fact check if the
   * sequence number of this reset is within our advertised window
   * before we accept the reset.
   */

  if ((tcp->flags & TCP_RST) != 0)
    {
      /* An RST received during the 3-way connection handshake requires
       * little more clean-up.
       */

      if ((conn->tcpstateflags & TCP_STATE_MASK) == TCP_SYN_RCVD)
        {
          conn->tcpstateflags = TCP_CLOSED;
          nwarn("WARNING: RESET in TCP_SYN_RCVD\n");

          /* We must free this TCP connection structure; this connection
           * will never be established.  There should only be one reference
           * on this connection when we allocated for the connection.
           */

          DEBUGASSERT(conn->crefs == 1);
          conn->crefs = 0;
          tcp_free(conn);
        }
      else
        {
          conn->tcpstateflags = TCP_CLOSED;
          nwarn("WARNING: RESET TCP state: TCP_CLOSED\n");

          /* Notify this connection of the reset event */

          tcp_callback(dev, conn, TCP_ABORT);
        }

      /* Drop the packet */

      goto drop;
    }

  /* Calculated the length of the data, if the application has sent
   * any data to us.
   */

  len = (tcp->tcpoffset >> 4) << 2;

  /* d_appdata should remove the tcp specific option field. */

  if ((tcp->tcpoffset & 0xf0) > 0x50)
    {
      if (dev->d_len >= len)
        {
          dev->d_appdata += len - TCP_HDRLEN;
        }
    }

  /* d_len will contain the length of the actual TCP data. This is
   * calculated by subtracting the length of the TCP header (in
   * len) and the length of the IP header.
   */

  dev->d_len -= (len + iplen);

#if defined(CONFIG_NET_STATISTICS) && \
    defined(CONFIG_NET_TCP_DEBUG_DROP_RECV)

#pragma message \
  "CONFIG_NET_TCP_DEBUG_DROP_RECV is selected, this is debug " \
  "feature to drop the tcp received packet on the floor, " \
  "please confirm the configuration again if you do not want " \
  "debug the TCP stack."

  /* Debug feature to drop the tcp received packet on the floor */

  if (dev->d_len > 0)
    {
      if ((g_netstats.tcp.recv %
          CONFIG_NET_TCP_DEBUG_DROP_RECV_PROBABILITY) == 0)
        {
          uint32_t seq = tcp_getsequence(tcp->seqno);

          g_netstats.tcp.drop++;

          ninfo("TCP DROP RCVPKT: "
                "[%d][%" PRIu32 " : %" PRIu32 " : %d]\n",
                g_netstats.tcp.drop, seq, TCP_SEQ_ADD(seq, dev->d_len),
                dev->d_len);

          dev->d_len = 0;
          return;
        }
    }
#endif

  /* Check if the incoming segment acknowledges any outstanding data. If so,
   * we update the sequence number, reset the length of the outstanding
   * data, calculate RTT estimations, and reset the retransmission timer.
   */

  if ((tcp->flags & TCP_ACK) != 0 && conn->tx_unacked > 0)
    {
      uint32_t unackseq;
      uint32_t ackseq;
      int timeout;

      /* The next sequence number is equal to the current sequence
       * number (sndseq) plus the size of the outstanding, unacknowledged
       * data (tx_unacked).
       */

#if defined(CONFIG_NET_TCP_WRITE_BUFFERS) && !defined(CONFIG_NET_SENDFILE)
      unackseq = conn->sndseq_max;
#elif defined(CONFIG_NET_TCP_WRITE_BUFFERS) && defined(CONFIG_NET_SENDFILE)
      if (!conn->sendfile)
        {
          unackseq = conn->sndseq_max;
        }
      else
        {
          unackseq = tcp_getsequence(conn->sndseq);
        }
#else
      unackseq = tcp_getsequence(conn->sndseq);
#endif

      /* Get the sequence number of that has just been acknowledged by this
       * incoming packet.
       */

      ackseq = tcp_getsequence(tcp->ackno);

      /* Check how many of the outstanding bytes have been acknowledged. For
       * most send operations, this should always be true.  However,
       * the send() API sends data ahead when it can without waiting for
       * the ACK.  In this case, the 'ackseq' could be less than then the
       * new sequence number.
       */

      if (TCP_SEQ_LTE(ackseq, unackseq))
        {
          /* Calculate the new number of outstanding, unacknowledged bytes */

          conn->tx_unacked = unackseq - ackseq;
        }
      else
        {
          /* What would it mean if ackseq > unackseq?  The peer has ACKed
           * more bytes than we think we have sent?  Someone has lost it.
           * Complain and reset the number of outstanding, unacknowledged
           * bytes
           */

          if ((conn->tcpstateflags & TCP_STATE_MASK) == TCP_ESTABLISHED)
            {
              nwarn("WARNING: ackseq > unackseq\n");
              nwarn("sndseq=%" PRIu32 " tx_unacked=%" PRIu32
                    " unackseq=%" PRIu32 " ackseq=%" PRIu32 "\n",
                    tcp_getsequence(conn->sndseq),
                    (uint32_t)conn->tx_unacked,
                    unackseq, ackseq);

              conn->tx_unacked = 0;
            }
        }

#ifdef CONFIG_NET_TCP_WRITE_BUFFERS
#ifdef CONFIG_NET_SENDFILE
      if (!conn->sendfile)
#endif
        {
          /* Update sequence number to the unacknowledge sequence number. If
           * there is still outstanding, unacknowledged data, then this will
           * be beyond ackseq.
           */

          uint32_t sndseq = tcp_getsequence(conn->sndseq);
          if (TCP_SEQ_LT(sndseq, ackseq))
            {
              ninfo("sndseq: %08" PRIx32 "->%08" PRIx32
                    " unackseq: %08" PRIx32 " new tx_unacked: %" PRIu32 "\n",
                    tcp_getsequence(conn->sndseq), ackseq, unackseq,
                    (uint32_t)conn->tx_unacked);
              tcp_setsequence(conn->sndseq, ackseq);
              conn->nrtx = 0;
            }
        }
#endif

      /* Do RTT estimation, unless we have done retransmissions. */

      if (conn->nrtx == 0)
        {
          signed char m;
          m = conn->rto - conn->timer;

          /* This is taken directly from VJs original code in his paper */

          m = m - (conn->sa >> 3);
          conn->sa += m;
          if (m < 0)
            {
              m = -m;
            }

          m = m - (conn->sv >> 2);
          conn->sv += m;
          conn->rto = (conn->sa >> 3) + conn->sv;
        }

      /* Set the acknowledged flag. */

      flags |= TCP_ACKDATA;

      /* Check if no packet need to retransmission, clear timer. */

      if (conn->tx_unacked == 0 && conn->tcpstateflags == TCP_ESTABLISHED)
        {
          timeout = 0;
        }
      else
        {
          timeout = conn->rto;
        }

      /* Reset the retransmission timer. */

      tcp_update_retrantimer(conn, timeout);
    }

  /* Check if the sequence number of the incoming packet is what we are
   * expecting next.  If not, we send out an ACK with the correct numbers
   * in, unless we are in the SYN_RCVD state and receive a SYN, in which
   * case we should retransmit our SYNACK (which is done further down).
   */

  if (!((((conn->tcpstateflags & TCP_STATE_MASK) == TCP_SYN_SENT) &&
        ((tcp->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))) ||
        (((conn->tcpstateflags & TCP_STATE_MASK) == TCP_SYN_RCVD) &&
        ((tcp->flags & TCP_CTL) == TCP_SYN))))
    {
      uint32_t seq;
      uint32_t rcvseq;

      seq = tcp_getsequence(tcp->seqno);
      rcvseq = tcp_getsequence(conn->rcvseq);

      /* According to RFC793, Section 3.4, Page 33.
       * In the SYN_SENT state, if receive a ACK without SYN,
       * we should reset the connection and retransmit the SYN.
       */

      if (((conn->tcpstateflags & TCP_STATE_MASK) == TCP_SYN_SENT) &&
          ((tcp->flags & TCP_SYN) == 0 && (tcp->flags & TCP_ACK) != 0))
        {
          /* Send the RST to close the half-open connection. */

          tcp_reset(dev, conn);

          /* Retransmit the SYN as soon as possible in order to establish
           * the tcp connection.
           */

          tcp_update_retrantimer(conn, 1);

          return;
        }

      if (seq != rcvseq)
        {
          /* Trim the head of the segment */

          if (TCP_SEQ_LT(seq, rcvseq))
            {
              uint32_t trimlen = TCP_SEQ_SUB(rcvseq, seq);

              if (tcp_trim_head(dev, tcp, trimlen))
                {
                  /* The segment was completely out of the window.
                   * E.g. a retransmit which was not necessary.
                   * E.g. a keep-alive segment.
                   */

                  tcp_send(dev, conn, TCP_ACK, tcpiplen);
                  return;
                }
            }
          else if ((conn->tcpstateflags & TCP_STATE_MASK) <= TCP_ESTABLISHED)
            {
#ifdef CONFIG_NET_TCP_OUT_OF_ORDER
              /* Queue out-of-order segments. */

              tcp_input_ofosegs(dev, conn, iplen);
#endif
              if ((conn->tcpstateflags & TCP_STATE_MASK) <= TCP_ESTABLISHED)
                {
                  tcp_send(dev, conn, TCP_ACK, tcpiplen);
                  return;
                }
            }
        }
    }

  tcp_clear_zero_probe(conn, tcp);

  /* Update the connection's window size */

  if ((tcp->flags & TCP_ACK) != 0 &&
      (conn->tcpstateflags & TCP_STATE_MASK) != TCP_SYN_RCVD)
    {
#ifdef CONFIG_NET_TCP_CC_NEWRENO
      /* If the packet is ack, update the cc var. */

      tcp_cc_recv_ack(conn, tcp);
#endif
      if (tcp_snd_wnd_update(conn, tcp))
        {
          /* Window updated, set the acknowledged flag. */

          flags |= TCP_ACKDATA;
        }
    }

  /* Do different things depending on in what state the connection is. */

  switch (conn->tcpstateflags & TCP_STATE_MASK)
    {
      /* CLOSED and LISTEN are not handled here. CLOSE_WAIT is not
       * implemented, since we force the application to close when the
       * peer sends a FIN (hence the application goes directly from
       * ESTABLISHED to LAST_ACK).
       */

      case TCP_SYN_RCVD:
        /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
         * we are waiting for an ACK that acknowledges the data we sent
         * out the last time. Therefore, we want to have the TCP_ACKDATA
         * flag set. If so, we enter the ESTABLISHED state.
         */

        if ((flags & TCP_ACKDATA) != 0)
          {
            /* The three way handshake is complete and the TCP connection
             * is now in the ESTABLISHED state.
             */

            conn->tcpstateflags = TCP_ESTABLISHED;

            /* Wake up any listener waiting for a connection on this port */

            if (tcp_accept_connection(dev, conn, tcp->destport) != OK)
              {
                /* No more listener for current port.  We can free conn here
                 * because it has not been shared with upper layers yet as
                 * handshake is not complete
                 */

                nwarn("WARNING: Listen canceled while waiting for ACK on "
                      "port %d\n", NTOHS(tcp->destport));

                /* Free the connection structure */

                conn->crefs = 0;
                tcp_free(conn);
                conn = NULL;

                /* And send a reset packet to the remote host. */

                goto reset;
              }

#ifdef CONFIG_NET_TCP_WRITE_BUFFERS
            conn->isn           = tcp_getsequence(tcp->ackno);
            tcp_setsequence(conn->sndseq, conn->isn);
            conn->sent          = 0;
            conn->sndseq_max    = 0;
#endif
            conn->tx_unacked    = 0;
            tcp_snd_wnd_init(conn, tcp);
            tcp_snd_wnd_update(conn, tcp);

#ifdef CONFIG_NET_TCP_CC_NEWRENO
            tcp_cc_update(conn, tcp);
#endif
            flags               = TCP_CONNECTED;
            ninfo("TCP state: TCP_ESTABLISHED\n");

            if (dev->d_len > 0)
              {
                flags          |= TCP_NEWDATA;
              }

            dev->d_sndlen       = 0;
            result              = tcp_callback(dev, conn, flags);
            tcp_appsend(dev, conn, result);
            return;
          }

        /* We need to retransmit the SYNACK */

        if ((tcp->flags & TCP_CTL) == TCP_SYN)
          {
#if !defined(CONFIG_NET_TCP_WRITE_BUFFERS)
            tcp_setsequence(conn->sndseq, conn->rexmit_seq);
#else
            /* REVISIT for the buffered mode */
#endif
            tcp_synack(dev, conn, TCP_ACK | TCP_SYN);
            return;
          }

        goto drop;

      case TCP_SYN_SENT:
        /* In SYN_SENT, we wait for a SYNACK that is sent in response to
         * our SYN. The rcvseq is set to sequence number in the SYNACK
         * plus one, and we send an ACK. We move into the ESTABLISHED
         * state.
         */

        if ((flags & TCP_ACKDATA) != 0 &&
            (tcp->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))
          {
            /* Parse the TCP MSS option, if present. */

            tcp_parse_option(dev, conn, iplen);

            conn->tcpstateflags = TCP_ESTABLISHED;
            memcpy(conn->rcvseq, tcp->seqno, 4);
            conn->rcv_adv = tcp_getsequence(conn->rcvseq);
            tcp_snd_wnd_init(conn, tcp);
            tcp_snd_wnd_update(conn, tcp);

#ifdef CONFIG_NET_TCP_CC_NEWRENO
            tcp_cc_update(conn, tcp);
#endif
            net_incr32(conn->rcvseq, 1); /* ack SYN */
            conn->tx_unacked    = 0;

#ifdef CONFIG_NET_TCP_WRITE_BUFFERS
            conn->isn           = tcp_getsequence(tcp->ackno);
            tcp_setsequence(conn->sndseq, conn->isn);
#endif
            dev->d_len          = 0;
            dev->d_sndlen       = 0;

            ninfo("TCP state: TCP_ESTABLISHED\n");
            result = tcp_callback(dev, conn, TCP_CONNECTED | TCP_NEWDATA);
            tcp_appsend(dev, conn, result);
            return;
          }

        /* Inform the application that the connection failed */

        tcp_callback(dev, conn, TCP_ABORT);

        /* The connection is closed after we send the RST */

        conn->tcpstateflags = TCP_CLOSED;
        ninfo("Connection failed - TCP state: TCP_CLOSED\n");

        /* We do not send resets in response to resets. */

        if ((tcp->flags & TCP_RST) != 0)
          {
            goto drop;
          }

        tcp_reset(dev, conn);
        return;

      case TCP_ESTABLISHED:
        /* In the ESTABLISHED state, we call upon the application to feed
         * data into the d_buf.  If the TCP_ACKDATA flag is set, the
         * application should put new data into the buffer, otherwise we are
         * retransmitting an old segment, and the application should put that
         * data into the buffer.
         *
         * If the incoming packet is a FIN, we should close the connection on
         * this side as well, and we send out a FIN and enter the LAST_ACK
         * state.  We require that there is no outstanding data; otherwise
         * the sequence numbers will be screwed up.
         */

        if ((tcp->flags & TCP_FIN) != 0 &&
            (conn->tcpstateflags & TCP_STOPPED) == 0)
          {
            /* Needs to be investigated further.
             * Windows often sends FIN packets together with the last ACK for
             * the received data. So the socket layer has to get this ACK
             * even if the connection is going to be closed.
             */

#if 0
            if (conn->tx_unacked > 0)
              {
                goto drop;
              }
#endif

            /* Update the sequence number and indicate that the connection
             * has been closed.
             */

            flags |= TCP_CLOSE;

            if (dev->d_len > 0)
              {
                flags |= TCP_NEWDATA;
              }

            result = tcp_callback(dev, conn, flags);

            if ((result & TCP_CLOSE) != 0)
              {
                conn->tcpstateflags = TCP_LAST_ACK;
                conn->tx_unacked    = 1;
                conn->nrtx          = 0;
                net_incr32(conn->rcvseq, 1); /* ack FIN */
#ifdef CONFIG_NET_TCP_WRITE_BUFFERS
                conn->sndseq_max    = tcp_getsequence(conn->sndseq) + 1;
#endif
                ninfo("TCP state: TCP_LAST_ACK\n");
                tcp_send(dev, conn, TCP_FIN | TCP_ACK, tcpiplen);
              }
            else
              {
                ninfo("TCP: Dropped a FIN\n");
                tcp_appsend(dev, conn, result);
              }

            return;
          }

#ifdef CONFIG_NET_TCPURGDATA
        /* Check the URG flag.  If this is set, the segment carries urgent
         * data that we must pass to the application.
         */

        if ((tcp->flags & TCP_URG) != 0)
          {
            dev->d_urglen = (tcp->urgp[0] << 8) | tcp->urgp[1];
            if (dev->d_urglen > dev->d_len)
              {
                /* There is more urgent data in the next segment to come. */

                dev->d_urglen = dev->d_len;
              }

             /* The d_len field contains the length of the incoming data.
              * d_urgdata points to the "urgent" data at the beginning of
              * the payload; d_appdata field points to the any "normal" data
              * that may follow the urgent data.
              *
              * NOTE: If the urgent data continues in the next packet, then
              * d_len will be zero and d_appdata will point past the end of
              * the payload (which is OK).
              */

            net_incr32(conn->rcvseq, dev->d_urglen);
            dev->d_len     -= dev->d_urglen;
            dev->d_urgdata  = dev->d_appdata;
            dev->d_appdata += dev->d_urglen;
          }
        else
          {
            /* No urgent data */

            dev->d_urglen   = 0;
          }

#else /* CONFIG_NET_TCPURGDATA */
        /* Check the URG flag.  If this is set, We must gracefully ignore
         * and discard the urgent data.
         */

        if ((tcp->flags & TCP_URG) != 0)
          {
            uint16_t urglen = (tcp->urgp[0] << 8) | tcp->urgp[1];
            if (urglen > dev->d_len)
              {
                /* There is more urgent data in the next segment to come. */

                urglen = dev->d_len;
              }

             /* The d_len field contains the length of the incoming data;
              * The d_appdata field points to the any "normal" data that
              * may follow the urgent data.
              *
              * NOTE: If the urgent data continues in the next packet, then
              * d_len will be zero and d_appdata will point past the end of
              * the payload (which is OK).
              */

            net_incr32(conn->rcvseq, urglen);
            dev->d_len     -= urglen;
            dev->d_appdata += urglen;
          }
#endif /* CONFIG_NET_TCPURGDATA */

#ifdef CONFIG_NET_TCP_KEEPALIVE
        /* If the established socket receives an ACK or any kind of data
         * from the remote peer (whether we accept it or not), then reset
         * the keep alive timer.
         */

        if (conn->keepalive &&
            (dev->d_len > 0 || (tcp->flags & TCP_ACK) != 0))
          {
            /* Reset the "alive" timer. */

            tcp_update_keeptimer(conn, conn->keepidle);
            conn->keepretries = 0;
          }
#endif

        /* If d_len > 0 we have TCP data in the packet, and we flag this
         * by setting the TCP_NEWDATA flag. If the application has stopped
         * the data flow using TCP_STOPPED, we must not accept any data
         * packets from the remote host.
         */

        if (dev->d_len > 0 && (conn->tcpstateflags & TCP_STOPPED) == 0)
          {
            flags |= TCP_NEWDATA;
          }

        /* If this packet constitutes an ACK for outstanding data (flagged
         * by the TCP_ACKDATA flag), we should call the application since it
         * might want to send more data.  If the incoming packet had data
         * from the peer (as flagged by the TCP_NEWDATA flag), the
         * application must also be notified.
         *
         * When the application is called, the d_len field
         * contains the length of the incoming data.  The application can
         * access the incoming data through the global pointer
         * d_appdata, which usually points hdrlen bytes into the d_buf
         * array.
         *
         * If the application wishes to send any data, this data should be
         * put into the d_appdata and the length of the data should be
         * put into d_len.  If the application don't have any data to
         * send, d_len must be set to 0.
         */

        if ((flags & (TCP_NEWDATA | TCP_ACKDATA)) != 0)
          {
            dev->d_sndlen = 0;

            /* Provide the packet to the application */

            result = tcp_callback(dev, conn, flags);

            /* Send the response, ACKing the data or not, as appropriate */

            tcp_appsend(dev, conn, result);
            return;
          }

        goto drop;

      case TCP_LAST_ACK:
        /* We can close this connection if the peer has acknowledged our
         * FIN. This is indicated by the TCP_ACKDATA flag.
         */

        if ((flags & TCP_ACKDATA) != 0)
          {
            conn->tcpstateflags = TCP_CLOSED;
            ninfo("TCP_LAST_ACK TCP state: TCP_CLOSED\n");

            tcp_callback(dev, conn, TCP_CLOSE);
          }
        break;

      case TCP_FIN_WAIT_1:
        /* The application has closed the connection, but the remote host
         * hasn't closed its end yet.  Thus we stay in the FIN_WAIT_1 state
         * until we receive a FIN from the remote.
         */

        if (dev->d_len > 0)
          {
            net_incr32(conn->rcvseq, dev->d_len);
          }

        if ((tcp->flags & TCP_FIN) != 0)
          {
            if ((flags & TCP_ACKDATA) != 0 && conn->tx_unacked == 0)
              {
                conn->tcpstateflags = TCP_TIME_WAIT;
                tcp_update_retrantimer(conn,
                                       TCP_TIME_WAIT_TIMEOUT * HSEC_PER_SEC);
                ninfo("TCP state: TCP_TIME_WAIT\n");
              }
            else
              {
                conn->tcpstateflags = TCP_CLOSING;
                ninfo("TCP state: TCP_CLOSING\n");
              }

            net_incr32(conn->rcvseq, 1); /* ack FIN */
            tcp_callback(dev, conn, TCP_CLOSE);
            tcp_send(dev, conn, TCP_ACK, tcpiplen);
            return;
          }
        else if ((flags & TCP_ACKDATA) != 0 && conn->tx_unacked == 0)
          {
            conn->tcpstateflags = TCP_FIN_WAIT_2;
            ninfo("TCP state: TCP_FIN_WAIT_2\n");
            goto drop;
          }

        if (dev->d_len > 0)
          {
            /* Due to RFC 2525, Section 2.17, we SHOULD send RST if we can no
             * longer read any received data. Also set state into TCP_CLOSED
             * because the peer will not send FIN after RST received.
             *
             * TODO: Modify shutdown behavior to allow read in FIN_WAIT.
             */

            conn->tcpstateflags = TCP_CLOSED;

            /* In the TCP_FIN_WAIT_1, we need call tcp_close_eventhandler to
             * release nofosegs, that we received in this state.
             */

            tcp_callback(dev, conn, TCP_CLOSE);
            tcp_reset(dev, conn);
            return;
          }

        goto drop;

      case TCP_FIN_WAIT_2:
        if (dev->d_len > 0)
          {
            net_incr32(conn->rcvseq, dev->d_len);
          }

        if ((tcp->flags & TCP_FIN) != 0)
          {
            conn->tcpstateflags = TCP_TIME_WAIT;
            tcp_update_retrantimer(conn,
                                   TCP_TIME_WAIT_TIMEOUT * HSEC_PER_SEC);
            ninfo("TCP state: TCP_TIME_WAIT\n");

            net_incr32(conn->rcvseq, 1); /* ack FIN */
            tcp_callback(dev, conn, TCP_CLOSE);
            tcp_send(dev, conn, TCP_ACK, tcpiplen);
            return;
          }

        if (dev->d_len > 0)
          {
            /* Due to RFC 2525, Section 2.17, we SHOULD send RST if we can no
             * longer read any received data. Also set state into TCP_CLOSED
             * because the peer will not send FIN after RST received.
             */

            conn->tcpstateflags = TCP_CLOSED;

            /* In the TCP_FIN_WAIT_2, we need call tcp_close_eventhandler to
             * release nofosegs, that we received in this state.
             */

            tcp_callback(dev, conn, TCP_CLOSE);
            tcp_reset(dev, conn);
            return;
          }

        goto drop;

      case TCP_TIME_WAIT:
        tcp_send(dev, conn, TCP_ACK, tcpiplen);
        return;

      case TCP_CLOSING:
        if ((flags & TCP_ACKDATA) != 0)
          {
            conn->tcpstateflags = TCP_TIME_WAIT;
            tcp_update_retrantimer(conn,
                                   TCP_TIME_WAIT_TIMEOUT * HSEC_PER_SEC);
            ninfo("TCP state: TCP_TIME_WAIT\n");
          }

      default:
        break;
    }

drop:
  dev->d_len = 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_reorder_ofosegs
 *
 * Description:
 *   Sort out-of-order segments by left edge
 *
 * Input Parameters:
 *   nofosegs - Number of out-of-order semgnets
 *   ofosegs  - Pointer to out-of-order segments
 *
 * Returned Value:
 *   True if re-order occurs
 *
 * Assumptions:
 *   The network is locked.
 *
 ****************************************************************************/

bool tcp_reorder_ofosegs(int nofosegs, FAR struct tcp_ofoseg_s *ofosegs)
{
  struct tcp_ofoseg_s segs;
  bool reordered = false;
  int i;
  int j;

  /* Sort out-of-order segments by left edge */

  for (i = 0; i < nofosegs - 1; i++)
    {
      for (j = 0; j < nofosegs - 1 - i; j++)
        {
          if (TCP_SEQ_GT(ofosegs[j].left,
                         ofosegs[j + 1].left))
            {
              segs = ofosegs[j];
              ofosegs[j] = ofosegs[j + 1];
              ofosegs[j + 1] = segs;
              reordered = true;
            }
        }
    }

  return reordered;
}

/****************************************************************************
 * Name: tcp_ipv4_input
 *
 * Description:
 *   Handle incoming TCP input with IPv4 header
 *
 * Input Parameters:
 *   dev - The device driver structure containing the received TCP packet.
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Called from the Ethernet driver with the network stack locked
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
void tcp_ipv4_input(FAR struct net_driver_s *dev)
{
  FAR struct ipv4_hdr_s *ipv4 = IPv4BUF;
  uint16_t iphdrlen;

  /* Configure to receive an TCP IPv4 packet */

  tcp_ipv4_select(dev);

  /* Get the IP header length (accounting for possible options). */

  iphdrlen = (ipv4->vhl & IPv4_HLMASK) << 2;

  /* Then process in the TCP IPv4 input */

  tcp_input(dev, PF_INET, iphdrlen);
}
#endif

/****************************************************************************
 * Name: tcp_ipv6_input
 *
 * Description:
 *   Handle incoming TCP input with IPv4 header
 *
 * Input Parameters:
 *   dev   - The device driver structure containing the received TCP packet.
 *   iplen - The size of the IPv6 header.  This may be larger than
 *           IPv6_HDRLEN the IPv6 header if IPv6 extension headers are
 *           present.
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Called from the Ethernet driver with the network stack locked
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
void tcp_ipv6_input(FAR struct net_driver_s *dev, unsigned int iplen)
{
  /* Configure to receive an TCP IPv6 packet */

  tcp_ipv6_select(dev);

  /* Then process in the TCP IPv6 input */

  tcp_input(dev, PF_INET6, iplen);
}
#endif

#endif /* CONFIG_NET  && CONFIG_NET_TCP */
