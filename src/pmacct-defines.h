/*
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2012 by Paolo Lucente
*/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* defines */
#define ARGS_NFACCTD "n:dDhP:b:f:F:c:m:p:r:s:S:L:l:v:o:O:uR"
#define ARGS_SFACCTD "n:dDhP:b:f:F:c:m:p:r:s:S:L:l:v:o:O:uR"
#define ARGS_PMACCTD "n:NdDhP:b:f:F:c:i:I:m:p:r:s:S:v:o:O:uwWL:R"
#define ARGS_UACCTD "n:NdDhP:b:f:F:c:m:p:r:s:S:v:o:O:uRg:L:"
#define ARGS_PMACCT "Ssc:Cetm:p:P:M:arN:n:lT:Ou"
#define N_PRIMITIVES 48
#define N_FUNCS 10 
#define MAX_N_PLUGINS 32
#define PROTO_LEN 12
#define MAX_MAP_ENTRIES 2048 /* allow maps */
#define BGP_MD5_MAP_ENTRIES 8192
#define AGG_FILTER_ENTRIES 128 
#define FOLLOW_BGP_NH_ENTRIES 32 
#define MAX_PROTOCOL_LEN 16
#define UINT32T_THRESHOLD 4290000000UL
#define UINT64T_THRESHOLD 18446744073709551360ULL
#ifndef UINT8_MAX
#define UINT8_MAX (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

#if defined ENABLE_IPV6
#define DEFAULT_SNAPLEN 128
#else
#define DEFAULT_SNAPLEN 68
#endif
#define SNAPLEN_ISIS_MIN 512
#define SNAPLEN_ISIS_DEFAULT 1476

#define SRVBUFLEN (256+MOREBUFSZ)
#define LONGSRVBUFLEN (384+MOREBUFSZ)
#define LONGLONGSRVBUFLEN (1024+MOREBUFSZ)
#define LARGEBUFLEN (8192+MOREBUFSZ)

#define MANTAINER "Paolo Lucente <paolo@pmacct.net>"
#define PMACCTD_USAGE_HEADER "Promiscuous Mode Accounting Daemon, pmacctd 0.14.2-cvs"
#define UACCTD_USAGE_HEADER "Linux NetFilter ULOG Accounting Daemon, pmacctd 0.14.2-cvs"
#define PMACCT_USAGE_HEADER "pmacct, pmacct client 0.14.2-cvs"
#define PMMYPLAY_USAGE_HEADER "pmmyplay, pmacct MySQL logfile player 0.14.2-cvs"
#define PMPGPLAY_USAGE_HEADER "pmpgplay, pmacct PGSQL logfile player 0.14.2-cvs"
#define NFACCTD_USAGE_HEADER "NetFlow Accounting Daemon, nfacctd 0.14.2-cvs"
#define SFACCTD_USAGE_HEADER "sFlow Accounting Daemon, sfacctd 0.14.2-cvs"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define ERR -1
#define SUCCESS 0

#define	E_NOTFOUND	2

#ifndef MIN
#define MIN(x, y) (x <= y ? x : y)
#endif

#ifndef MAX
#define MAX(x, y) (x <= y ? y : x)
#endif

/* acct_type */ 
#define ACCT_PM		1	/* promiscuous mode */
#define ACCT_NF		2	/* NetFlow */
#define ACCT_SF		3	/* sFlow */
#define ACCT_UL		4	/* Linux NetFilter ULOG */

/* map type */
#define MAP_TAG 		0	/* pre_tag_map */
#define MAP_BGP_PEER_AS_SRC	100	/* bgp_peer_src_as_map */
#define MAP_BGP_TO_XFLOW_AGENT	101	/* bgp_to_agent_map */
#define MAP_BGP_SRC_LOCAL_PREF	102	/* bgp_src_local_pref_map */
#define MAP_BGP_SRC_MED		103	/* bgp_src_med_map */
#define MAP_BGP_IFACE_TO_RD	104	/* bgp_iface_to_rd */
#define MAP_SAMPLING		105	/* sampling_map */

