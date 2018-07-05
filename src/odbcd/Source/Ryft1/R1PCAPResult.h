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
#include <netinet/in.h>
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

#include "R1IQueryResult.h"

#define DOMAIN_FRAME        0x0000
#define DOMAIN_ETHER        0x0100
#define DOMAIN_IP           0x0200
#define DOMAIN_LAYER4       0x0400
#define DOMAIN_TCP          0x0800
#define DOMAIN_UDP          0x1000
#define DOMAIN_HTTP         0x2000

#define FRAME_TIME          DOMAIN_FRAME | 1
#define FRAME_NUMBER        DOMAIN_FRAME | 2
#define FRAME_LEN           DOMAIN_FRAME | 3
#define FRAME_PROTOCOLS     DOMAIN_FRAME | 4

#define ETH_DST             DOMAIN_ETHER | 1
#define ETH_DST_RESOLVED    DOMAIN_ETHER | 2
#define ETH_SRC             DOMAIN_ETHER | 3
#define ETH_SRC_RESOLVED    DOMAIN_ETHER | 4

#define IP_DST              DOMAIN_IP | 1
#define IP_SRC              DOMAIN_IP | 2
#define IP_GEOIP_SRC_LAT    DOMAIN_IP | 3
#define IP_GEOIP_SRC_LON    DOMAIN_IP | 4
#define IP_GEOIP_DST_LAT    DOMAIN_IP | 5
#define IP_GEOIP_DST_LON    DOMAIN_IP | 6

#define PAYLOAD             DOMAIN_LAYER4 | 1

#define TCP_SRCPORT         DOMAIN_TCP | 1
#define TCP_DSTPORT         DOMAIN_TCP | 2
#define TCP_LEN             DOMAIN_TCP | 3
#define TCP_SEQ             DOMAIN_TCP | 4
#define TCP_ACK             DOMAIN_TCP | 5

#define UDP_SRCPORT         DOMAIN_UDP | 1
#define UDP_DSTPORT         DOMAIN_UDP | 2
#define UDP_LENGTH          DOMAIN_UDP | 3

#define HTTP_REQ_METHOD     DOMAIN_HTTP | 1
#define HTTP_REQ_URI        DOMAIN_HTTP | 2
#define HTTP_REQ_HEADERS    DOMAIN_HTTP | 3
#define HTTP_REQ_CONNECTION DOMAIN_HTTP | 4

const char __hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

const string __httpVerbs = "OPTIONS,GET,HEAD,POST,PUT,DELETE,TRACE,CONNECT";

const char whitespace[] = " \f\n\r\t\v";

#define is_whitespace(c) \
    ((!c) || strchr(whitespace, (c)))

struct membuf : std::streambuf
{
    membuf(char * begin, size_t len) {
        this->setg(begin, begin, begin + len);
    }
};

class RyftOne_PCAPResult : public IQueryResult {
public:
    RyftOne_PCAPResult(ILogger *log, string geoipPath) : IQueryResult(log) 
    { 
        __gi = GeoIP_open(geoipPath.c_str(), GEOIP_INDEX_CACHE); 
    }
   ~RyftOne_PCAPResult() 
    { 
        if(__gi)
            GeoIP_delete(__gi); 
    }

    virtual bool OpenIndexedResult();
    virtual bool CloseIndexedResult();
    virtual bool FetchNextIndexedResult();

protected:
    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config);
    virtual IFile * __createFormattedFile(const char *filename);
    virtual string __getFormatString();
    virtual bool __isStructuredType();
    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);

private:

    void __loadHttpRequest(char *ptr, size_t len, string& method, string& uri, map<string, string>& headers);

    vector<int> __colQuantity;

    pcap_t *__pcap;
    char __errbuf[PCAP_ERRBUF_SIZE];

    GeoIP *__gi;
};
#endif

