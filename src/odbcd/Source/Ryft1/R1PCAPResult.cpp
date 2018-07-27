// =================================================================================================
///  @file R1JSONResult.cpp
///
///  Implements the result class for PCAP data searches
///
///  Copyright (C)      Ryft Systems, Inc.
// =================================================================================================

#include "R1PCAPResult.h"

bool RyftOne_PCAPResult::OpenIndexedResult()
{
    __pcap = pcap_open_offline(__idxFile.c_str(), __errbuf);
    if(__pcap == NULL) 
        return false;

    resultCols::iterator itr;
    for (itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
        (*itr).colResult.text = (char *)sqlite3_malloc((*itr).charCols + 1);
    }

    RyftOne_Columns::iterator colitr;
    for (colitr = __cols.begin(); colitr != __cols.end(); colitr++) {
        int colQuantity = 0;
        if (!colitr->m_colAlias.compare("frame.time")) {
            colQuantity = FRAME_TIME;
        }
        else if (!colitr->m_colAlias.compare("frame.len")) {
            colQuantity = FRAME_LEN;
        }
        else if (!colitr->m_colAlias.compare("frame.number")) {
            colQuantity = FRAME_NUMBER;
        }
        else if (!colitr->m_colAlias.compare("frame.protocols")) {
            colQuantity = FRAME_PROTOCOLS;
        }
        else if (!colitr->m_colAlias.compare("eth.src")) {
            colQuantity = ETH_SRC;
        }
        else if (!colitr->m_colAlias.compare("eth.src_resolved")) {
            colQuantity = ETH_SRC_RESOLVED;
        }
        else if (!colitr->m_colAlias.compare("eth.dst")) {
            colQuantity = ETH_DST;
        }
        else if (!colitr->m_colAlias.compare("eth.dst_resolved")) {
            colQuantity = ETH_DST_RESOLVED;
        }
        else if (!colitr->m_colAlias.compare("vlan.priority")) {
            colQuantity = VLAN_PRIORITY;
        }
        else if (!colitr->m_colAlias.compare("vlan.cfi")) {
            colQuantity = VLAN_CFI;
        }
        else if (!colitr->m_colAlias.compare("vlan.id")) {
            colQuantity = VLAN_ID;
        }
        else if (!colitr->m_colAlias.compare("vlan.len")) {
            colQuantity = VLAN_LEN;
        }
        else if (!colitr->m_colAlias.compare("vlan.etype")) {
            colQuantity = VLAN_ETYPE;
        }
        else if (!colitr->m_colAlias.compare("vlan.padding")) {
            colQuantity = VLAN_PADDING;
        }
        else if (!colitr->m_colAlias.compare("vlan.trailer")) {
            colQuantity = VLAN_TRAILER;
        }
        else if (!colitr->m_colAlias.compare("ip.src")) {
            colQuantity = IP_SRC;
        }
        else if (!colitr->m_colAlias.compare("ip.dst")) {
            colQuantity = IP_DST;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.src_lat")) {
            colQuantity = IP_GEOIP_SRC_LAT;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.src_lon")) {
            colQuantity = IP_GEOIP_SRC_LON;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.src_city")) {
            colQuantity = IP_GEOIP_SRC_CITY;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.src_country")) {
            colQuantity = IP_GEOIP_SRC_COUNTRY;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.dst_lat")) {
            colQuantity = IP_GEOIP_DST_LAT;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.dst_lon")) {
            colQuantity = IP_GEOIP_DST_LON;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.dst_city")) {
            colQuantity = IP_GEOIP_DST_CITY;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.dst_country")) {
            colQuantity = IP_GEOIP_DST_COUNTRY;
        }
        else if (!colitr->m_colAlias.compare("payload")) {
            colQuantity = PAYLOAD;
        }
        else if (!colitr->m_colAlias.compare("tcp.srcport")) {
            colQuantity = TCP_SRCPORT;
        }
        else if (!colitr->m_colAlias.compare("tcp.dstport")) {
            colQuantity = TCP_DSTPORT;
        }
        else if (!colitr->m_colAlias.compare("tcp.len")) {
            colQuantity = TCP_LEN;
        }
        else if (!colitr->m_colAlias.compare("tcp.ack")) {
            colQuantity = TCP_ACK;
        }
        else if (!colitr->m_colAlias.compare("tcp.seq")) {
            colQuantity = TCP_SEQ;
        }
        else if (!colitr->m_colAlias.compare("udp.srcport")) {
            colQuantity = UDP_SRCPORT;
        }
        else if (!colitr->m_colAlias.compare("udp.dstport")) {
            colQuantity = UDP_DSTPORT;
        }
        else if (!colitr->m_colAlias.compare("udp.length")) {
            colQuantity = UDP_LENGTH;
        }
        else if (!colitr->m_colAlias.compare("http.request.method")) {
            colQuantity = HTTP_REQ_METHOD;
        }
        else if (!colitr->m_colAlias.compare("http.request.uri")) {
            colQuantity = HTTP_REQ_URI;
        }
        else if (!colitr->m_colAlias.compare("http.request.headers")) {
            colQuantity = HTTP_REQ_HEADERS;
        }
        else if (!colitr->m_colAlias.compare("http.request.connection")) {
            colQuantity = HTTP_REQ_CONNECTION;
        }
        __colQuantity.push_back(colQuantity);
    }
    __idxCurRow = 0;

    return FetchNextIndexedResult();
}

