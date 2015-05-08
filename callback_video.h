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

map<int, Peer> PeerTable;
set<int> livefd;
vector<vector<int> > groupTable;

unsigned char* imageBuf = new unsigned char[200000];
unsigned char* textBuf = new unsigned char[10000];
int *pos = new int[PIECE_NUM];
int *imageSize = new int[PIECE_NUM];
bool piece_send[PIECE_NUM];
int imageID;

//send pieces in round-robin way
void pieceReorder(){
	for (auto& pair: PeerTable){
		Peer p = pair.second;
		for(unsigned int i=0;i<p.pieceID.size();i++) pair.second.pieceID[i] = (p.pieceID[i]+1)%PIECE_NUM;
	}
}

//find the peer who can split his content or accept more content
int findIndex(int type, vector<int> group, int fd){
	unsigned int val = MAX_GROUP_SIZE / group.size();
	for (int i=0;i<(int)group.size();i++){
		Peer p = PeerTable[group[i]];
		if (type==GROUP_INCREMENT && p.pieceID.size() >= 2*val) return i;
		if (type==GROUP_DELETE && p.pieceID.size() <= val && group[i]!=fd) return i;
	}
	return -1;
}

void Redistribute(int type, int fd){
//when a new peer joins the group, redistribute pieces to the new peer
	if (type == GROUP_MERGE){
		int groupNo = fd;
		int pieceNumPerPeer = MAX_GROUP_SIZE / groupTable[groupNo].size();
		int rest = MAX_GROUP_SIZE % groupTable[groupNo].size();
		int pieceID=0;
		for(unsigned i=0;i<groupTable[groupNo].size();i++){
			int fd = groupTable[groupNo][i];
			PeerTable[fd].pieceID.clear();
			if (i<rest)
				for(int j=0;j<=pieceNumPerPeer;j++) PeerTable[fd].pieceID.push_back(pieceID++);
			else
				for(int j=0;j<pieceNumPerPeer;j++) PeerTable[fd].pieceID.push_back(pieceID++);
		}
	}
	else{
		Peer p = PeerTable[fd];
		vector<int> group = groupTable[p.groupNo];
		int index = findIndex(type, group, fd);
		int source = groupTable[p.groupNo][index];
		if (type == GROUP_INCREMENT){
			PeerTable[fd].pieceID.clear();
			int size = PeerTable[source].pieceID.size()/2;
			for(int j=0;j<size;j++){
				PeerTable[fd].pieceID.push_back(PeerTable[source].pieceID.back());
				PeerTable[source].pieceID.pop_back();
			}
		}
		else if (type == GROUP_DELETE){
			int size = PeerTable[fd].pieceID.size();
			for(int j=0;j<size;j++){
				PeerTable[source].pieceID.push_back(PeerTable[fd].pieceID.back());
				PeerTable[fd].pieceID.pop_back();
			}
		}
		for(unsigned int i=0;i<group.size();i++) {
			cout << group[i] <<": ";
			printVector(PeerTable[groupTable[p.groupNo][i]].pieceID);
		}
	}
}

int findSlot(){
	for(unsigned int i=0;i<groupTable.size();i++)
		if (groupTable[i].size() != MAX_GROUP_SIZE) return i;
	return -1;
}

//manage group when there is peer join or leave
void manageGroup(int type, int fd){
//type = GROUP_INCREMENT, new peer joins
	int groupNo = findSlot();
	if (type==GROUP_INCREMENT){
		//if no group exists or all groups are full
		if (groupNo==-1) {
			vector<int> thisGroup;
			thisGroup.push_back(fd);
			PeerTable[fd].groupNo = groupTable.size();
			groupTable.push_back(thisGroup);
		}
		//when a slot is found
		else{
			int pos = (int)groupTable[groupNo].size();
			for(int i=0;i<pos;i++){
				int peer = groupTable[groupNo][i];
				PeerTable[peer].mode = RESPONSIBILITY_MAP;
//				PeerTable[peer].peerToConnect.push(fd);
				PeerTable[fd].peerToConnect.push(peer);
			}
			PeerTable[fd].mode = GROUP_INCREMENT;
			PeerTable[fd].groupNo = groupNo;
			groupTable[groupNo].push_back(fd);
			Redistribute(type, fd);
		}
	}
//type = GROUP_DELETE, peer left
	else if (type==GROUP_DELETE){
		int groupNo = PeerTable[fd].groupNo;
		for(unsigned int i=0;i<groupTable[groupNo].size();i++) {
			int peerfd = groupTable[groupNo][i];
			PeerTable[peerfd].mode = GROUP_DELETE;
		}
		int pos = -1;
		if (groupTable.empty() || groupTable[groupNo].empty()) {cerr<<"Empty deletion error"<<endl;return;}
		Redistribute(type, fd);
		for(unsigned int i=0;i<groupTable[groupNo].size();i++)
			if (groupTable[groupNo][i] == fd) pos = i;
		if (pos==-1) cerr<<"Can't find fd in group table error"<<endl;
		groupTable[groupNo].erase(groupTable[groupNo].begin()+pos);
		//delete the group if it is empty
		if (groupTable[groupNo].empty()) {
			int size = groupTable.back().size();
			for(int i=0;i<size;i++){
				int peerfd = groupTable.back().back();
				groupTable[groupNo].push_back(peerfd);
				groupTable.back().pop_back();
				PeerTable[peerfd].groupNo = groupNo;
			}
			groupTable.pop_back();
		}
	}
}

