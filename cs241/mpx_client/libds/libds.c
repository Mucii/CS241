/** @file libds.c */
/* 
 * CS 241
 * The University of Illinois
 */

#define _GNU_SOURCE
#include "libds.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "jansson.h"
#include "libhttp.h"

int sock = 0;
/**
 * Sets the server the data store should connect to for
 * NoSQL operations.  You may assume this call will be 
 * made before any calls to datastore_init().
 *
 * @param server
 *   The hostname of the server.
 * @param port
 *   The post on the server to connect to.
 */




void datastore_set_server(const char *server, int port)
{

	 sock = socket(AF_INET, SOCK_STREAM, 0);
	//struct addrinfo * info = NULL;
	struct  hostent *hostp = NULL;
	
	//struct sockaddr * server = NULL;

	
	/* START OF TODO BLOCK */
	struct sockaddr_in server_x;
	server_x.sin_family = AF_INET;

	if((server_x.sin_addr.s_addr = inet_addr(server)) == (unsigned long)INADDR_NONE)
		{
	 
			/* When passing the host name of the server as a */
			/* parameter to this program, use the gethostbyname() */
			/* function to retrieve the address of the host server. */
			/***************************************************/
			/* get host address */
			hostp = gethostbyname(server);
			if(hostp == (struct hostent *)NULL)
			{
			printf("HOST NOT FOUND --> ");
			/* h_errno is usually defined */
			/* in netdb.h */
			printf("h_errno = %d\n",h_errno);
			printf("---This is a client program---\n");
			printf("Command usage: %s <server name or IP>\n", server);
			close(sock);
			exit(-1);
			}
			memcpy(&server_x.sin_addr, hostp->h_addr, sizeof(server_x.sin_addr));
		}
 







	server_x.sin_port = htons(port);

	int res = connect(sock, (struct sockaddr*) &server_x, sizeof(struct sockaddr_in));
	if(res < 0)
	{
		perror("connect");
		exit(1);
	}


	//printf("finish conection here \n");




}


/**
 * Initializes the data store.
 *
 * @param ds
 *    An uninitialized data store.
 */
void datastore_init(datastore_t *ds)
{
	pthread_mutex_init(&ds->mutex, NULL);
}


/**
 * Adds the key-value pair (key, value) to the data store, if and only if the
 * key does not already exist in the data store.
 *
 * @param ds
 *   An initialized data store.
 * @param key
 *   The key to be added to the data store.
 * @param value
 *   The value to associated with the new key.
 *
 * @retval 0
 *   The key already exists in the data store.
 * @retval non-zero
 *   The revision number assigned to the specific value for the given key.
 */
unsigned long datastore_put(datastore_t *ds, const char *key, const char *value)
{

	pthread_mutex_lock(&ds->mutex);
	//prepare request
	char * he = NULL;
	char * bod = NULL;
	asprintf(&bod,"{\"Value\":\"%s\"}",value);
	asprintf(&he,"PUT /%s HTTP/1.1\r\nContent-Type: application/json\r\nContent-Length: %lu\r\n\r\n",key,strlen(bod));
	
	//(stderr, "%s\n", bod);

	//send the request
	char temp;
	unsigned int length;
	int s =0;
	
	

	s = send(sock, he, strlen(he), 0);
	
	if(s < 0){
		perror("Client-write() error");
		s = getsockopt(sock,SOL_SOCKET,SO_ERROR,&temp,&length);
		}
		else if(s == 0){
			errno = temp;
			perror("SO_ERROR was");
		
		close(sock);
		exit(-1);
	}

	s = send(sock, bod, strlen(bod), 0);
	
	
	if(s < 0){
		perror("Client-write() error");
		s = getsockopt(sock,SOL_SOCKET,SO_ERROR,&temp,&length);
		}
		else if(s == 0){
			errno = temp;
			perror("SO_ERROR was");
		
		close(sock);
		exit(-1);
	}

	http_t http;
	int t = http_read(&http, sock);
	

	//fprintf(stderr, "http:%s\n",http.status);
	//fprintf(stderr, "http:%s\n",http.body);


	if(t == -1)
		exit(0);

	const char *header = http_get_status(&http);
	 size_t len = atol(http_get_header(&http, "Content-Length"));
	const char *body  = http_get_body(&http, &len);
	if(strcmp(header,"HTTP/1.1 201 Created") == 0){
		
		json_t *json_b = json_loads(body, 0, NULL);
		if(json_b == NULL)
			printf("Fail to parse json\n");

		
		char * revision = (char *)json_string_value(json_object_get(json_b, "rev"));
		unsigned long revi = atol(revision);
		pthread_mutex_unlock(&ds->mutex);
		return revi;
	}
	else if(strcmp(header,"HTTP/1.1 409 Conflict") == 0){
		pthread_mutex_unlock(&ds->mutex);
		return 0;
	}

	else
		return -1;
	
}


