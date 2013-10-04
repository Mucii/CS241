#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include "libds.h"
#include "libhttp.h"
#include "jansson.h"




const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";
datastore_t * database = NULL;
/**
 * Processes the request line of the HTTP header.
 * 
 * @param request The request line of the HTTP header.  This should be
 *                the first line of an HTTP request header and must
 *                NOT include the HTTP line terminator ("\r\n").
 *
 * @return The filename of the requested document or NULL if the
 *         request is not supported by the server.  If a filename
 *         is returned, the string must be free'd by a call to free().
 */
char* process_http_header_request(const char *request, char **operation)
{
	// Ensure our request type is correct...
	char * copy = NULL;
	asprintf(&copy, "%s",request);
	*operation = strtok(copy," ");
	fprintf(stderr, "request%s\n", request);

	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " HTTP/1.1"(9)...
	int len = strlen(request) - strlen(*operation) - 9 -2;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + strlen(*operation) +2, len);
	filename[len] = '\0';

	// Prevent a directory attack...
	//  (You don't want someone to go to http://server:1234/../server.c to view your source code.)
	if (strstr(filename, ".."))
	{
		free(filename);
		return NULL;
	}

	return filename;
}

void p_request(void * cf){
	int cfd = *((int *)cf);
	http_t http_request;
	

	

	while(1){
		
	int temp = http_read(&http_request, cfd);
	if(temp == -1)
		return ;
	
	fprintf(stderr, "%s\n",http_request.status);
	fprintf(stderr, "%s\n",http_request.body);

	

	
	char *body = NULL;
	char *operation = NULL;
	const char *statusline= http_get_status(&http_request);
	char *key = process_http_header_request(statusline,&operation);
	fprintf(stderr, "key:%s\noperation: %s\n", key,operation);
	char *header = NULL;
	unsigned long revision = 0;
	const char *value = NULL;
	const char *len = NULL;
	size_t leng = 0;
	const char * data_put = NULL;
	json_t * json_b = NULL;
	if(strcmp(operation, "GET")==0){
			value = datastore_get(database, key,&revision);
			if(value == NULL){
				asprintf(&body,"{\"error\":\"not found\",\"reason\":\"The key does not exist in the database.\"}");

				asprintf(&header, "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n",strlen(body));
				
				fprintf(stderr, "headerlen: %ld\n", strlen(header)+strlen(body));
			}
			else{
				asprintf(&body,"{\"_id\":\"%s\",\"_rev\":\"%ld\",\"Value\":\"%s\"}",key,revision,value);
				asprintf(&header, "HTTP/1.1 200 OK\r\nEtag: \"%ld\"\r\nContent-Type: application/json\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n",revision,strlen(body));
			}
		}

	else if(strcmp(operation, "PUT")==0){
			const char *va = NULL;
			const char *revi = NULL;
			len = http_get_header(&http_request, "Content-Length");
			leng = atol(len);
			data_put = http_get_body(&http_request, &leng);
			json_b = json_loads(data_put,0, NULL);
			if(json_b == NULL)
				fprintf(stderr, "fail to parse json\n" );
				va = json_string_value(json_object_get(json_b,"Value"));
				revi = json_string_value(json_object_get(json_b,"_rev"));
			unsigned long return_rev = 0;
			if(revi == NULL)
			return_rev = datastore_put(database, key,va);
			else
			return_rev = datastore_update(database, key,va,atol(revi));	
			if(return_rev == 0){
				asprintf(&body,"{\"error\":\"conflict\",\"reason\":\"Document update conflict.\"}");
				asprintf(&header,"HTTP/1.1 409 Conflict\r\nContent-Type: application/json\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n",strlen(body));
			}
			else{
				asprintf(&body, "{\"ok\":true,\"rev\":\"%ld\",\"id\":\"%s\"}",return_rev,key);
				asprintf(&header, "HTTP/1.1 201 Created\r\nEtag: \"%ld\"\r\nContent-Type: application/json\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n",return_rev,strlen(body));
			}
		}
		else if(strcmp(operation,"DELETE")==0){
				 const char * num = NULL;
				 num = http_get_header(&http_request,"If-Match");
			unsigned long return_rev= datastore_delete(database,key,atol(num));
			if(return_rev == 0){
				asprintf(&body,"{\"error\":\"conflict\",\"reason\":\"Revision number was out of date.\"}");
				asprintf(&header,"HTTP/1.1 409 Conflict\r\nContent-Type: application/json\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n",strlen(body));
			}
			else{
				asprintf(&body,"{\"ok\":true,\"rev\":\"%ld\"}",return_rev);
				asprintf(&header,"HTTP/1.1 200 OK\r\nEtag: \"%ld\"\r\nContent-Type: application/json\r\nContent-Length: %ld\r\nConnection: keep-alive\r\n\r\n",return_rev,strlen(body));
			}

		}
		else{
			fprintf(stderr, "wrong operation\n" );
			exit(1);
		}
	

		fprintf(stderr, "%s\n",header );
		fprintf(stderr, "%s\n",body );
		
		unsigned int sent_b=0;
		assert(strlen(header) == (unsigned)send(cfd, header,strlen(header),0));
		// unsigned int a = send(cfd, header+sent_h,strlen(header)-sent_h,0);
	while(1){
		unsigned int b = 0;

		
			b = send(cfd,body + sent_b,strlen(body)-sent_b,0);
			sent_b = sent_b +b;
			if(sent_b == strlen(body))
					break;
		

		}




	
		

			free(body);
			free(header);


	}
	close(cfd);

}

void handler() {
	//printf("must free meme here\n");
	exit(0);

}

int main(int argc, char **argv)
{
	signal(SIGINT,handler);
    database = (datastore_t *)malloc(sizeof(datastore_t));
	datastore_init(database);

	struct addrinfo hints;
	struct addrinfo *result, *rp;
	//struct sockaddr_in client_addr;
	struct sockaddr client_addr;
	
	socklen_t client_addr_len;
	int s,sfd,cfd, i;
	int pthread_num = 10;
	if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

   memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;


   s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
            continue;

       if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  // Success 

       close(sfd);
    }
    
   


   if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

   freeaddrinfo(result);           /* No longer needed */
	
	if(listen(sfd, 10) == -1)
		exit(EXIT_FAILURE);

		pthread_t *pid = (pthread_t *)malloc(sizeof(pthread_t) * pthread_num);
		int counter = 0;

		while(1){
		cfd = accept(sfd, &client_addr, &client_addr_len);

		if(counter < pthread_num)
		pthread_create(&pid[counter], NULL, (void *)p_request, (void *)(&cfd));
		else{
			pthread_num = pthread_num *2;
			pthread_t *new_id = (pthread_t *)malloc(sizeof(pthread_t) * pthread_num);
			for(i=0; i< (pthread_num/2) ; i++){
				new_id[i] = pid[i];
			}
			free(pid);
			pid = new_id;
			new_id = NULL;
			pthread_create(&pid[counter], NULL, (void *)p_request, (void *)(&cfd));
		}

			counter++;
		}//while








	return 0;
}