/* PRIMITIVES DEFINITION: START */
/* 50 primitives currently defined */
/* internal: first registry */
#define COUNT_INT_SRC_HOST		0x0001000000000001ULL
#define COUNT_INT_DST_HOST		0x0001000000000002ULL
#define COUNT_INT_SUM_HOST		0x0001000000000004ULL
#define COUNT_INT_SRC_PORT		0x0001000000000008ULL
#define COUNT_INT_DST_PORT		0x0001000000000010ULL
#define COUNT_INT_IP_PROTO		0x0001000000000020ULL
#define COUNT_INT_SRC_MAC 		0x0001000000000040ULL
#define COUNT_INT_DST_MAC 		0x0001000000000080ULL
#define COUNT_INT_SRC_NET		0x0001000000000100ULL
#define COUNT_INT_DST_NET		0x0001000000000200ULL
#define COUNT_INT_ID			0x0001000000000400ULL
#define COUNT_INT_VLAN			0x0001000000000800ULL
#define COUNT_INT_IP_TOS		0x0001000000001000ULL
#define COUNT_INT_NONE			0x0001000000002000ULL
#define COUNT_INT_SRC_AS		0x0001000000004000ULL
#define COUNT_INT_DST_AS		0x0001000000008000ULL
#define COUNT_INT_SUM_NET		0x0001000000010000ULL
#define COUNT_INT_SUM_AS		0x0001000000020000ULL
#define COUNT_INT_SUM_PORT		0x0001000000040000ULL
#define INT_TIMESTAMP			0x0001000000080000ULL /* USE_TIMESTAMPS */
#define COUNT_INT_FLOWS			0x0001000000100000ULL
#define COUNT_INT_SUM_MAC		0x0001000000200000ULL
#define COUNT_INT_CLASS			0x0001000000400000ULL
#define COUNT_INT_COUNTERS		0x0001000000800000ULL
#define COUNT_INT_PAYLOAD		0x0001000001000000ULL
#define COUNT_INT_TCPFLAGS		0x0001000002000000ULL
#define COUNT_INT_STD_COMM		0x0001000004000000ULL
#define COUNT_INT_EXT_COMM		0x0001000008000000ULL
#define COUNT_INT_AS_PATH		0x0001000010000000ULL
#define COUNT_INT_LOCAL_PREF		0x0001000020000000ULL
#define COUNT_INT_MED			0x0001000040000000ULL
#define COUNT_INT_PEER_SRC_AS		0x0001000080000000ULL
#define COUNT_INT_PEER_DST_AS		0x0001000100000000ULL
#define COUNT_INT_PEER_SRC_IP		0x0001000200000000ULL
#define COUNT_INT_PEER_DST_IP		0x0001000400000000ULL
#define COUNT_INT_ID2			0x0001000800000000ULL
#define COUNT_INT_SRC_AS_PATH		0x0001001000000000ULL
#define COUNT_INT_SRC_STD_COMM		0x0001002000000000ULL
#define COUNT_INT_SRC_EXT_COMM		0x0001004000000000ULL
#define COUNT_INT_SRC_LOCAL_PREF	0x0001008000000000ULL
#define COUNT_INT_SRC_MED		0x0001010000000000ULL
#define COUNT_INT_MPLS_VPN_RD		0x0001020000000000ULL
#define COUNT_INT_IN_IFACE		0x0001040000000000ULL
#define COUNT_INT_OUT_IFACE		0x0001080000000000ULL
#define COUNT_INT_SRC_NMASK		0x0001100000000000ULL
#define COUNT_INT_DST_NMASK		0x0001200000000000ULL
#define COUNT_INT_COS			0x0001400000000000ULL
#define COUNT_INT_ETHERTYPE		0x0001800000000000ULL

/* internal: second registry */
#define COUNT_INT_SAMPLING_RATE		0x0002000000000001ULL
#define COUNT_INT_SRC_COUNTRY		0x0002000000000002ULL
#define COUNT_INT_DST_COUNTRY		0x0002000000000004ULL

#define COUNT_INDEX_MASK	0xFFFF
#define COUNT_REGISTRY_MASK	0xFFFFFFFFFFFFULL
#define COUNT_REGISTRY_BITS	48