bool RyftOne_PCAPResult::CloseIndexedResult()
{
    pcap_close(__pcap);
    __pcap = NULL;

    resultCols::iterator itr;
    for (itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
        sqlite3_free((*itr).colResult.text);
    }

    return true;
}

bool RyftOne_PCAPResult::FetchNextIndexedResult()
{
    struct pcap_pkthdr hdr;
    const u_char *pkt;

    __idxCurRow++;
    if (pkt = pcap_next(__pcap, &hdr)) {
        const struct ether_header* etherHeader;
        const struct vlan_hdr* vlanHeader;
        const struct ip* ipHeader;
        const struct tcphdr* tcpHeader;
        const struct udphdr* udpHeader;
        unsigned short ethType;
        unsigned short traverse = 0;
        int payloadLen = 0;
        u_char *payloadData = NULL;
        char addr[INET_ADDRSTRLEN];
        GeoIPRecord *gir;

        etherHeader = (struct ether_header *)(pkt + traverse);
        traverse += sizeof(struct ether_header);
        ethType = ntohs(etherHeader->ether_type);
        bool isVLAN = (ethType == ETHERTYPE_VLAN);
        if (isVLAN) {
            vlanHeader = (struct vlan_hdr *)(pkt + traverse);
            traverse += sizeof(struct vlan_hdr);
            ethType = ntohs(vlanHeader->h_vlan_encapsulated_proto);
        }
        bool isIP = (ethType == ETHERTYPE_IP);
        if (isIP) {
            ipHeader = (struct ip *)(pkt + traverse);
            traverse += sizeof(struct ip);
        }

        bool isTCP = isIP && (ipHeader->ip_p == IPPROTO_TCP);
        bool isUDP = isIP && (ipHeader->ip_p == IPPROTO_UDP);
        if (isTCP) {
            tcpHeader = (tcphdr *)(pkt + traverse);
            traverse += sizeof(struct tcphdr);
            payloadLen = hdr.len - traverse;
            payloadData = (u_char *)(pkt + traverse);
        }
        else if (isUDP) {
            udpHeader = (udphdr *)(pkt + traverse);
            traverse += sizeof(struct udphdr);
            payloadLen = udpHeader->len;
            payloadData = (u_char *)(pkt + traverse);
        }
        bool isHTTPReq = isTCP && (ntohs(tcpHeader->dest) == 80 || ntohs(tcpHeader->dest) == 8008 || 
                ntohs(tcpHeader->dest) == 8080);
        string httpMethod;
        string httpURI;
        map<string, string> httpHeaders;
        if (isHTTPReq) {
            __loadHttpRequest((char *)payloadData, payloadLen, httpMethod, httpURI, httpHeaders);
        }
        int idx;
        RyftOne_Columns::iterator itr;
        for (idx = 0, itr = __cols.begin(); itr != __cols.end(); itr++, idx++) {
            char *ptr = __cursor.__row[idx].colResult.text;
            int len = __cursor.__row[idx].charCols;
            *ptr = '\0';
            switch(__colQuantity[idx]) {
            case FRAME_TIME: {
                char tmbuf[64];
                struct tm *ptm = gmtime(&hdr.ts.tv_sec);
                strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", ptm);
                snprintf(ptr, len, "%s.%06ld UTC", tmbuf, hdr.ts.tv_usec);
                break;
            }
            case FRAME_NUMBER:
                snprintf(ptr, len, "%d", __idxCurRow);
                break;
            case FRAME_LEN:
                snprintf(ptr, len, "%d", hdr.len);
                break;
            case FRAME_PROTOCOLS: {
                int s_port = 0;
                int d_port = 0;
                struct servent *_servent;
                string proto = "eth:ethertype";
                if (isVLAN)
                    proto += ":vlan:ethertype";
                if (isIP)
                    proto += ":ip";
                if (isTCP) {
                    proto += ":tcp";
                    s_port = tcpHeader->source;
                    d_port = tcpHeader->dest;
                }
                if (isUDP) {
                    proto += ":udp";
                    s_port = udpHeader->source;
                    d_port = udpHeader->dest;
                }
                if (s_port != 0 && d_port != 0) {
                    _servent = getservbyport(s_port, NULL);
                    if (_servent == NULL)
                        _servent = getservbyport(d_port, NULL);
                    if (_servent != NULL) {
                        proto += ":";
                        proto += _servent->s_name;
                    }
                    endservent();
                }
                snprintf(ptr, len, "%s", proto.c_str());
                break;
            }
            case ETH_SRC_RESOLVED:
                if(0 == ether_ntohost(ptr, (const struct ether_addr *)&etherHeader->ether_shost))
                    break;
                if (__getManufAddr(ptr, len, (const struct ether_addr *)&etherHeader->ether_shost))
                    break;
                // fallthrough
            case ETH_SRC:
                snprintf(ptr, len, "%02x:%02x:%02x:%02x:%02x:%02x",
                    etherHeader->ether_shost[0], etherHeader->ether_shost[1], etherHeader->ether_shost[2],
                    etherHeader->ether_shost[3], etherHeader->ether_shost[4], etherHeader->ether_shost[5]);
                break;
            case ETH_DST_RESOLVED:
                if (0 == ether_ntohost(ptr, (const struct ether_addr *)&etherHeader->ether_dhost))
                    break;
                if (__getManufAddr(ptr, len, (const struct ether_addr *)&etherHeader->ether_dhost))
                    break;
                // fallthrough
            case ETH_DST:
                snprintf(ptr, len, "%02x:%02x:%02x:%02x:%02x:%02x",
                    etherHeader->ether_dhost[0], etherHeader->ether_dhost[1], etherHeader->ether_dhost[2],
                    etherHeader->ether_dhost[3], etherHeader->ether_dhost[4], etherHeader->ether_dhost[5]);
                break;
            case VLAN_PRIORITY:
                if (isVLAN) {
                    unsigned short us;
                    us = ntohs(vlanHeader->h_vlan_TCI);
                    us >>= 13;
                    // First 3 bits of the TCI are the priority
                    snprintf(ptr, len, "%d", us);
                }
                break;
            case VLAN_CFI:
                if (isVLAN) {
                    unsigned short us;
                    us = ntohs(vlanHeader->h_vlan_TCI);
                    // 4th bit of the TCI is the CFI flag
                    snprintf(ptr, len, "%d", (us & 0x1000) ? 1 : 0);
                }
                break;
            case VLAN_ID:
                if (isVLAN) {
                    unsigned short us;
                    us = ntohs(vlanHeader->h_vlan_TCI);
                    // ID is last 12 bits of the TCI
                    snprintf(ptr, len, "%d", (us & 0xFFF));
                }
                break;
            case VLAN_LEN:
                break;
            case VLAN_ETYPE:
                if (isVLAN)
                    snprintf(ptr, len, "%d", ethType);
                break;
            case VLAN_PADDING:
            case VLAN_TRAILER:
                break;
            case IP_SRC:
                if (isIP)
                    inet_ntop(AF_INET, &(ipHeader->ip_src), ptr, min(INET_ADDRSTRLEN, len));
                break;
            case IP_DST:
                if (isIP)
                    inet_ntop(AF_INET, &(ipHeader->ip_dst), ptr, min(INET_ADDRSTRLEN, len));
                break;
            case IP_GEOIP_SRC_LAT:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_src), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%f", gir->latitude);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_SRC_LON:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_src), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%f", gir->longitude);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_SRC_CITY:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_src), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%s, %s", gir->city, gir->region);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_SRC_COUNTRY:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_src), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%s", gir->country_name);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_DST_LAT:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_dst), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%f", gir->latitude);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_DST_LON:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_dst), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%f", gir->longitude);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_DST_CITY:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_dst), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%s, %s", gir->city, gir->region);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case IP_GEOIP_DST_COUNTRY:
                if (isIP && __gi) {
                    inet_ntop(AF_INET, &(ipHeader->ip_dst), addr, INET_ADDRSTRLEN);
                    gir = GeoIP_record_by_name(__gi, (const char *)addr);
                    if (gir != NULL) {
                        snprintf(ptr, len, "%s", gir->country_name);
                        GeoIPRecord_delete(gir);
                    }
                }
                break;
            case PAYLOAD: {
                if (payloadLen) {
                    string payload(payloadLen, '.');
                    for (int i = 0; i < payloadLen; i++) {
                        if ((payloadData[i] >= 0x20) && payloadData[i] != 0x7F)
                            payload[i] = payloadData[i];
                    }
                    snprintf(ptr, len, "%s", payload.c_str());
                }
                break;
            }
            case TCP_SRCPORT:
                if (isTCP)
                    snprintf(ptr, len, "%d", ntohs(tcpHeader->source));
                break;
            case TCP_DSTPORT:
                if (isTCP)
                    snprintf(ptr, len, "%d", ntohs(tcpHeader->dest));
                break;
            case TCP_LEN:
                if (isTCP)
                    snprintf(ptr, len, "%d", payloadLen);
                break;
            case TCP_ACK:
                if (isTCP)
                    snprintf(ptr, len, "%ud", ntohl(tcpHeader->ack_seq));
                break;
            case TCP_SEQ:
                if (isTCP)
                    snprintf(ptr, len, "%ud", ntohl(tcpHeader->seq));
                break;
            case UDP_SRCPORT:
                if (isUDP)
                    snprintf(ptr, len, "%d", ntohs(udpHeader->source));
                break;
            case UDP_DSTPORT:
                if (isUDP)
                    snprintf(ptr, len, "%d", ntohs(udpHeader->dest));
                break;
            case UDP_LENGTH:
                if (isUDP)
                    snprintf(ptr, len, "%d", ntohs(udpHeader->len));
                break;
            case HTTP_REQ_METHOD:
                snprintf(ptr, len, "%s", httpMethod.c_str());
                break;
            case HTTP_REQ_URI:
                snprintf(ptr, len, "%s", httpURI.c_str());
                break;
            case HTTP_REQ_HEADERS: {
                string headers;
                map<string, string>::iterator itr;
                for (itr = httpHeaders.begin(); itr != httpHeaders.end(); itr++) {
                    headers += itr->first;
                    headers += ": ";
                    headers += itr->second;
                    headers += "\n";
                }
                snprintf(ptr, len, "%s", headers.c_str());
                break;
            }
            case HTTP_REQ_CONNECTION:
                if (httpHeaders.find("Connection") != httpHeaders.end())
                    snprintf(ptr, len, "%s", httpHeaders["Connection"].c_str());
                break;
            }
        }
    }
    return true;
}

