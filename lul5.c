#include <stdio.h> 
#include <sys/types.h> 
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h> // for creating humu
#include <syscall.h>

#define VALID_ARGC 3

typedef enum {
    _nullstat,
    _busystat,
    _freestat
} thread_status_t;

typedef struct {
    thread_status_t *thread_status;
    char *filename;
} thread_params_t;

pthread_t *THREADS;
thread_status_t *THREADS_STATUS;

char *progname;

int getfrequency(const char *filename)
{
    int fd;
    unsigned char buf[1024*1024];
    ssize_t nread;
    
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "%s: %s %s\n", progname, filename, strerror(errno));
        return -1;
    }

    int counts[256] = {0};
	int count = 0;
		
    nread = read(fd, buf, sizeof buf);
    while (nread > 0)
    {
        for(int i = 0; i < nread; i++)
		{
			counts[buf[i]]++;
		}
		count += nread;
        nread = read(fd, buf, sizeof buf);
    }

    if (nread == -1) 
    {
        fprintf(stderr, "%s: %s %s\n", progname, filename, strerror(errno));
        return -1;
    }
    
    if (close(fd) == -1) 
    {
        fprintf(stderr, "%s: %s %s\n", progname, filename, strerror(errno));
        return -1;
    }
    
    printf("%ld %s %d:", syscall(SYS_gettid), filename, count);
	for(int i = 0; i < 256; i++)
	{
		if(counts[i] != 0)
			printf(" (%X %d) ", i, counts[i]);
		count -= counts[i];
	}
	printf("\n");
    return 0;
}

void *passtofunction(void *args)
{
    thread_params_t *thread_params = (thread_params_t *) args;

    getfrequency(thread_params->filename);

    *(thread_params->thread_status) = 2;
    while (*(thread_params->thread_status) != 0);

    free(thread_params->filename);
    free(thread_params);

    return;
}

void searchdir(const char *dirpath, const int threads_count)
{
    DIR *currdir;
    if (!(currdir = opendir(dirpath))) 
	{
        fprintf(stderr, "%s : %s\n", dirpath, strerror(errno));
        return;
    }

    struct dirent *cdirent;
    thread_params_t *thread_params;
    pthread_t thread;
    errno = 0;

    while (cdirent = readdir(currdir)) 
	{
        if (!strcmp(".", cdirent->d_name) || !strcmp("..", cdirent->d_name))
            continue;

        char *full_path = malloc(strlen(cdirent->d_name) + strlen(dirpath) + 2);
        strcpy(full_path, dirpath);
        strcat(full_path, "/");
        strcat(full_path, cdirent->d_name);
        
        struct stat info;
        if (lstat(full_path, &info) == -1)
        {
            fprintf(stderr, "%s : %s - %s\n", progname, full_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(info.st_mode)) 
        {
            searchdir(full_path, threads_count);
        } 
        else
        {
            if (S_ISREG(info.st_mode)) 
            {
                int thread_id = 0;
                while (THREADS_STATUS[thread_id] == 1) 
                {
                    thread_id++;
                    if (thread_id == threads_count)
                    {
                        thread_id = 0;
                    }
                }
                
                THREADS_STATUS[thread_id] = 0;
                if (pthread_join(THREADS[thread_id], NULL) == -1)  // function for waiting for finishing of thread
                {
                    fprintf(stderr, "%s : %s\n", progname, strerror(errno));
                    return;
                };

                thread_params = malloc(sizeof(thread_params_t));
                thread_params->thread_status = &(THREADS_STATUS[thread_id]);
                thread_params->filename = full_path;

                // creating fhumu
                THREADS_STATUS[thread_id] = 1;

                if (pthread_create(&thread, NULL, &passtofunction, thread_params) == -1) // function starts a new thread in the calling process.
                {
                    fprintf(stderr, "%s : %s\n", progname, strerror(errno));
                    return;
                };

                THREADS[thread_id] = thread;
            }
        }
    }	

    if(errno)
    {
        fprintf(stderr, "%s: %s\n", progname, strerror(errno));
		return;
    }

    if(closedir(currdir) != 0)
    {
        fprintf(stderr, "%s: %s\n", progname, strerror(errno));
        return;
    }
}

char thread_finished(int threads_count) 
{
    int i;
    for (i = 0; (i < threads_count); i++) 
    {
        if (THREADS_STATUS[i] != 2) 
        {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{
    //output: id, full_path, size of a file, sym - freq
    progname = basename(argv[0]);
    int threads_count;

    if (argc != VALID_ARGC) 
    {
        fprintf(stderr, "%s : Missing argument\n", progname);
        return 1;
    }
    // fprintf(stderr, "%s GGGGGGGGGGGGGGGGGGGGGG\n", progname);
    threads_count = atoi(argv[2]);
    if (threads_count < 1) 
    {
        fprintf(stderr, "%s: Thread number must be bigger than 1.\n", progname);
        return 1;
    }

    THREADS_STATUS = calloc(sizeof(thread_status_t), (size_t) threads_count);
    for (int i = 0; i < threads_count; i++) 
    {
        THREADS_STATUS[i] = 0;
    }

    THREADS = calloc(sizeof(pthread_t), (size_t) threads_count);
    searchdir(argv[1], threads_count);
    // while (!thread_finished(threads_count));   
    return 0;
}