/* external: first registry */
#define COUNT_SRC_HOST                  (COUNT_INT_SRC_HOST & COUNT_REGISTRY_MASK)
#define COUNT_DST_HOST                  (COUNT_INT_DST_HOST & COUNT_REGISTRY_MASK)
#define COUNT_SUM_HOST                  (COUNT_INT_SUM_HOST & COUNT_REGISTRY_MASK)
#define COUNT_SRC_PORT                  (COUNT_INT_SRC_PORT & COUNT_REGISTRY_MASK)
#define COUNT_DST_PORT                  (COUNT_INT_DST_PORT & COUNT_REGISTRY_MASK)
#define COUNT_IP_PROTO                  (COUNT_INT_IP_PROTO & COUNT_REGISTRY_MASK)
#define COUNT_SRC_MAC                   (COUNT_INT_SRC_MAC & COUNT_REGISTRY_MASK)
#define COUNT_DST_MAC                   (COUNT_INT_DST_MAC & COUNT_REGISTRY_MASK)
#define COUNT_SRC_NET                   (COUNT_INT_SRC_NET & COUNT_REGISTRY_MASK)
#define COUNT_DST_NET                   (COUNT_INT_DST_NET & COUNT_REGISTRY_MASK)
#define COUNT_ID                        (COUNT_INT_ID & COUNT_REGISTRY_MASK)
#define COUNT_VLAN                      (COUNT_INT_VLAN & COUNT_REGISTRY_MASK)
#define COUNT_IP_TOS                    (COUNT_INT_IP_TOS & COUNT_REGISTRY_MASK)
#define COUNT_NONE                      (COUNT_INT_NONE & COUNT_REGISTRY_MASK)
#define COUNT_SRC_AS                    (COUNT_INT_SRC_AS & COUNT_REGISTRY_MASK)
#define COUNT_DST_AS                    (COUNT_INT_DST_AS & COUNT_REGISTRY_MASK)
#define COUNT_SUM_NET                   (COUNT_INT_SUM_NET & COUNT_REGISTRY_MASK)
#define COUNT_SUM_AS                    (COUNT_INT_SUM_AS & COUNT_REGISTRY_MASK)
#define COUNT_SUM_PORT                  (COUNT_INT_SUM_PORT & COUNT_REGISTRY_MASK)
#define TIMESTAMP                       (INT_TIMESTAMP & COUNT_REGISTRY_MASK)
#define COUNT_FLOWS                     (COUNT_INT_FLOWS & COUNT_REGISTRY_MASK)
#define COUNT_SUM_MAC                   (COUNT_INT_SUM_MAC & COUNT_REGISTRY_MASK)
#define COUNT_CLASS                     (COUNT_INT_CLASS & COUNT_REGISTRY_MASK)
#define COUNT_COUNTERS                  (COUNT_INT_COUNTERS & COUNT_REGISTRY_MASK)
#define COUNT_PAYLOAD                   (COUNT_INT_PAYLOAD & COUNT_REGISTRY_MASK)
#define COUNT_TCPFLAGS                  (COUNT_INT_TCPFLAGS & COUNT_REGISTRY_MASK)
#define COUNT_STD_COMM                  (COUNT_INT_STD_COMM & COUNT_REGISTRY_MASK)
#define COUNT_EXT_COMM                  (COUNT_INT_EXT_COMM & COUNT_REGISTRY_MASK)
#define COUNT_AS_PATH                   (COUNT_INT_AS_PATH & COUNT_REGISTRY_MASK)
#define COUNT_LOCAL_PREF                (COUNT_INT_LOCAL_PREF & COUNT_REGISTRY_MASK)
#define COUNT_MED                       (COUNT_INT_MED & COUNT_REGISTRY_MASK)
#define COUNT_PEER_SRC_AS               (COUNT_INT_PEER_SRC_AS & COUNT_REGISTRY_MASK)
#define COUNT_PEER_DST_AS               (COUNT_INT_PEER_DST_AS & COUNT_REGISTRY_MASK)
#define COUNT_PEER_SRC_IP               (COUNT_INT_PEER_SRC_IP & COUNT_REGISTRY_MASK)
#define COUNT_PEER_DST_IP               (COUNT_INT_PEER_DST_IP & COUNT_REGISTRY_MASK)
#define COUNT_ID2                       (COUNT_INT_ID2 & COUNT_REGISTRY_MASK)
#define COUNT_SRC_AS_PATH               (COUNT_INT_SRC_AS_PATH & COUNT_REGISTRY_MASK)
#define COUNT_SRC_STD_COMM              (COUNT_INT_SRC_STD_COMM & COUNT_REGISTRY_MASK)
#define COUNT_SRC_EXT_COMM              (COUNT_INT_SRC_EXT_COMM & COUNT_REGISTRY_MASK)
#define COUNT_SRC_LOCAL_PREF            (COUNT_INT_SRC_LOCAL_PREF & COUNT_REGISTRY_MASK)
#define COUNT_SRC_MED                   (COUNT_INT_SRC_MED & COUNT_REGISTRY_MASK)
#define COUNT_MPLS_VPN_RD               (COUNT_INT_MPLS_VPN_RD & COUNT_REGISTRY_MASK)
#define COUNT_IN_IFACE                  (COUNT_INT_IN_IFACE & COUNT_REGISTRY_MASK)
#define COUNT_OUT_IFACE                 (COUNT_INT_OUT_IFACE & COUNT_REGISTRY_MASK)
#define COUNT_SRC_NMASK                 (COUNT_INT_SRC_NMASK & COUNT_REGISTRY_MASK)
#define COUNT_DST_NMASK                 (COUNT_INT_DST_NMASK & COUNT_REGISTRY_MASK)
#define COUNT_COS                       (COUNT_INT_COS & COUNT_REGISTRY_MASK)
#define COUNT_ETHERTYPE                 (COUNT_INT_ETHERTYPE & COUNT_REGISTRY_MASK)

