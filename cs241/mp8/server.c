/** @file server.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <queue.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "queue.h"
#include "libhttp.h"
#include "libdictionary.h"

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";
pthread_t *pid ;
int f_signal =1;
int pthread_num = 10;
int counter =0;
int sfd =0;
int f_main = 1;
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
char* process_http_header_request(const char *request)
{
	// Ensure our request type is correct...
	if (strncmp(request, "GET ", 4) != 0)
		return NULL;

	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " HTTP/1.1"(9)...
	int len = strlen(request) - 4 - 9;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 4, len);
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
	int flag = 0, f_type =0;
	char * code_string = NULL;
	char *length = NULL;
	struct stat sb;
	
	printf("Thread started\n");

	while(f_signal){
		flag = 0;
		f_type =0;
	int temp = http_read(&http_request, cfd);
	if(temp == -1)
		break;
	
	 

	int status_code =0;
	char *body = NULL;
	const char *statusline= http_get_status(&http_request);
	char *filename = process_http_header_request(statusline);
	char * file_type = NULL;
	char * new = NULL;
	if(filename != NULL){
		
		if(strcmp (filename, "/") == 0)
			asprintf(&new,"web/index.html");
			else
				asprintf(&new,"web%s", filename);

				free(filename);
		

				  char *redex = NULL;
				int i;
			for(i=0; new[i] != '\n'; i++)
			if(new[i] == '.')
			{
			
				break;
			}


			asprintf(&redex,"%s",new+i);


			if(strcmp(redex, ".html") == 0){
				asprintf(&file_type, "text/html");
			}
			else if(strcmp(redex, ".css") == 0){
				asprintf(&file_type, "text/css");

			}
			else if(strcmp(redex, ".jpg") == 0){
				asprintf(&file_type, "image/jpeg");

			}
			else if(strcmp(redex, ".png") == 0){
				asprintf(&file_type, "image/png");

			}
			else{
				asprintf(&file_type,"text/plain");
			}

			free(redex);


		FILE * ffdd = fopen((const char *)new, "r");
		if(ffdd == NULL){
			
			asprintf(&body,"%s",HTTP_404_CONTENT);
			
			free(file_type);
			asprintf(&file_type, "text/html");
			asprintf(&code_string, "%s", HTTP_404_STRING);
			
			status_code = 404;
		}
		else{
						
			// struct stat sb;
			
			stat(new,&sb);
			body = malloc(sb.st_size);
			
			fread(body, 1, sb.st_size, ffdd);
			
			status_code = 200;
			asprintf(&code_string, "%s",HTTP_200_STRING);
			f_type = 1;
			fclose(ffdd);
			
		}
		

	}
	else{
			
			asprintf(&body,"%s",HTTP_501_CONTENT);
			

			asprintf(&file_type, "text/html");
			status_code = 501;
			asprintf(&code_string, "%s",HTTP_501_STRING);
		

		
		}

		
		if (f_type == 1)
		{

			asprintf(&length,"%ld", sb.st_size);

		}

		else if(f_type == 0)
		{
			
			asprintf(&length,"%ld", strlen(body));
		}
	//prepare a response http
	

	char *header = NULL, *type = NULL, *con_length = NULL, * cont = NULL;
	



	asprintf(&type, "Content-Type: %s\r\n",file_type);
	free(file_type);
	

	asprintf(&con_length, "Content-Length: %s\r\n",length);
	free(length);
	
	
	const char *value =http_get_header(&http_request, "Connection");
	char * status  = NULL;

	if(strcasecmp(value,  "Keep-Alive") == 0){
		asprintf(&status, "Keep-Alive");
		
	}
	else{
		asprintf(&status, "close");
		flag = 1;
	}
	asprintf(&cont, "Content-Type: %s\r\n",status);
	
	

	free(status);
	asprintf(&header, "HTTP/1.1 %d %s\r\n%s%s%s\r\n",status_code , code_string,type,con_length,cont);


	free(code_string);
	free(type);
	free(con_length);
	free(cont);
	free(new);
	http_free(&http_request);


	






		unsigned int sent_b=0;
		assert(strlen(header) == (unsigned)send(cfd, header,strlen(header),0));
		// unsigned int a = send(cfd, header+sent_h,strlen(header)-sent_h,0);
	while(1){
		unsigned int b = 0;

		if(f_type == 0)
			b = send(cfd,body + sent_b,strlen(body)-sent_b,0);
		else {
			b = send(cfd,body + sent_b, sb.st_size-sent_b,0);
			
		}
		
		sent_b = sent_b +b;
		if(f_type ==0){
			if(sent_b == strlen(body))
					break;
		}
		else if(f_type == 1){
			if(sent_b == sb.st_size)
				break;

		}

	}




	
			if(flag == 1)
				break;	

			free(body);
			free(header);


	}

	fprintf(stderr, "Thread finish\n");

	close(cfd);
	return NULL;
}

void handler() {
	//printf("must free meme here\n");
	f_signal =0;
	int i;
	for(i=0; i<counter; i++)
	pthread_detach(pid[i]);
	free(pid);
	close(sfd);
	//f_main = 0;
	exit(0);

}

int main(int argc, char **argv)
{
	signal(SIGINT,handler);

	struct addrinfo hints;
	struct addrinfo *result, *rp;
	//struct sockaddr_in client_addr;
	struct sockaddr client_addr;
	
	socklen_t client_addr_len = sizeof(client_addr);
	int s=0,cfd=0, i=0;
	
	if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

   memset(&hints, 0, sizeof(struct addrinfo));
   memset(&client_addr,0,sizeof(struct sockaddr));
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

		pid = (pthread_t *)malloc(sizeof(pthread_t) * pthread_num);
		//memset(&pid,0,sizeof(pthread_t));
		counter = 0;

		while(f_main){
		cfd = accept(sfd, &client_addr, &client_addr_len);
		
		if(counter < pthread_num)
		pthread_create(&pid[counter], NULL, (void *)p_request, (void *)(&cfd));
		else{
			pthread_num = pthread_num *2;
			pthread_t *new_id = (pthread_t *)malloc(sizeof(pthread_t) * pthread_num);
			//memset(&new_id,0,sizeof(pthread_t));
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
