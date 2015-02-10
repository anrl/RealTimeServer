/*
 * callback_video.h
 *
 *  Created on: 2014-04-17
 *      Author: Mingzhou Yang
 */

#ifndef CALLBACK_VIDEO_H_
#define CALLBACK_VIDEO_H_

#include "Capture.h"
#include "imageHash.h"
#include "server.h"


//map<int, string> fdtoip;
//map<int, string> fdtoid;
//int *groupMode = new int[MAX_CLIENT_NUM];

map<int, Peer> PeerTable;
set<int> livefd;
vector<vector<int> > groupTable;
//map<int, vector<int>> groupTable;

unsigned char* imageBuf = new unsigned char[200000];
unsigned char* textBuf = new unsigned char[10000];
int buffSize;
int GROUP_SIZE = 1;
int GROUP_NUM = 0;
int *pos = new int[PIECE_NUM];
int *imageSize = new int[PIECE_NUM];
pthread_mutex_t mutexbuf;

struct timespec currentTime;

void Redistribute(int type, int fd){
//when a new peer joins the group, redistribute pieces to the new peer
	Peer p = PeerTable[fd];
	vector<int> group = groupTable[p.groupNo];
	int size = group.size();
	int mid=1, source;
	for(;mid*2<size;mid*=2) {}
	if (p.groupPos < mid) source = mid + p.groupPos;
	else source = p.groupPos - mid;
	source = groupTable[p.groupNo][source];
	PeerTable[fd].pieceID.clear();
	size = PeerTable[source].pieceID.size();
	for(int j=0;j<size/2;j++){
		PeerTable[fd].pieceID.push_back(PeerTable[source].pieceID.back());
		PeerTable[source].pieceID.pop_back();
	}
//TODO delete redistribution
}

void manageGroup(int type, int fd){
//type = GROUP_INCREMENT, new peer joins
//Only consider the first group right now
	if (type==GROUP_INCREMENT){
		if (groupTable.empty() || (int)livefd.size() == GROUP_NUM*MAX_GROUP_SIZE+1) {
			vector<int> thisGroup;
			thisGroup.push_back(fd);
			PeerTable[fd].groupNo = GROUP_NUM;
			GROUP_NUM ++;
			groupTable.push_back(thisGroup);
		}
		else{
			int pos = (int)groupTable[GROUP_NUM-1].size();
			for(int i=0;i<pos;i++){
				int peer = groupTable[GROUP_NUM-1][i];
//				PeerTable[peer].mode = GROUP_INCREMENT;
//				PeerTable[peer].peerToConnect.push(fd);
				PeerTable[fd].peerToConnect.push(peer);
			}
			PeerTable[fd].mode = GROUP_INCREMENT;
			PeerTable[fd].groupNo = GROUP_NUM-1;
			PeerTable[fd].groupPos = pos;
			groupTable[GROUP_NUM-1].push_back(fd);
			Redistribute(type, fd);
		}
	}
//type = GROUP_DELETE, peer left
	else if (type==GROUP_DELETE){
		int groupNo = PeerTable[fd].groupNo;
		if (groupTable.empty() || groupTable[groupNo].empty()) {cerr<<"Empty deletion error"<<endl;return;}
		groupTable[groupNo][PeerTable[fd].groupPos] = -1;
//Todo:	if (groupIsEmpty(groupTable[groupNo])) server stops transmitting data to this group
		if (!groupIsEmpty(groupTable[groupNo])) Redistribute(type, fd);
	}
}

int makeTextPacket(int type, int fd){
	if (type == GROUP_INCREMENT){
		string packet = "1";
		while(!PeerTable[fd].peerToConnect.empty()){
			int peer = PeerTable[fd].peerToConnect.front();
			packet = packet + PeerTable[peer].id + "#";
			PeerTable[fd].peerToConnect.pop();
		}
		memcpy(textBuf, packet.c_str(), packet.size());
		PeerTable[fd].mode = 0;
		return packet.size()-1;
	}
/*	if (livefd.size()>1){
		for(set<int>::iterator it=livefd.begin();it!=livefd.end();++it){
			if (*it!=fd){
				string packet = "1";
				packet = packet + PeerTable[*it].id + "#";
//				packet = packet + fdtoid[*it] + "#";
//				cout<<*it<<" "<<fdtoid[*it]<<endl;
				memcpy(textBuf, packet.c_str(), packet.size());
				return packet.size()-1;
			}
		}
	}*/
	return 0;
}

