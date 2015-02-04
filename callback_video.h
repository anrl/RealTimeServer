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

void manageGroup(int type, int fd){
//type = GROUP_INCREMENT, new peer joins
//Only consider the first group right now
	if (type==GROUP_INCREMENT){
		if (groupTable.empty() || (int)livefd.size() == GROUP_NUM*MAX_GROUP_SIZE) {
			vector<int> thisGroup;
			thisGroup.push_back(fd);
			PeerTable[fd].groupNo = GROUP_NUM;
			GROUP_NUM ++;
			groupTable.push_back(thisGroup);
		}
		else{
			for(int i=0;i<(int)groupTable[GROUP_NUM-1].size();i++){
				int peer = groupTable[GROUP_NUM-1][i];
				PeerTable[peer].mode = GROUP_INCREMENT;
				PeerTable[peer].peerToConnect.push(fd);
//				PeerTable[fd].peerToConnect.push(peer);
			}
			PeerTable[fd].mode = GROUP_CREATE;
			groupTable[GROUP_NUM-1].push_back(fd);
		}
	}
//type = GROUP_DELETE, peer left
	else if (type==GROUP_DELETE){

	}
}

int makeTextPacket(int type, int fd){
	if (livefd.size()>1){
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
	}
/*	int i, j, num=-1;
	if (type==1){
		for(i=0;i<(int)groupTable.size();i++)
			for(j=0;j<(int)groupTable[i].size();j++)
				if (groupTable[i][j]==fd){
					num = i;
					break;
				}
		if (num!=-1){
			string packet = "1";
			for(int j=0;j<(int)groupTable[num].size();j++)
				if (groupTable[num][j]!=fd) {
					packet = packet + itoa(groupTable[num][j]) + "#";
				}
			memcpy(textBuf, packet.c_str(), packet.size());
			return packet.size()-1;
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
	unsigned long long timeInterval;

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
//		clock_gettime(CLOCK_MONOTONIC, &time1);
//		cout << "time: " << timespecDiff(&time1, &time2)/1000000 << endl;
//		clock_gettime(CLOCK_MONOTONIC, &time2);
//		memcpy(textBuf, PeerTable[wsi->sock].ip.c_str(), PeerTable[wsi->sock].ip.size());
//		libwebsocket_write(wsi, textBuf, PeerTable[wsi->sock].ip.size(), LWS_WRITE_TEXT);
//		if (groupMode[wsi->sock] == GROUP_OVERWRITE){
		if (PeerTable[wsi->sock].mode == GROUP_INCREMENT
				|| PeerTable[wsi->sock].mode == GROUP_CREATE){
			length = makeTextPacket(1, wsi->sock);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
			if (!PeerTable[wsi->sock].peerToConnect.empty()) PeerTable[wsi->sock].peerToConnect.pop();
			if (PeerTable[wsi->sock].peerToConnect.empty()) PeerTable[wsi->sock].mode = 0;
		}


//		libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], 101, LWS_WRITE_TEXT);
//		libwebsocket_write(wsi, &imageBuf[LWS_SEND_BUFFER_PRE_PADDING], buffSize, LWS_WRITE_BINARY);
//		cout<<"transfer size "<<libwebsocket_write(wsi, imageBuf, buffSize, LWS_WRITE_BINARY)<<endl;
		for(int i=0;i<PIECE_NUM;i++){
			clock_gettime(CLOCK_MONOTONIC, &currentTime);
			timeInterval = timespecDiff(&currentTime, &PeerTable[wsi->sock].lastSend)/1000;
			if (timeInterval < WAIT_TIME) usleep(WAIT_TIME - timeInterval);
			//in order not to overwhelm client
			while (lws_send_pipe_choked(wsi)) {}	//in order not to overwhelm server
			libwebsocket_write(wsi, &imageBuf[pos[i]], imageSize[i], LWS_WRITE_BINARY);
			clock_gettime(CLOCK_MONOTONIC, &PeerTable[wsi->sock].lastSend);
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
