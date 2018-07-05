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
        else if (!colitr->m_colAlias.compare("ip.geoip.dst_lat")) {
            colQuantity = IP_GEOIP_DST_LAT;
        }
        else if (!colitr->m_colAlias.compare("ip.geoip.dst_lon")) {
            colQuantity = IP_GEOIP_DST_LON;
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
        const struct ip* ipHeader;
        const struct tcphdr* tcpHeader;
        const struct udphdr* udpHeader;
        int payloadLen = 0;
        u_char *payloadData = NULL;
        char addr[INET_ADDRSTRLEN];
        GeoIPRecord *gir;
        etherHeader = (struct ether_header *)pkt;
        bool isIP = (ntohs(etherHeader->ether_type) == ETHERTYPE_IP);
        ipHeader = (struct ip *)(pkt + sizeof(struct ether_header));
        bool isTCP = isIP && (ipHeader->ip_p == IPPROTO_TCP);
        bool isUDP = isIP && (ipHeader->ip_p == IPPROTO_UDP);
        if (isTCP) {
            tcpHeader = (tcphdr *)(pkt + sizeof(struct ether_header) + sizeof(struct ip));
            payloadLen = hdr.len - (sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct tcphdr));
            payloadData = (u_char *)(pkt + (sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct tcphdr)));
        }
        else if (isUDP) {
            udpHeader = (udphdr *)(pkt + sizeof(struct ether_header) + sizeof(struct ip));
            payloadLen = udpHeader->len;
            payloadData = (u_char *)(pkt + (sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct udphdr)));
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
                // fallthrough
            case ETH_SRC:
                snprintf(ptr, len, "%02x:%02x:%02x:%02x:%02x:%02x",
                    etherHeader->ether_shost[0], etherHeader->ether_shost[1], etherHeader->ether_shost[2],
                    etherHeader->ether_shost[3], etherHeader->ether_shost[4], etherHeader->ether_shost[5]);
                break;
            case ETH_DST_RESOLVED:
                if (0 == ether_ntohost(ptr, (const struct ether_addr *)&etherHeader->ether_dhost))
                    break;
                // fallthrough
            case ETH_DST:
                snprintf(ptr, len, "%02x:%02x:%02x:%02x:%02x:%02x",
                    etherHeader->ether_dhost[0], etherHeader->ether_dhost[1], etherHeader->ether_dhost[2],
                    etherHeader->ether_dhost[3], etherHeader->ether_dhost[4], etherHeader->ether_dhost[5]);
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
        method = token;
        token = strtok(NULL, " ");
        if(token)
            uri = token;
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