//manage group when two groups should be merged into one
void manageGroup(int type, int group1, int group2){
	for(unsigned int i=0;i<groupTable[group2].size();i++){
		int fd = groupTable[group2][i];
		for(unsigned int j=0;j<groupTable[group1].size();j++) PeerTable[fd].peerToConnect.push(groupTable[group1][j]);
//		PeerTable[fd].peerToConnect.insert(PeerTable[fd].peerToConnect.begin(), groupTable[group1].begin(), groupTable[group1].end());
		PeerTable[fd].mode = GROUP_INCREMENT;
	}
	int size = groupTable[group1].size();
	for(int i=0;i<size;i++){
		int fd = groupTable[group1].back();
		groupTable[group2].push_back(fd);
		groupTable[group1].pop_back();
		PeerTable[fd].groupNo = group2;
		PeerTable[fd].mode = RESPONSIBILITY_MAP;
	}
	size = groupTable.back().size();
	for(int i=0;i<size;i++){
		int fd = groupTable.back().back();
		groupTable[group1].push_back(fd);
		groupTable.back().pop_back();
		PeerTable[fd].groupNo = group1;
	}
	groupTable.pop_back();
	Redistribute(type, group2);
}

//check if there are two groups that can be merged into one group
void mergeGroups(){
	if (groupTable.size() < 2) return;
	int min1 = (groupTable[0].size() < groupTable[1].size()) ? 0 : 1,
			min2 = (groupTable[0].size() < groupTable[1].size()) ? 1 : 0;
	for (unsigned int i=2;i<groupTable.size();i++){
		if (groupTable[i].size() < groupTable[min1].size()) {
			min2 = min1;
			min1 = i;
		}
		else if (groupTable[i].size() < groupTable[min2].size()) min2 = i;
	}
	//merge
	if (groupTable[min1].size() + groupTable[min2].size() <= MAX_GROUP_SIZE){
		manageGroup(GROUP_MERGE, min1, min2);
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
	else if (type == PIECE_DUPLICATE){
		string packet;
		textBuf[0] = '2';
		memcpy(textBuf+1, imageBuf+1, 5);
		for(int i=0;i<PIECE_NUM;i++)
			if (!piece_send[i]) packet = packet + itoa(i) + '#';
		if (packet.size()==0) return 0;
		memcpy(textBuf+6, packet.c_str(), packet.size()-1);
		return packet.size() + 6 - 1;
	}
	else if (type == RESPONSIBILITY_MAP){
		int groupNo = PeerTable[fd].groupNo;
		vector<int> group = groupTable[groupNo];
		string packet = itoa(imageID);
		packet = packet + '#' + itoa(fd);
		textBuf[0] = '3';
		for(unsigned int i=0;i<group.size();i++){
			vector<int> pieceID = PeerTable[group[i]].pieceID;
			for(unsigned int j=0;j<pieceID.size();j++){
				packet = packet + '#' + itoa(group[i]) + ':' + itoa(pieceID[j]);
			}
		}
		memcpy(textBuf+1, packet.c_str(), packet.size());
		PeerTable[fd].mode = 0;
		return packet.size() + 1;
	}
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
		manageGroup(GROUP_INCREMENT, wsi->sock);
//		fdtoip[wsi->sock] = addr;
		packet = "0" + string(RTC_KEY);
//		packet = "0" + itoa(wsi->sock) + "#" + RTC_KEY;
//		memcpy(textBuf, packet.c_str(), packet.size());
//		libwebsocket_write(wsi, textBuf, packet.size(), LWS_WRITE_TEXT);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (PeerTable[wsi->sock].mode == GROUP_INCREMENT){
			length = makeTextPacket(GROUP_INCREMENT, wsi->sock);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
			length = makeTextPacket(RESPONSIBILITY_MAP, wsi->sock);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
		}
		else if (PeerTable[wsi->sock].mode == RESPONSIBILITY_MAP || PeerTable[wsi->sock].mode == GROUP_DELETE){
			length = makeTextPacket(RESPONSIBILITY_MAP, wsi->sock);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
		}
		if (!PeerTable[wsi->sock].sendAll){
			for(int i=0;i<PIECE_NUM;i++){
				while (lws_send_pipe_choked(wsi)) {}	//in order not to overwhelm server
				libwebsocket_write(wsi, &imageBuf[pos[i]], imageSize[i], LWS_WRITE_BINARY);
			}
		}
		else {
			length = makeTextPacket(PIECE_DUPLICATE, -1);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
			for(unsigned int i=0;i<PeerTable[wsi->sock].pieceID.size();i++){
				int id = PeerTable[wsi->sock].pieceID[i];
	//			clock_gettime(CLOCK_MONOTONIC, &currentTime);
	//			timeInterval = timespecDiff(&currentTime, &PeerTable[wsi->sock].lastSend)/1000;
	//			if (timeInterval < WAIT_TIME) usleep(WAIT_TIME - timeInterval);
				//in order not to overwhelm client
				if (piece_send[id]){
					while (lws_send_pipe_choked(wsi)) {}	//in order not to overwhelm server
					libwebsocket_write(wsi, &imageBuf[pos[id]], imageSize[id], LWS_WRITE_BINARY);
				}

	//			clock_gettime(CLOCK_MONOTONIC, &PeerTable[wsi->sock].lastSend);
			}
		}
		PeerTable[wsi->sock].sendAll = (PeerTable[wsi->sock].sendAll+1)%10;
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
			break;
		}
//		cout << (const char *)in << endl;
		break;

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		/* you could return non-zero here and kill the connection */
		break;

	case LWS_CALLBACK_CLOSED:
		manageGroup(GROUP_DELETE, wsi->sock);
		PeerTable.erase(wsi->sock);

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