RyftOne_Columns RyftOne_PCAPResult::__getColumns(__meta_config__ meta_config)
{
	int idx;
	RyftOne_Column col;
	RyftOne_Columns cols;
	vector<__meta_config__::__meta_col__>::iterator colItr;
	for (idx = 0, colItr = meta_config.columns.begin(); colItr != meta_config.columns.end(); colItr++, idx++) {
		col.m_ordinal = idx + 1;
		col.m_tableName = meta_config.table_name;
		col.m_colAlias = colItr->json_or_xml_tag;
		col.m_colName = colItr->name;
		col.m_typeName = colItr->type_def;
		RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
		col.m_description = colItr->description;
		cols.push_back(col);
	}
	return cols;
}

IFile *RyftOne_PCAPResult::__createFormattedFile(const char *filename)
{
	return new JSONFile(filename, __no_top);
}

string RyftOne_PCAPResult::__getFormatString()
{
	return "pcap";
}

bool RyftOne_PCAPResult::__isStructuredType()
{
	return true;
}

void RyftOne_PCAPResult::__loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
{
	size_t idx1;
	string path;
	glob_t glob_results;
	char **relpaths;
	vector<__meta_config__::__meta_col__>::iterator colItr;

	__name = in_name;
	__path = in_itr->__path;

	path = __path;
	path += "/";
	path += in_itr->meta_config.file_glob;

	idx1 = in_itr->meta_config.file_glob.find(".");
	if (idx1 != string::npos)
		__extension = in_itr->meta_config.file_glob.substr(idx1 + 1);

	glob(path.c_str(), GLOB_TILDE, NULL, &glob_results);
	relpaths = new char *[glob_results.gl_pathc];
	for (int i = 0; i < glob_results.gl_pathc; i++) {
		__glob.push_back(glob_results.gl_pathv[i]);
	}
	globfree(&glob_results);

	__no_top = in_itr->meta_config.no_top;

	for (colItr = in_itr->meta_config.columns.begin(); (colItr != in_itr->meta_config.columns.end()); colItr++)
		__metaTags.push_back(colItr->json_or_xml_tag);

	__delimiter = in_itr->meta_config.delimiter;
}

