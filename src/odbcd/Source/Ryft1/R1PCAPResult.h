// =================================================================================================
///  @file R1PCAPResult.h
///
///  Implements the result class for PCAP data searches
///
///  Copyright (C) 2018 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1PCAPRESULT_H_
#define _R1PCAPRESULT_H_

#include <string.h>
#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <GeoIP.h>
#include <GeoIPCity.h>
#include <string>
#include <vector>
#include <deque>
#include <streambuf>
#include <istream>
#include <iostream>
#include <map>
using namespace std;

/*
* 	struct vlan_hdr - vlan header
* 	@h_vlan_TCI: priority and VLAN ID
*	@h_vlan_encapsulated_proto: packet type ID or len
*/
struct vlan_hdr {
    __be16	h_vlan_TCI;
    __be16	h_vlan_encapsulated_proto;
};

#define ETHERTYPE_MPLS  0x8847

/* Reference: RFC 5462, RFC 3032
*
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |                Label                  | TC  |S|       TTL     |
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*	Label:  Label Value, 20 bits
*	TC:     Traffic Class field, 3 bits
*	S:      Bottom of Stack, 1 bit
*	TTL:    Time to Live, 8 bits
*/

struct mpls_label {
    __be32 entry;
};

#define MPLS_LS_LABEL_MASK      0xFFFFF000
#define MPLS_LS_LABEL_SHIFT     12
#define MPLS_LS_TC_MASK         0x00000E00
#define MPLS_LS_TC_SHIFT        9
#define MPLS_LS_S_MASK          0x00000100
#define MPLS_LS_S_SHIFT         8
#define MPLS_LS_TTL_MASK        0x000000FF
#define MPLS_LS_TTL_SHIFT       0

/* Reserved labels */
#define MPLS_LABEL_IPV4NULL		0 /* RFC3032 */
#define MPLS_LABEL_RTALERT		1 /* RFC3032 */
#define MPLS_LABEL_IPV6NULL		2 /* RFC3032 */
#define MPLS_LABEL_IMPLNULL		3 /* RFC3032 */
#define MPLS_LABEL_ENTROPY		7 /* RFC6790 */
#define MPLS_LABEL_GAL			13 /* RFC5586 */
#define MPLS_LABEL_OAMALERT		14 /* RFC3429 */
#define MPLS_LABEL_EXTENSION	15 /* RFC7274 */

#define MPLS_LABEL_FIRST_UNRESERVED	16 /* RFC3032 */

#include "R1IQueryResult.h"

#define COLRESULT_FLAGS         0xFF000000
#define FLAG_FILTER             0x10000000

#define DOMAIN_FRAME            0x00000000
#define DOMAIN_ETHER            0x00000100
#define DOMAIN_VLAN             0x00000200
#define DOMAIN_MPLS             0x00000400
#define DOMAIN_IP               0x00000800
#define DOMAIN_IPV6             0x00001000
#define DOMAIN_ICMP             0x00002000
#define DOMAIN_ICMPV6           0x00004000
#define DOMAIN_LAYER4           0x00008000
#define DOMAIN_TCP              0x00010000
#define DOMAIN_UDP              0x00020000
#define DOMAIN_HTTP             0x00040000

#define FRAME_TIME              DOMAIN_FRAME | 1
#define FRAME_NUMBER            DOMAIN_FRAME | 2
#define FRAME_LEN               DOMAIN_FRAME | 3
#define FRAME_PROTOCOLS         DOMAIN_FRAME | 4

#define ETH_DST                 DOMAIN_ETHER | 1
#define ETH_DST_RESOLVED        DOMAIN_ETHER | 2
#define ETH_SRC                 DOMAIN_ETHER | 3
#define ETH_SRC_RESOLVED        DOMAIN_ETHER | 4

#define VLAN_PRIORITY           DOMAIN_VLAN | 1
#define VLAN_CFI                DOMAIN_VLAN | 2
#define VLAN_ID                 DOMAIN_VLAN | 3
#define VLAN_LEN                DOMAIN_VLAN | 4
#define VLAN_ETYPE              DOMAIN_VLAN | 5
#define VLAN_PADDING            DOMAIN_VLAN | 6
#define VLAN_TRAILER            DOMAIN_VLAN | 7

#define MPLS_LABEL              DOMAIN_MPLS | 1
#define MPLS_EXP                DOMAIN_MPLS | 2
#define MPLS_BOTTOM             DOMAIN_MPLS | 3
#define MPLS_TTL                DOMAIN_MPLS | 4

#define IP_DST                  DOMAIN_IP | 1
#define IP_SRC                  DOMAIN_IP | 2
#define IP_GEOIP_SRC_LAT        DOMAIN_IP | 3
#define IP_GEOIP_SRC_LON        DOMAIN_IP | 4
#define IP_GEOIP_SRC_CITY       DOMAIN_IP | 5
#define IP_GEOIP_SRC_COUNTRY    DOMAIN_IP | 6
#define IP_GEOIP_DST_LAT        DOMAIN_IP | 7
#define IP_GEOIP_DST_LON        DOMAIN_IP | 8
#define IP_GEOIP_DST_CITY       DOMAIN_IP | 9
#define IP_GEOIP_DST_COUNTRY    DOMAIN_IP | 10
#define IP_TTL                  DOMAIN_IP | 11

#define IPV6_SRC                DOMAIN_IPV6 | 1
#define IPV6_SRC_SA_MAC         DOMAIN_IPV6 | 2
#define IPV6_DST                DOMAIN_IPV6 | 3
#define IPV6_DST_SA_MAC         DOMAIN_IPV6 | 4

