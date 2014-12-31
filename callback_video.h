/*
 * callback_video.h
 *
 *  Created on: 2014-04-17
 *      Author: frye
 */


/* dumb_increment protocol */

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 * for this example protocol we use it to individualize the count for each
 * connection.
 */

#ifndef CALLBACK_VIDEO_H_
#define CALLBACK_VIDEO_H_

#include "Capture.h"
#include "imageHash.h"
#include "server.h"

map<int, string> fdtoip;
set<int> livefd;
vector<vector<int> > groupTable;
//map<int, vector<int>> groupTable;

unsigned char* imageBuf = new unsigned char[200000];
unsigned char* textBuf = new unsigned char[10000];
int buffSize;
int GROUP_SIZE = 1;
int *pos = new int[MAX_GROUP_SIZE];
int *imageSize = new int[MAX_GROUP_SIZE];
pthread_mutex_t mutexbuf;

enum groupManage{
	GROUP_OVERWRITE = 1,
	GROUP_DELETE,
	GROUP_INCREMENT
};
int *groupMode = new int[MAX_CLIENT_NUM];

void imageTransfer(struct libwebsocket_context *context, struct libwebsocket *wsi){
	FILE * pFile;
	int i=0;
	char temp[2];

	pFile = fopen ("4.jpg" , "r");

	if (pFile == NULL) perror ("Error opening file");
	else
	{
		while ( ! feof (pFile) )
		{
			if ( fgets (temp , 2 , pFile) == NULL ) break;
			imageBuf[i] = temp[0];
		    i++;
		}
		imageBuf[i] = '\0';
		fclose (pFile);
	}
	buffSize = i;
/*	while(1) {
		libwebsocket_write(wsi, imageBuf, i, LWS_WRITE_BINARY);
		usleep(50000);
	}*/
}

void *storeVideo(){
	VideoCapture camera;
	vector<uchar> imageVec;
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);
	camera.open(0);
	if(!camera.isOpened()) {
	    std::cerr<<"Error opening camera 0"<<std::endl;
	    camera.open(1);
	    if(!camera.isOpened())
	    	exit(1);
	}

	pthread_mutex_init(&mutexbuf, NULL);

	while(true) {
		Mat frame;
		camera>>frame;
		if(!imencode(".jpg", frame, imageVec, compression_params)) printf("Write error\n");
		string temp(imageVec.begin(), imageVec.end());
		pthread_mutex_lock (&mutexbuf);
		imageBuf = (unsigned char*)temp.c_str();
		pthread_mutex_unlock (&mutexbuf);
		buffSize = temp.size();
		usleep(100000);
	}
	pthread_mutex_destroy(&mutexbuf);
}

void videoTransfer(struct libwebsocket_context *context, struct libwebsocket *wsi){
	VideoCapture camera;
	vector<uchar> imageVec;
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);
	camera.open(0);
	if(!camera.isOpened()) {
	    std::cerr<<"Error opening camera"<<std::endl;
	    exit(1);
	}
	while(true) {
		Mat frame;
		camera>>frame;
		if(!imencode(".jpg", frame, imageVec, compression_params)) printf("Write error\n");
		buffSize = imageVec.size();
//		unsigned char *temp = imageVec.data();
		for(int i=0;i<buffSize;i++) imageBuf[i] = imageVec[i];

		libwebsocket_write(wsi, imageBuf, buffSize, LWS_WRITE_BINARY);

	}
}

void manageGroup(int type){
	if (livefd.size() <= 1) return;
	if (type==1){
		groupTable.clear();
		vector<int> thisGroup;
		int counter=0;
		for(set<int>::iterator it=livefd.begin();it!=livefd.end();++it){
			thisGroup.push_back(*it);
			counter++;
			if (counter==GROUP_SIZE){
				counter = 0;
				groupTable.push_back(thisGroup);
				thisGroup.clear();
			}
			groupMode[*it] = GROUP_OVERWRITE;
		}
	}
}

int makeTextPacket(int type, int fd){
	if (livefd.size()>1){
		for(set<int>::iterator it=livefd.begin();it!=livefd.end();++it){
			if (*it!=fd){
				string packet = "1";
				packet = packet + itoa(*it) + "#";
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
	if (wsi!=NULL){
		addr = fdtoip[wsi->sock];
	}



//	int id;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_video_transfer: "
						 "LWS_CALLBACK_ESTABLISHED\n");
//		videoTransfer(context, wsi);
//		imageTransfer(context, wsi);
		addr = getAddress(context, wsi);
		fdtoip[wsi->sock] = addr;
		cout<<"addr is "<<addr<<" fd is "<<wsi->sock<<endl;
		livefd.insert(wsi->sock);
		manageGroup(1);
		packet = "0" + itoa(wsi->sock) + "#" + RTC_KEY;
		memcpy(textBuf, packet.c_str(), packet.size());
		libwebsocket_write(wsi, textBuf, packet.size(), LWS_WRITE_TEXT);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
//		memcpy(textBuf, addr.c_str(), addr.size());
//		libwebsocket_write(wsi, textBuf, addr.size(), LWS_WRITE_TEXT);
		if (groupMode[wsi->sock] == GROUP_OVERWRITE){
			length = makeTextPacket(1, wsi->sock);
			if (length) libwebsocket_write(wsi, textBuf, length, LWS_WRITE_TEXT);
			groupMode[wsi->sock] = 0;
		}


//		libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], 101, LWS_WRITE_TEXT);
//		libwebsocket_write(wsi, &imageBuf[LWS_SEND_BUFFER_PRE_PADDING], buffSize, LWS_WRITE_BINARY);
//		cout<<"transfer size "<<libwebsocket_write(wsi, imageBuf, buffSize, LWS_WRITE_BINARY)<<endl;
		libwebsocket_write(wsi, &imageBuf[pos[wsi->sock%GROUP_SIZE]], imageSize[wsi->sock%GROUP_SIZE], LWS_WRITE_BINARY);


		break;

	case LWS_CALLBACK_RECEIVE:
//		cout<<libwebsockets_remaining_packet_payload(wsi)<<endl;
		cout << (const char *)in << endl;
		break;
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		/* you could return non-zero here and kill the connection */
		break;

	case LWS_CALLBACK_CLOSED:
		fdtoip.erase(wsi->sock);
		livefd.erase(wsi->sock);
		cout<<"fd "<<wsi->sock<<" left"<<endl;
		break;
	default:
		break;
	}

	return 0;
}

#endif /* CALLBACK_VIDEO_H_ */