void RyftOne_PCAPResult::__loadHttpRequest(char *ptr, size_t len, string& method, string& uri, map<string, string>& headers)
{
    membuf httpBuf(ptr, len);
    istream in(&httpBuf);
    string reqLine;
    char *dup;
    char *token;
    getline(in, reqLine);
    if (!reqLine.empty()) {
        dup = strdup(reqLine.c_str());
        token = strtok(dup, " ");
        if (token) {
            method = token;
            token = strtok(NULL, " ");
            if (token)
                uri = token;
        }
        free(dup);
    }
    if (method.empty() || (__httpVerbs.find(method) == string::npos))
        return;
    if (uri.empty())
        return;
    string header;
    while (getline(in, header)) {
        size_t idx;
        if ((idx = header.rfind('\r')) != string::npos)
            header = header.substr(0, idx);
        if (header.empty())
            break;
        dup = strdup(header.c_str());
        token = strtok(dup, ":");
        token = strtok(NULL, "");
        while (is_whitespace(*token))
            token++;
        if(token)
            headers[dup] = token;
        free(dup);
    }
}

void insert_in_list(MANUF& manuf, MANUFS& list)
{
    // insert from most significant to least significant
    MANUFS::iterator itr;
    for (itr = list.begin(); itr != list.end(); itr++) {
        if (manuf._sig > itr->_sig) {
            list.insert(itr, manuf);
            return;
        }
    }
    list.push_back(manuf);
}