/* external: second registry */
#define COUNT_SAMPLING_RATE		(COUNT_INT_SAMPLING_RATE & COUNT_REGISTRY_MASK)
#define COUNT_SRC_COUNTRY		(COUNT_INT_SRC_COUNTRY & COUNT_REGISTRY_MASK)
#define COUNT_DST_COUNTRY		(COUNT_INT_DST_COUNTRY & COUNT_REGISTRY_MASK)
/* PRIMITIVES DEFINITION: END */

/* BYTES and PACKETS are used into templates; we let their values to
   overlap with some values we will not need into templates */ 
#define LT_BYTES		COUNT_SRC_NET
#define LT_PACKETS		COUNT_DST_NET
#define LT_FLOWS		COUNT_SUM_HOST
#define LT_NO_L2		COUNT_SUM_NET

#define FAKE_SRC_MAC		0x00000001
#define FAKE_DST_MAC		0x00000002
#define FAKE_SRC_HOST		0x00000004
#define FAKE_DST_HOST		0x00000008
#define FAKE_SRC_AS		0x00000010
#define FAKE_DST_AS		0x00000020
#define FAKE_COMMS		0x00000040
#define FAKE_PEER_SRC_AS	0x00000080
#define FAKE_PEER_DST_AS	0x00000100
#define FAKE_PEER_SRC_IP	0x00000200
#define FAKE_PEER_DST_IP	0x00000400
#define FAKE_AS_PATH		0x00000800

#define COUNT_MINUTELY          0x00000001
#define COUNT_HOURLY            0x00000002
#define COUNT_DAILY             0x00000004
#define COUNT_WEEKLY		0x00000008
#define COUNT_MONTHLY		0x00000010

#define WANT_STATS		0x00000001
#define WANT_ERASE		0x00000002
#define WANT_STATUS		0x00000004
#define WANT_COUNTER		0x00000008
#define WANT_MATCH		0x00000010
#define WANT_RESET		0x00000020
#define WANT_CLASS_TABLE	0x00000040
#define WANT_LOCK_OP		0x00000080

#define PIPE_TYPE_METADATA	0x00000001
#define PIPE_TYPE_PAYLOAD	0x00000002
#define PIPE_TYPE_EXTRAS	0x00000004
#define PIPE_TYPE_BGP		0x00000008
#define PIPE_TYPE_MSG		0x00000010

#define CHLD_WARNING		0x00000001
#define CHLD_ALERT		0x00000002

#define BGP_SRC_PRIMITIVES_KEEP	0x00000001
#define BGP_SRC_PRIMITIVES_MAP	0x00000002
#define BGP_SRC_PRIMITIVES_BGP	0x00000004

#define PRINT_OUTPUT_FORMATTED	0x00000001
#define PRINT_OUTPUT_CSV	0x00000002

#define DIRECTION_UNKNOWN	0x00000000
#define DIRECTION_IN		0x00000001
#define DIRECTION_OUT		0x00000002
#define DIRECTION_TAG		0x00000004
#define DIRECTION_TAG2		0x00000008

#define IFINDEX_STATIC		0x00000001
#define IFINDEX_TAG		0x00000002
#define IFINDEX_TAG2		0x00000004

typedef u_int32_t pm_class_t;
typedef u_int64_t pm_id_t;

#if defined HAVE_64BIT_COUNTERS
typedef u_int64_t pm_counter_t;
#else
typedef u_int32_t pm_counter_t;
#endif

/* Keep common NF_AS and NF_NET values aligned, ie. NF_[NET|AS]_KEEP == 0x00000001 */
#define NF_AS_COMPAT    0x00000000 /* Unused */
#define NF_AS_KEEP	0x00000001 /* Keep AS numbers in Sflow or NetFlow packets */
#define NF_AS_NEW 	0x00000002 /* ignore ASN from NetFlow and generate from network files */
#define NF_AS_BGP	0x00000004 /* ignore ASN from NetFlow and generate from BGP peerings */
#define NF_AS_FALLBACK	0x80000000 /* Fallback flag */

#define NF_NET_COMPAT   0x00000000 /* Backward compatibility selection */
#define NF_NET_KEEP     0x00000001 /* Determine IP network prefixes from sFlow or NetFlow data */
#define NF_NET_NEW      0x00000002 /* Determine IP network prefixes from network files */
#define NF_NET_BGP      0x00000004 /* Determine IP network prefixes from BGP peerings */
#define NF_NET_STATIC   0x00000008 /* Determine IP network prefixes from static mask */
#define NF_NET_IGP	0x00000010 /* Determine IP network prefixes from IGP */
#define NF_NET_FALLBACK	0x80000000 /* Fallback flag */
