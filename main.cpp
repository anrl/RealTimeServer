/*
 * main.cpp
 *
 *  Created on: 2014-04-07
 *      Author: frye
 */
#define EXTERNAL_POLL

#include "callback_http.h"
#include "callback_video.h"
#include "http.h"

int max_poll_elements;

struct pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
//extern unsigned char* imageBuf;
//extern int *pos;
//extern int *imageSize;

static volatile int force_exit = 0;
static struct libwebsocket_context *context;



/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
		"callback_video_transfer",
		callback_video_transfer,
		0,
		100,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

void sighandler(int sig)
{
	force_exit = 1;
	libwebsocket_cancel_service(context);
}

int main(int argc, char **argv)
{
	char cert_path[1024];
	char key_path[1024];
	int n = 0;
	int use_ssl = 0;
	int opts = 0;
	const char *iface = NULL;
#ifndef WIN32
	int syslog_options = LOG_PID | LOG_PERROR;
#endif
	unsigned int oldus = 0;
	struct lws_context_creation_info info;

	int debug_level = 7;
#ifndef LWS_NO_DAEMONIZE
	int daemonize = 0;
#endif

	memset(&info, 0, sizeof info);
	info.port = 9002;

#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
	/*
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
	if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
		fprintf(stderr, "Failed to daemonize\n");
		return 1;
	}
#endif

	signal(SIGINT, sighandler);

#ifndef WIN32
	/* we will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", syslog_options, LOG_DAEMON);
#endif

	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);

	lwsl_notice("libwebsockets test server - "
			"(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> - "
						    "licensed under LGPL2.1\n");
#ifdef EXTERNAL_POLL
	max_poll_elements = getdtablesize();
	pollfds = (pollfd*)malloc(max_poll_elements * sizeof (struct pollfd));
	fd_lookup = (int*)malloc(max_poll_elements * sizeof (int));
	if (pollfds == NULL || fd_lookup == NULL) {
		lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
		return -1;
	}
#endif

	info.iface = iface;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}
	info.gid = -1;
	info.uid = -1;
	info.options = opts;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

//Start http server
	pthread_t httpServerThread;
	int rc = pthread_create(&httpServerThread, NULL, startHttpServer, NULL);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

//		Using video streaming
	int imageID = 0;
//	int sliceID = 0;
	char *header = new char[HEADER_LENGTH];
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


	n = 0;
	while (n >= 0 && !force_exit) {
		struct timeval tv;

		gettimeofday(&tv, NULL);

		if (((unsigned int)tv.tv_usec - oldus) > 50000) {
			libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_VIDEO_TRANSFER]);
			oldus = tv.tv_usec;
		}

#ifdef EXTERNAL_POLL

		/*
		 * this represents an existing server's single poll action
		 * which also includes libwebsocket sockets
		 */

		n = poll(pollfds, count_pollfds, 50);
		if (n < 0)
			continue;
		if (n){
//		Using video streaming
			imageID = (imageID+1)%100000;
 			Mat frame;
			camera>>frame;
			Mat small;
			double factor = 0.5;
			resize(frame, small, Size(), factor, factor, INTER_AREA);


			std::thread imageHashThread(imageHash, frame);

			for(int i=0;i<PIECE_NUM;i++){

				sprintf(header, "0%5d%2d", imageID, i);
				int sliceWidth = 640 * factor / PIECE_NUM;
				int sliceHeight = 480 * factor;

				Mat slice = small(Rect(i*sliceWidth, 0, sliceWidth, sliceHeight));
				if(!imencode(".jpg", slice, imageVec, compression_params)) printf("Write error\n");
				imageSize[i] = imageVec.size();

				pos[i] = i==0?0:pos[i-1]+imageSize[i-1];
				memcpy(&imageBuf[pos[i]], header, HEADER_LENGTH);
				memcpy(&imageBuf[pos[i]+HEADER_LENGTH], imageVec.data(), imageSize[i]);
				imageSize[i] += HEADER_LENGTH;
			}

			//send pieces in round-robin way
			pieceReorder();

			imageHashThread.join();

			for (n = 0; n < count_pollfds; n++)

				if (pollfds[n].revents)
					/*
					* returns immediately if the fd does not
					* match anything under libwebsockets
					* control
					*/
					if (libwebsocket_service_fd(context,
								  &pollfds[n]) < 0)
						goto done;

		}

#else
		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */
		n = libwebsocket_service(context, 50);
#endif
	}

#ifdef EXTERNAL_POLL
done:
#endif

	libwebsocket_context_destroy(context);

	lwsl_notice("libwebsockets-test-server exited cleanly\n");

#ifndef WIN32
	closelog();
#endif

	return 0;
}
