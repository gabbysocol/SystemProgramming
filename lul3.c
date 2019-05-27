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

#define VALID_ARGC 3

char *progname;
char **files;
int fcount = 0;
int pcount = 1;
int pmax;

int getfrequency(const char *filename)
{
    int fd;
    unsigned char buf[1024*1024];
    ssize_t nread;
    
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "%d: %s %s\n", getpid(), filename, strerror(errno));
        exit(1);
    }

    int counts[256] = {0};
	int count = 0;
		
    while (nread = read(fd, buf, sizeof buf), nread > 0)
    {
        for(int i = 0; i < nread; i++)
		{
			counts[buf[i]]++;
		}
		count += nread;
    }
    close(fd);
    
    printf("%d %s %d:", getpid(), filename, count);
	for(int i = 0; i < 256; i++)
	{
		if(counts[i] != 0)
			printf(" (%X %d) ", i, counts[i]);
		count -= counts[i];
	}
	printf("\n");
    return 0;
}

void getcounts()
{
    pid_t pid;
    for(int i = 0; i < fcount; i++)
    {
        if (pcount == pmax)
		{
		    wait();
		    pcount--;
		}
        
		pcount++;
		pid = fork();
		if (pid == 0)
		{
		    getfrequency(files[i]);
		    exit(0);
		}
		else 
            if (pid < 0)
            {
                fprintf(stderr, "%d: %s\n", getpid(), strerror(errno));
                return;
            }
    }
}

void searchdir(const char *dirpath)
{
    DIR *currdir;
    if (!(currdir = opendir(dirpath))) 
	{
        fprintf(stderr, "%d: %s %s\n", getpid(), dirpath, strerror(errno));
        return;
    }

    struct dirent *cdirent;
    errno = 0;

    while (cdirent = readdir(currdir)) 
	{
        if (!strcmp(".", cdirent->d_name) || !strcmp("..", cdirent->d_name))
            continue;

        char new_dirpath[strlen(dirpath) + strlen(cdirent->d_name) + 2];
        strcpy(new_dirpath, dirpath);
        strcat(new_dirpath, "/");
        strcat(new_dirpath, cdirent->d_name);

        if (cdirent->d_type == DT_DIR) 
			searchdir(new_dirpath);
        else 
            if (cdirent->d_type == DT_REG) 
            {
                files = (char**)realloc(files, (++fcount)*sizeof(char*));
                files[fcount - 1] = (char*)malloc((strlen(new_dirpath) + 1) * sizeof(char));
                strcpy(files[fcount - 1], new_dirpath);	
                continue;
            }
    }	

    if(errno)
    {
        fprintf(stderr, "%d: %s %s\n", getpid(), dirpath, strerror(errno));
		return;
    }

    if(closedir(currdir) != 0)
    {
        fprintf(stderr, "%d: %s %s\n", getpid(), dirpath, strerror(errno));
        return;
    }
}

int main(int argc, char *argv[])
{
    progname = basename(argv[0]);
    if (argc != VALID_ARGC) 
    {
        fprintf(stderr, "%s %d: Missing argument\n", progname, getpid());
        return 1;
    }
    
    pmax = atoi(argv[2]);

    searchdir(argv[1]);
    getcounts();
    while(pcount--)
        wait();
    return 0;
}