/**
 * Retrieves the current value, and its revision number, for a specific key.
 *
 * @param ds
 *   An initialized data store.
 * @param key
 *   The specific key to retrieve the value.
 * @param revision
 *   If non-NULL, the revision number of the returned value will be written
 *   to the address pointed to by <code>revision</code>.
 *
 * @return
 *   If the data store contains the key, a new string containing the value
 *   will be returned.  It is the responsibility of the user of the data
 *   store to free the value returned.  If the data store does not contain
 *   the key, NULL will be returned and <code>revision</code> will be unmodified.
 */
const char *datastore_get(datastore_t *ds, const char *key, unsigned long *revision)
{
	pthread_mutex_lock(&ds->mutex);
	char *request = NULL;
	char temp;
	
	unsigned int length;
	asprintf(&request,"GET /%s HTTP/1.1\r\n\r\n",key);
	int s =0;

	s = send(sock, request, strlen(request), 0);
		
	
	if(s < 0){
		perror("Client-write() error");
		s = getsockopt(sock,SOL_SOCKET,SO_ERROR,&temp,&length);
		}
		else if(s == 0){
			errno = temp;
			perror("SO_ERROR was");
		
		close(sock);
		exit(-1);
	}



	//fprintf(stderr, "request:%s",request);


	

	http_t http;
	int t = http_read(&http, sock);
	

	if(t == -1)
		exit(0);

	const char *header = http_get_status(&http);

	 size_t len = atol(http_get_header(&http, "Content-Length"));
	const char *body 	 = http_get_body(&http, &len);
	if(strcmp(header,"HTTP/1.1 200 OK") == 0){
		
	
		//fprintf(stderr, "%s\n",header);
		json_t *json_b = json_loads(body, 0, NULL);
		if(json_b == NULL)
			printf("Fail to parse json\n");

		
		//json_unpack(json_b,"{s:s,s:i,s:s}","_id",&name,"_rev",&revision,"Value",&value);
		char *value = (char *)json_string_value(json_object_get(json_b, "Value"));
		*revision = atol(json_string_value(json_object_get(json_b,"_rev")));

		//fprintf(stderr, "revision:%d\n", *revision);
		pthread_mutex_unlock(&ds->mutex);
		return value;
	}
	else if(strcmp(header,"HTTP/1.1 404 Not Found") == 0){
		pthread_mutex_unlock(&ds->mutex);

		return NULL;
	}
	else
		return NULL;
}


/**
 * Updates the specific key in the data store if and only if the
 * key exists in the data store and the key's revision in the data
 * store matches the knwon_revision specified.
 * 
 * @param ds
 *   An initialized data store.
 * @param key
 *   The specific key to update in the data store.
 * @param value
 *   The updated value to the key in the data store.
 * @param known_revision
 *   The revision number for the key specified that is expected to be found
 *   in the data store.  If the revision number specified in calling the
 *   function does not match the revision number in the data store, this
 *   function will not update the data store.
 *
 * @retval 0
 *    The revision number specified did not match the revision number in the
 *    data store or the key was not found in the data store.  If the key is
 *    in the data store, this indicates that the data has been modified since
 *    you last performed a datastore_get() operation.  You should get an
 *    updated value from the data store.
 * @retval non-zero
 *    The new revision number for the key, now associated with the new value.
 */