bool comparitor16(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]))
        return true;

    return false;
}

bool comparitor24(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]))
        return true;

    return false;
}

bool comparitor25(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        ((a1->ether_addr_octet[3] & 0x80) == (a2->ether_addr_octet[3] & 0x80)))
        return true;

    return false;
}

bool comparitor28(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        ((a1->ether_addr_octet[3] & 0xF0) == (a2->ether_addr_octet[3] & 0xF0)))
        return true;

    return false;
}

bool comparitor32(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        (a1->ether_addr_octet[3] == a2->ether_addr_octet[3]))
        return true;

    return false;
}

bool comparitor36(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        (a1->ether_addr_octet[3] == a2->ether_addr_octet[3]) &&
        ((a1->ether_addr_octet[4] & 0xF0) == (a2->ether_addr_octet[4] & 0xF0)))
        return true;

    return false;
}

bool comparitor40(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        (a1->ether_addr_octet[3] == a2->ether_addr_octet[3]) &&
        (a1->ether_addr_octet[4] == a2->ether_addr_octet[4]))
        return true;

    return false;
}

bool comparitor44(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        (a1->ether_addr_octet[3] == a2->ether_addr_octet[3]) &&
        (a1->ether_addr_octet[4] == a2->ether_addr_octet[4]) &&
        ((a1->ether_addr_octet[5] & 0xF0) == (a2->ether_addr_octet[5] & 0xF0)))
        return true;

    return false;
}