#define ICMP_TYPE               DOMAIN_ICMP | 1
#define ICMP_CODE               DOMAIN_ICMP | 2
#define ICMP_CHECKSUM           DOMAIN_ICMP | 3
#define ICMP_IDENT              DOMAIN_ICMP | 4
#define ICMP_SEQ_LE             DOMAIN_ICMP | 5
#define ICMP_DATA               DOMAIN_ICMP | 6

#define ICMPV6_TYPE             DOMAIN_ICMPV6 | 1
#define ICMPV6_CODE             DOMAIN_ICMPV6 | 2
#define ICMPV6_CHECKSUM         DOMAIN_ICMPV6 | 3
#define ICMPV6_DATA             DOMAIN_ICMPV6 | 4

#define PAYLOAD                 DOMAIN_LAYER4 | 1

#define TCP_SRCPORT             DOMAIN_TCP | 1
#define TCP_DSTPORT             DOMAIN_TCP | 2
#define TCP_LEN                 DOMAIN_TCP | 3
#define TCP_SEQ                 DOMAIN_TCP | 4
#define TCP_ACK                 DOMAIN_TCP | 5
#define TCP_FLAGS_RESET         DOMAIN_TCP | 6
#define TCP_FLAGS_SYN           DOMAIN_TCP | 7
#define TCP_FLAGS_FIN           DOMAIN_TCP | 8

#define UDP_SRCPORT             DOMAIN_UDP | 1
#define UDP_DSTPORT             DOMAIN_UDP | 2
#define UDP_LENGTH              DOMAIN_UDP | 3

#define HTTP_CONTENT_ENCODING   DOMAIN_HTTP | 1
#define HTTP_CONTENT_LENGTH     DOMAIN_HTTP | 2
#define HTTP_CONTENT_TYPE       DOMAIN_HTTP | 3
#define HTTP_REQ_HEADERS        DOMAIN_HTTP | 4
#define HTTP_REQ_METHOD         DOMAIN_HTTP | 5
#define HTTP_REQ_URI            DOMAIN_HTTP | 6
#define HTTP_REQ_HOST           DOMAIN_HTTP | 7
#define HTTP_REQ_REFERER        DOMAIN_HTTP | 8
#define HTTP_REQ_AUTHORIZATION  DOMAIN_HTTP | 9
#define HTTP_REQ_USER_AGENT     DOMAIN_HTTP | 10
#define HTTP_REQ_CONNECTION     DOMAIN_HTTP | 11
#define HTTP_REQ_VERSION        DOMAIN_HTTP | 12
#define HTTP_RES_HEADERS        DOMAIN_HTTP | 13
#define HTTP_RES_CODE           DOMAIN_HTTP | 14
#define HTTP_RES_PHRASE         DOMAIN_HTTP | 15
#define HTTP_RES_WWW_AUTH       DOMAIN_HTTP | 16
#define HTTP_RES_NUMBER         DOMAIN_HTTP | 17 

const char __hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

const string __httpVerbs = "OPTIONS,GET,HEAD,POST,PUT,DELETE,TRACE,CONNECT";

const char whitespace[] = " \f\n\r\t\v";

#define is_whitespace(c) \
    ((c) && strchr(whitespace, (c)))

struct membuf : std::streambuf
{
    membuf(char * begin, size_t len) {
        this->setg(begin, begin, begin + len);
    }
};

inline void my_getline(membuf& in, string&out) {
    out.clear();
    for (char cstr1 = in.sbumpc(); in.in_avail(); cstr1 = in.sbumpc()) {
        if (cstr1 == '\r' || cstr1 == '\n') {
            char cstr2 = in.sbumpc();
            if (cstr2 != '\r' && cstr2 != '\n') 
                in.sungetc();
            out += "\r\n";
            break;
        }
        else 
            out += cstr1;
    }
}

typedef bool (*comparitor)(const struct ether_addr *a1, const struct ether_addr *a2);

typedef struct {
    struct ether_addr _addr;
    unsigned char _sig;
    comparitor _comparitor;
    string _manuf;
} MANUF;
typedef vector<MANUF> MANUFS;

class RyftOne_PCAPResult : public IQueryResult {
public:
    RyftOne_PCAPResult(ILogger *log, string geoipPath, string manufPath) : IQueryResult(log) 
    { 
        __gi = GeoIP_open(geoipPath.c_str(), GEOIP_INDEX_CACHE);
        __loadManufs(manufPath);
    }
   ~RyftOne_PCAPResult() 
    { 
        if(__gi)
            GeoIP_delete(__gi); 
    }

    virtual bool OpenIndexedResult();
    virtual bool CloseIndexedResult();
    virtual bool FetchNextIndexedResult();
    virtual bool IndexedResultEof();

    virtual bool HasResultThinner(string columnName);
    virtual string GetResultThinnerQuery(string columnName, int type);

protected:
    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config);
    virtual IFile * __createFormattedFile(const char *filename);
    virtual string __getFormatString();
    virtual bool __isStructuredType();
    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);

private:

    bool __internalFetch();
    bool __applyColFilter();

    void __loadHttpRequest(char *ptr, size_t len, string& method, string& uri, string& version, map<string, string>& headers);
    int __loadHttpResponse(char *ptr, size_t len, string& version, string& status, map<string, string>& headers);
    void __readHeaders(membuf& httpBuf, map<string, string>& headers);

    void __loadManufs(string& manufPath);
    bool __getManufAddr(char *ptr, size_t len, const struct ether_addr *);

    vector<long> __colQuantity;
    vector<ColFilters> __appliedFilters;

    pcap_t *__pcap;
    char __errbuf[PCAP_ERRBUF_SIZE];

    GeoIP *__gi;
    MANUFS __manufs[256];

    long __httpResponseNum;
};
#endif