unsigned long datastore_update(datastore_t *ds, const char *key, const char *value, unsigned long known_revision)
{
	//prepare request
		pthread_mutex_lock(&ds->mutex);
	
	char * he = NULL;
	char * bod = NULL;
	asprintf(&bod,"{\"Value\":\"%s\",\"_rev\":\"%lu\"}",value,known_revision);
	asprintf(&he,"PUT /%s HTTP/1.1\r\nContent-Type: application/json\r\nContent-Length: %lu\r\n\r\n",key,strlen(bod));
	

	//send the request
	char temp;
	unsigned int length;
	int s =0;
	s = send(sock, he, strlen(he), 0);
	
	
	if(s < 0){
		perror("Client-write() error");
		s = getsockopt(sock,SOL_SOCKET,SO_ERROR,&temp,&length);
		}
		else if(s == 0){
			errno = temp;
			perror("SO_ERROR was");
		
		close(sock);
		exit(-1);
	}

	s = send(sock, bod, strlen(bod), 0);
	
	
	if(s < 0){
		perror("Client-write() error");
		s = getsockopt(sock,SOL_SOCKET,SO_ERROR,&temp,&length);
		}
		else if(s == 0){
			errno = temp;
			perror("SO_ERROR was");
		
		close(sock);
		exit(-1);
	}

	http_t http;
	int t = http_read(&http, sock);
	

	if(t == -1)
		exit(0);

	const char *header = http_get_status(&http);
	 size_t len 	 = atol(http_get_header(&http, "Content-Length"));
	const char *body 	 = http_get_body(&http, &len);
	if(strcmp(header,"HTTP/1.1 201 Created") == 0){
		unsigned long revision =0;

		json_t *json_b = json_loads(body, 0, NULL);
		if(json_b == NULL)
			printf("Fail to parse json\n");

		
		
		revision = atol(json_string_value(json_object_get(json_b, "rev")));
		pthread_mutex_unlock(&ds->mutex);
		
		return revision;
	}
	else if(strcmp(header,"HTTP/1.1 409 Conflict") == 0){
		pthread_mutex_unlock(&ds->mutex);

		return 0;
	}


	else
		return -1;
	
}


/**
 * Deletes a specific key from the data store.
 *
 * @param ds
 *   An initialized data store.
 * @param key
 *   The specific key to update in the data store.
 * @param known_revision
 *   The revision number for the key specified that is expected to be found
 *   in the data store.  If the revision number specified in calling the
 *   function does not match the revision number in the data store, this
 *   function will not update the data store.
 *
 * @retval 0
 *    The revision number specified did not match the revision number in the
 *    data store or the key was not found in the data store.  If the key is
 *    in the data store, this indicates that the data has been modified since
 *    you last performed a datastore_get() operation.  You should get an
 *    updated value from the data store.
 * @retval non-zero
 *    The key was deleted from the data store.
 */
unsigned long datastore_delete(datastore_t *ds, const char *key, unsigned long known_revision)
{

	pthread_mutex_lock(&ds->mutex);
	
	char *request = NULL;
	char temp;
	
	unsigned int length;
	asprintf(&request,"DELETE /%s HTTP/1.1\r\nIf-Match:\"%lu\"\r\n\r\n",key,known_revision);
	int s =0;

	s = send(sock, request, strlen(request), 0);
		
	
	if(s < 0){
		perror("Client-write() error");
		s = getsockopt(sock,SOL_SOCKET,SO_ERROR,&temp,&length);
	}
	else if(s == 0){
			errno = temp;
			perror("SO_ERROR was");
		
		close(sock);
		exit(-1);
	}



	//fprintf(stderr, "request:%s",request);


	

	http_t http;
	int t = http_read(&http, sock);

	if(t == -1)
		exit(0);

	const char *header = http_get_status(&http);
	//size_t len 	 = (size_t)http_get_header(&http, "Content-Length");
	//const char *body 	 = http_get_body(&http, &len);
	if(strcmp(header,"HTTP/1.1 200 OK") == 0){
		pthread_mutex_unlock(&ds->mutex);

		return 1;
	}
	else if(strcmp(header,"HTTP/1.1 409 Conflict") == 0){
		pthread_mutex_unlock(&ds->mutex);

		return 0;
	}
	return -1;
}


/**
 * Destroys the data store, freeing any memory that is in use by the
 * data store.
 *
 * @param ds
 *   An initialized data store.
 */
void datastore_destroy(datastore_t *ds)
{
	pthread_mutex_destroy(&ds->mutex);

}