bool comparitor45(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        (a1->ether_addr_octet[3] == a2->ether_addr_octet[3]) &&
        (a1->ether_addr_octet[4] == a2->ether_addr_octet[4]) &&
        ((a1->ether_addr_octet[5] & 0xF8) == (a2->ether_addr_octet[5] & 0xF8)))
        return true;

    return false;
}

bool comparitor48(const struct ether_addr *a1, const struct ether_addr *a2)
{
    if ((a1->ether_addr_octet[0] == a2->ether_addr_octet[0]) &&
        (a1->ether_addr_octet[1] == a2->ether_addr_octet[1]) &&
        (a1->ether_addr_octet[2] == a2->ether_addr_octet[2]) &&
        (a1->ether_addr_octet[3] == a2->ether_addr_octet[3]) &&
        (a1->ether_addr_octet[4] == a2->ether_addr_octet[4]) &&
        (a1->ether_addr_octet[5] == a2->ether_addr_octet[5]))
        return true;

    return false;
}

void RyftOne_PCAPResult::__loadManufs(string& manufPath)
{
    char readfile[256];
    char eth[128];
    char manufacturer[128];
    char *pstr;
    MANUF manuf;

    FILE *manufFile = fopen(manufPath.c_str(), "r");
    if (manufFile == NULL)
        return;

    while (fgets(readfile, sizeof(readfile), manufFile) != NULL) {

        pstr = readfile;
        while (is_whitespace(*pstr))
            pstr++;
        if (!*pstr)
            continue;
        if (*pstr == '#')
            continue;
        sscanf(pstr, "%s\t%s", eth, manufacturer);

        int sig = 24;
        memset(&manuf._addr, 0, sizeof(struct ether_addr));
        int elements = sscanf(eth, "%x:%x:%x:%x:%x:%x/%d",
            &manuf._addr.ether_addr_octet[0], &manuf._addr.ether_addr_octet[1], &manuf._addr.ether_addr_octet[2],
            &manuf._addr.ether_addr_octet[3], &manuf._addr.ether_addr_octet[4], &manuf._addr.ether_addr_octet[5],
            &sig);
        if (elements == 3 || elements == 7) {
            // matched pattern xx:xx:xx OR xx:xx:xx:xx:xx:xx/sig 
            manuf._sig = sig;
        }
        else {
            elements = sscanf(eth, "%x-%x-%x-%x-%x-%x/%d",
                &manuf._addr.ether_addr_octet[0], &manuf._addr.ether_addr_octet[1], &manuf._addr.ether_addr_octet[2],
                &manuf._addr.ether_addr_octet[3], &manuf._addr.ether_addr_octet[4], &manuf._addr.ether_addr_octet[5],
                &sig);
            if (elements == 3) {
                // matched pattern xx-xx-xx
                manuf._sig = 24;
            }
            else if (elements == 6) {
                // matched pattern xx-xx-xx-xx-xx-xx
                manuf._sig = 48;
            }
            else if (elements == 7) {
                // matched pattern xx-xx-xx-xx-xx-xx/sig
                manuf._sig = sig;
            }
            else {
                elements = sscanf(eth, "%x-%x-%x-%x-%x/%d",
                    &manuf._addr.ether_addr_octet[0], &manuf._addr.ether_addr_octet[1], &manuf._addr.ether_addr_octet[2],
                    &manuf._addr.ether_addr_octet[3], &manuf._addr.ether_addr_octet[4], &sig);

                if (elements == 6) {
                    // matched pattern xx-xx-xx-xx-xx/sig
                    manuf._sig = sig;
                }
                else {
                    elements = sscanf(eth, "%x-%x-%x/%d",
                        &manuf._addr.ether_addr_octet[0], &manuf._addr.ether_addr_octet[1], &manuf._addr.ether_addr_octet[2],
                        &sig);

                    if (elements == 4) {
                        // matched pattern xx-xx-xx/sig
                        manuf._sig = sig;
                    }
                    else
                        continue;
                }
            }
        }
        manuf._manuf = manufacturer;
        switch (manuf._sig) {
        case 16:
            manuf._comparitor = comparitor16;
            break;
        case 24:
            manuf._comparitor = comparitor24;
            break;
        case 25:
            manuf._comparitor = comparitor25;
            break;
        case 28:
            manuf._comparitor = comparitor28;
            break;
        case 32:
            manuf._comparitor = comparitor32;
            break;
        case 36:
            manuf._comparitor = comparitor36;
            break;
        case 40:
            manuf._comparitor = comparitor40;
            break;
        case 44:
            manuf._comparitor = comparitor44;
            break;
        case 45:
            manuf._comparitor = comparitor45;
            break;
        case 48:
            manuf._comparitor = comparitor48;
            break;
        default:
            DEBUG_LOG(__log, "RyftOne", "RyftOne_PCAPResult", "__loadManufs", "Count not find a comparitor for %d bits", manuf._sig);
            break;
        }

        int idx = (manuf._addr.ether_addr_octet[0] + manuf._addr.ether_addr_octet[1]) % 0xFF;
        insert_in_list(manuf, __manufs[idx]);
    }

    fclose(manufFile);
}

