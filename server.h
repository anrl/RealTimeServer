/*
 * server.h
 *
 *  Created on: 2014-04-01
 *      Author: frye
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <syslog.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <queue>
#include <set>

#include "libwebsockets.h"
#include "private-libwebsockets.h"

using namespace std;

#define LOCAL_RESOURCE_PATH "/home/frye/workspace/tryCV"
#define MAX_GROUP_SIZE 8
#define MAX_CLIENT_NUM 256
#define HEADER_LENGTH 8
#define RTC_KEY "1ps524qi2fdy4x6r"
#define KEY_LEN 16
#define PIECE_NUM 8
#define WAIT_TIME 0

char resource_path[] = LOCAL_RESOURCE_PATH;


enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,
	PROTOCOL_VIDEO_TRANSFER,

	/* always last */
	DEMO_PROTOCOL_COUNT
};

/*
 * We take a strict whitelist approach to stop ../ attacks
 */

struct serveable {
	const char *urlpath;
	const char *mimetype;
};

struct per_session_data__http {
	int fd;
};

/*
 * this is just an example of parsing handshake headers, you don't need this
 * in your code unless you will filter allowing connections by the header
 * content
 */

static void
dump_handshake_info(struct libwebsocket *wsi)
{
	unsigned int n;
	static const char *token_names[] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_POST_URI]		=*/ "POST URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",

		"Accept:",
		"If-Modified-Since:",
		"Accept-Encoding:",
		"Accept-Language:",
		"Pragma:",
		"Cache-Control:",
		"Authorization:",
		"Cookie:",
		"Content-Length:",
		"Content-Type:",
		"Date:",
		"Range:",
		"Referer:",
		"Uri-Args:",

		/*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
	};
	char buf[256];

	for (n = 0; n < sizeof(token_names) / sizeof(token_names[0]); n++) {
		if (!lws_hdr_total_length(wsi, (lws_token_indexes)n))
			continue;

		lws_hdr_copy(wsi, buf, sizeof buf, (lws_token_indexes)n);

		fprintf(stderr, "    %s = %s\n", token_names[n], buf);
	}
}

const char * get_mimetype(const char *file)
{
	int n = strlen(file);

	if (n < 5)
		return NULL;

	if (!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";

	if (!strcmp(&file[n - 4], ".png"))
		return "image/png";

	if (!strcmp(&file[n - 4], ".jpg"))
			return "image/jpg";

	if (!strcmp(&file[n - 5], ".html"))
		return "text/html";

	return NULL;
}

struct per_session_data__dumb_increment {
	int number;
};


/* lws-mirror_protocol */

#define MAX_MESSAGE_QUEUE 32


unsigned int getPort(struct libwebsocket_context *context,
	struct libwebsocket *wsi, int fd)
{
	socklen_t len;
#ifdef LWS_USE_IPV6
	struct sockaddr_in6 sin6;
#endif
	struct sockaddr_in sin4;

	int ret = -1;

	lws_latency_pre(context, wsi);

#ifdef LWS_USE_IPV6
	if (LWS_IPV6_ENABLED(context)) {
		len = sizeof(sin6);
		if (getpeername(fd, (struct sockaddr *) &sin6, &len) < 0) {
			lwsl_warn("getpeername: %s\n", strerror(LWS_ERRNO));
			goto bail;
		}
	} else
#endif
	{
		len = sizeof(sin4);
		if (getpeername(fd, (struct sockaddr *) &sin4, &len) < 0) {
			lwsl_warn("getpeername: %s\n", strerror(LWS_ERRNO));
			goto bail;
		}
	}

bail:
	lws_latency(context, wsi, "libwebsockets_get_peer_addresses", ret, 1);
#ifdef LWS_USE_IPV6
	if (true) return sin6.sin6_port; else
#endif
	return sin4.sin_port;
}

string getAddress(struct libwebsocket_context *context, struct libwebsocket *wsi){
	char *name = new char[100];
	char *rip = new char[100];
	libwebsockets_get_peer_addresses(context, wsi, wsi->sock, name, 100, rip, 100);
	int port = getPort(context, wsi, wsi->sock);
	stringstream ss;
	string ret, portnum;
	ss << rip;
	ss << ":";
	ss << port;
	ss >> ret;
	return ret;
}

string itoa(int num){
	string ret;
	stringstream ss;
	ss << num;
	ss >> ret;
	return ret;
}

template <typename T>
void printVector(vector<T> v){
	for(unsigned int i=0;i<v.size();i++){
		cout<<v[i]<<" ";
	}
	cout<<endl;
}

template <typename T>
void printVecVector(vector<vector<T> > v){
	for(unsigned int i=0;i<v.size();i++) printVector(v[i]);
}

struct Peer{
	string ip;
	string id;
	int groupNo;
	queue<int> peerToConnect;
	int mode;
	Peer():groupNo(0), mode(0) {
		for(int i=0;i<PIECE_NUM;i++) pieceID.push_back(i);
	}
	vector<int> pieceID;
	struct timespec lastSend;
};


enum groupManage{
	GROUP_INCREMENT = 1,
	GROUP_CREATE,
	GROUP_DELETE,
	GROUP_OVERWRITE
};

#endif /* SERVER_H_ */
