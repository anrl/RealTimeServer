/*
 * http.h
 *
 *  Created on: Sep 15, 2014
 *      Author: frye
 */

#ifndef HTTP_H_
#define HTTP_H_

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define PORT 9001
//#define FILENAME "picture.png"
//#define MIMETYPE "image/png"

#define INDEX "index.html"

const char *getType(const char *url){
	const char * ptr = url;
	while (*ptr!='.' && *ptr!=' ') ptr++;
	return ++ptr;
}

static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  struct MHD_Response *response;
  int fd;
  int ret;
  struct stat sbuf;

  if (0 != strcmp (method, "GET"))
    return MHD_NO;
//  cout << url << endl;
  const char* filename= strcmp(url, "/")?url+1:INDEX;
//  cout << "filename: " << filename << endl;

  if ( (-1 == (fd = open (filename, O_RDONLY))) ||
       (0 != fstat (fd, &sbuf)) )
    {
      /* error accessing file */
      if (fd != -1)
	(void) close (fd);
      const char *errorstr =
        "<html><body>An internal server error has occured!\
                              </body></html>";
      response = MHD_create_response_from_buffer (strlen (errorstr),
					 (void *) errorstr, MHD_RESPMEM_PERSISTENT);
      if (NULL != response)
        {
          ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
          MHD_destroy_response (response);

          return ret;
        }
      else
        return MHD_NO;
    }
  response = MHD_create_response_from_fd_at_offset (sbuf.st_size, fd, 0);
  if (strcmp (getType(url), "js")==0) MHD_add_response_header (response, "Content-Type", "text/javascript");
  else MHD_add_response_header (response, "Content-Type", "text/html");
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}

void *startHttpServer (void *arg)
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon)
  {
	  std::cerr<<"Error starting server"<<std::endl;
	  exit(0);
  }

  (void) getchar ();

  MHD_stop_daemon (daemon);

  exit(0);
}


#endif /* HTTP_H_ */