bool RyftOne_PCAPResult::__getManufAddr(char *ptr, size_t len, const struct ether_addr *a1)
{
    int idx = (a1->ether_addr_octet[0] + a1->ether_addr_octet[1]) % 0xFF;
    MANUFS::iterator itr;
    for (itr = __manufs[idx].begin(); itr != __manufs[idx].end(); itr++) {
        if (itr->_comparitor(a1, &itr->_addr)) {
            switch (itr->_sig) {
            case 16:
                snprintf(ptr, len, "%s_%02x:%02x:%02x:%02x",
                    itr->_manuf.c_str(), a1->ether_addr_octet[2], a1->ether_addr_octet[3], a1->ether_addr_octet[4],
                    a1->ether_addr_octet[5]);
                break;
            case 24:
                snprintf(ptr, len, "%s_%02x:%02x:%02x",
                    itr->_manuf.c_str(), a1->ether_addr_octet[3], a1->ether_addr_octet[4], a1->ether_addr_octet[5]);
                break;
            case 25:
                snprintf(ptr, len, "%s_%02x:%02x:%02x",
                    itr->_manuf.c_str(), a1->ether_addr_octet[3] & 0x7F, a1->ether_addr_octet[4], a1->ether_addr_octet[5]);
                break;
            case 28:
                snprintf(ptr, len, "%s_%01x:%02x:%02x",
                    itr->_manuf.c_str(), a1->ether_addr_octet[3] & 0x0F, a1->ether_addr_octet[4], a1->ether_addr_octet[5]);
                break;
            case 32:
                snprintf(ptr, len, "%s_%02x:%02x", itr->_manuf.c_str(), a1->ether_addr_octet[4], a1->ether_addr_octet[5]);
                break;
            case 36:
                snprintf(ptr, len, "%s_%01x:%02x", itr->_manuf.c_str(), a1->ether_addr_octet[4] & 0x0F, a1->ether_addr_octet[5]);
                break;
            case 40:
                snprintf(ptr, len, "%s_%02x", itr->_manuf.c_str(), a1->ether_addr_octet[5]);
                break;
            case 44:
                snprintf(ptr, len, "%s_%01x", itr->_manuf.c_str(), a1->ether_addr_octet[5] & 0x0F);
                break;
            case 45:
                snprintf(ptr, len, "%s_%01x", itr->_manuf.c_str(), a1->ether_addr_octet[5] & 0x07);
                break;
            case 48:
                snprintf(ptr, len, "%s", itr->_manuf.c_str());
                break;
            }
            return true;
        }
    }
    return false;
}
