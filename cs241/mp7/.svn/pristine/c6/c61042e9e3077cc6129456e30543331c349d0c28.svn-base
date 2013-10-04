/* 
 * CS 241
 * The University of Illinois
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "libmapreduce.h"

#define CHARS_PER_INPUT 30000
#define INPUTS_NEEDED 10

static const char *long_key = "Longest_Word";

/* Takes input string and maps the longest
 * word to the key, long_key.
 */
void map(int fd, const char *data)
{
		int i;
		char *word = malloc(100);
		char *buf  = malloc(100);
		word[0] = buf[0] = '\0';
		buf[0] = '\0';
		int max=0;
		//fprintf(stderr, "data :%s\n", data);
		const char *word_end = data;
		int word_len;

		int ith_word = 0;
	while (1)
	{

		 word_len = 0;
		
		if(ith_word != 0) // can't do word_end == data since it might be the first letter is not the letter then inifinite loop here
			word_end++;


		ith_word++;
		
		/* Get each word... */
		while (1)
		{
			//fprintf(stderr,"loop1 %d\n",*word_end);
			if ((isalpha(*word_end) == 0)|| (word_len == 99)){
			
				break;
			}
			else {
				word[word_len] = *word_end;
				word_len++;
				
			}

			word_end++;
		}
		
		word[word_len] = '\0';
		
		if(max < word_len)
		{
			for(i=0; i<=word_len; i++)
				buf[i] = word[i];
			max = word_len;
			
		}
		if (*word_end == '\0') { break; }
		

	}
		// char s[100];
		char s[200];	//need some room to store the long_key
		int len = sprintf(s,"%s: %s\n", long_key,buf);
		//fprintf(stderr, "string :%s\n", s);
		write(fd,s, len);
		close(fd);
	
}

/* Takes two words and reduces to the longer of the two
 */
const char *reduce(const char *value1, const char *value2)
{
	int i;
	for(i=0; value1[i]!=0 && value2[i]!=0; i++);
	char *new = (char *)malloc(sizeof(char *)*i);	
	if(value1[i] == 0)
		strcpy(new,value2);
	else
		
		strcpy(new,value1);
		//free(v1);
		//free(v2);
	return new;
}


int main()
{
	FILE *file = fopen("alice.txt", "r");
	char s[1024];
	int i;

	char **values = malloc(INPUTS_NEEDED * sizeof(char *));
	int values_cur = 0;
	
	values[0] = malloc(CHARS_PER_INPUT + 1);
	values[0][0] = '\0';

	while (fgets(s, 1024, file) != NULL)
	{
		if (strlen(values[values_cur]) + strlen(s) < CHARS_PER_INPUT)
			strcat(values[values_cur], s);
		else
		{
			values_cur++;
			values[values_cur] = malloc(CHARS_PER_INPUT + 1);
			values[values_cur][0] = '\0';
			strcat(values[values_cur], s);
		}
	}

	values_cur++;
	values[values_cur] = NULL;
	
	fclose(file);

	mapreduce_t mr;
	mapreduce_init(&mr, map, reduce);

	mapreduce_map_all(&mr, (const char **)values);
	mapreduce_reduce_all(&mr);
	
	const char *result_longest = mapreduce_get_value(&mr, long_key);

	if (result_longest == NULL) { printf("MapReduce returned (null).  The longest word was not found.\n"); }
	else { printf("Longest Word: %s\n", result_longest); free((void *)result_longest); }

	mapreduce_destroy(&mr);

	for (i = 0; i < values_cur; i++)
		free(values[i]);
	free(values);

	return 0;
}