static int
callback_video_transfer(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	string addr;
	int length;
	string packet;
	const char * rcvData;
//	unsigned long long timeInterval;

//	int id;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_video_transfer: "
						 "LWS_CALLBACK_ESTABLISHED\n");
//		videoTransfer(context, wsi);
//		imageTransfer(context, wsi);
		addr = getAddress(context, wsi);
		PeerTable[wsi->sock] = Peer();
		PeerTable[wsi->sock].ip = addr;
//		fdtoip[wsi->sock] = addr;
		packet = "0" + string(RTC_KEY);
//		packet = "0" + itoa(wsi->sock) + "#" + RTC_KEY;
		memcpy(textBuf, packet.c_str(), packet.size());
		libwebsocket_write(wsi, textBuf, packet.size(), LWS_WRITE_TEXT);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
//		memcpy(textBuf, PeerTable[wsi->sock].ip.c_str(), PeerTable[wsi->sock].ip.size());
//		libwebsocket_write(wsi, textBuf, PeerTable[wsi->sock].ip.size(), LWS_WRITE_TEXT);
//		if (groupMode[wsi->sock] == GROUP_OVERWRITE){
		if (PeerTable[wsi->sock].mode == GROUP_INCREMENT){
			length = makeTextPacket(GROUP_INCREMENT, wsi->sock);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
		}


//		libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], 101, LWS_WRITE_TEXT);
//		libwebsocket_write(wsi, &imageBuf[LWS_SEND_BUFFER_PRE_PADDING], buffSize, LWS_WRITE_BINARY);
//		cout<<"transfer size "<<libwebsocket_write(wsi, imageBuf, buffSize, LWS_WRITE_BINARY)<<endl;
		for(unsigned int i=0;i<PeerTable[wsi->sock].pieceID.size();i++){
			int id = PeerTable[wsi->sock].pieceID[i];
//			clock_gettime(CLOCK_MONOTONIC, &currentTime);
//			timeInterval = timespecDiff(&currentTime, &PeerTable[wsi->sock].lastSend)/1000;
//			if (timeInterval < WAIT_TIME) usleep(WAIT_TIME - timeInterval);
			//in order not to overwhelm client
			while (lws_send_pipe_choked(wsi)) {}	//in order not to overwhelm server
			libwebsocket_write(wsi, &imageBuf[pos[id]], imageSize[id], LWS_WRITE_BINARY);
//			clock_gettime(CLOCK_MONOTONIC, &PeerTable[wsi->sock].lastSend);
		}

		break;

	case LWS_CALLBACK_RECEIVE:
		rcvData = (const char *)in;
		switch(rcvData[0]){
		//receive client id
		case '0':
//			fdtoid[wsi->sock] = rcvData+1;
			PeerTable[wsi->sock].id = rcvData+1;
			cout<<wsi->sock<<" id: "<<rcvData<<endl;
			livefd.insert(wsi->sock);
			manageGroup(GROUP_INCREMENT, wsi->sock);
			break;
		}
//		cout << (const char *)in << endl;
		break;

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		/* you could return non-zero here and kill the connection */
		break;

	case LWS_CALLBACK_CLOSED:
		PeerTable.erase(wsi->sock);
		manageGroup(GROUP_DELETE, wsi->sock);
//		fdtoip.erase(wsi->sock);
		livefd.erase(wsi->sock);
//		fdtoid.erase(wsi->sock);
		cout<<"fd "<<wsi->sock<<" left"<<endl;
		break;
	default:
		break;
	}

	return 0;
}

#endif /* CALLBACK_VIDEO_H_ */
