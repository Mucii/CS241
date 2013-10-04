/** @file libmapreduce.c */
/* 
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>

#include "libmapreduce.h"
#include "libds/libds.h"


static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */


/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value, mapreduce_t *mr)
{
	unsigned long rec;
	char *old_value;
	old_value= (char *)datastore_get(mr->datastore,key, &rec);
	if(old_value == NULL){ //can't find it
	datastore_put(mr->datastore, key, value);
	free((void *)key);
	free((void *)value);

	}
	else{
		const char *new_free;
		new_free = mr->myreduce(old_value, value);
		datastore_update(mr->datastore, key, new_free, rec);
		free((void *)key);
		free((void *)old_value);
		free((void *)value);
		free((void *)new_free);
	}
}


/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr)
{
	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	if (bytes_read == 0)
		return 0;
	else if(bytes_read < 0)
	{
		fprintf(stderr, "error in read.\n");
		return -1;
	}

	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL)
	{
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
	buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
	}

	return 1;
}

void select_read(void *mrr){
	int i;
	mapreduce_t * mr = (mapreduce_t *)mrr;
	fd_set set1,set2;					//needs two sets since the select founc will change the set
	char **buf = (char **)malloc(sizeof(char *)*(mr->size));
	for(i=0; i<mr->size; i++){
		buf[i] = (char *)malloc((BUFFER_SIZE+1)*sizeof(char));
			buf[i][0] ='\0';
	}

	
	FD_ZERO(&set1);
	for(i=0; i< (mr->size); i++)
		FD_SET(mr->pipe[i][0], &set1);
	int n_active = mr->size;
	while(n_active > 0){
		int nfd;
		set2= set1;
		nfd = select(FD_SETSIZE, &set2, NULL, NULL, NULL);
		if(nfd < 0){
			perror("select failed");
			exit(1);
		}
		for(i=0; i< (mr->size); i++){

			if(FD_ISSET(mr->pipe[i][0], &set2)){ // select ith pipe
				int success = read_from_fd(mr->pipe[i][0], buf[i], mr);
				if(success == -1){
					fprintf(stderr,"fail to read selected pipe\n");
					exit(1);
				}
				else if(success == 0){
					n_active--;
					close(mr->pipe[i][0]);
					FD_CLR(mr->pipe[i][0], &set1);
				}
				
			}

		}
	}//while
	
		for(i=0; i<mr->size; i++)
		free(buf[i]);
	free(buf);

}
/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr, 
                    void (*mymap)(int, const char *), 
                    const char *(*myreduce)(const char *, const char *))
{	
	mr->mymap = mymap;
	mr->myreduce = myreduce;
	mr->pipe = NULL;
	mr->tid = NULL;
	mr->size = 0;
	mr->datastore = (datastore_t *)malloc(sizeof(datastore_t));
	datastore_init(mr->datastore);

}


/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values)		//how to free the values using mr or not?
{
	int size,i;
	for(size=0; values[size] != 0 ; size++);
		mr->size = size;
	
	mr->pipe = (int **)malloc(sizeof(int *)*size);
	for(i=0; i<size; i++)
		mr->pipe[i] = (int *)malloc(sizeof(int)*2);
	

	mr->pid = (pid_t *)malloc(sizeof(pid_t) * size);

	for(i=0; i<size; i++){
		pipe(mr->pipe[i]); // make sure the read port will continuous
		if((mr->pid[i] = fork()) <= 0){// child process
			close(mr->pipe[i][0]);  //close read port
			mr->mymap(mr->pipe[i][1], values[i]);
		//	free(values[i]);
			mapreduce_destroy(mr);
			exit(0);
		}
		else{
			close(mr->pipe[i][1]);
			

		}
	}

	

	mr->tid = (pthread_t *)malloc(sizeof(pthread_t));
	pthread_create(mr->tid, NULL, (void *)select_read, (void *)mr);

		

}


/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr)
{
	//fprintf(stderr, "%s\n", "I entered");
	pthread_join(*(mr->tid), NULL);
	//fprintf(stderr, "%s\n", "I exited");
}


/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key)
{
	const char *value;
	unsigned long recv;
	value=datastore_get(mr->datastore,(char *)result_key, &recv);
	if(value == NULL)//can't find it
		return NULL;
		else
			return value;
}


/**
 * Destroys the mapreduce data structure
 */
void mapreduce_destroy(mapreduce_t *mr)
{
	datastore_destroy(mr->datastore);
	free((void *)mr->datastore);
	int i;
	for(i=0; i<mr->size; i++)
	free((void *)mr->pipe[i]);
	free((void *)mr->pipe);
	free((void *)mr->tid);
	free((void *)mr->pid);
	
}
