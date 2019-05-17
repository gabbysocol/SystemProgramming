#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#define BUFF_SIZE 4096
#define PATH_LEN 65535



static char* prog_name;

char* basename(char *filename)
{
    char *p = strrchr(filename, '/');
    return p ? p + 1 : filename;
}

// trying open directory
unsigned long long bypassdir(const char *src_dir, const char *dest_dir, FILE *outfile, unsigned long long files_count, int N1, int N2)
{
    files_count = 0;
    DIR *curr = opendir(src_dir);
    if (!curr) 
    {
        fprintf(stderr, "%s : can''t open directory - %s, %s \n",prog_name, src_dir, strerror(errno));
        return 0;
    }

    // dir = opendir( "some/path/name" )
    // entry = readdir( dir )
    // while entry is not NULL:
    //     do_something_with( entry )
    //     entry = readdir( dir )
    // closedir( dir )

    struct dirent *info;
    char *full_path = (char*)malloc(PATH_LEN * sizeof (char));

    if (!full_path) 
    {
        fprintf(stderr,"%s : can''t allocate memory for directory - %s, %s \n", prog_name, src_dir, strerror(errno));
        return 0;
    }

    if (!realpath(src_dir, full_path))
    {
        fprintf(stderr,"%s : can''t get full path of directory - %s, %s \n", prog_name, src_dir, strerror(errno));
        free(full_path);
        closedir(curr);
        return 0;
    }

    char* out_path = malloc(PATH_LEN * sizeof (char));
    if (!realpath(dest_dir, out_path)) 
    {
        fprintf(stderr,"%s : can''t get full path of file - %s, %s \n", prog_name, dest_dir, strerror(errno));
        return 0;
    }

    strcat(full_path, "/");
    size_t len_dir = strlen(full_path);

    while ((info = readdir(curr))) 
    {
        full_path[len_dir] = 0;
        strcat(full_path, info->d_name);
        // DT_DIR - for directories
        if (info->d_type == DT_DIR && strcmp(info->d_name, ".") && strcmp(info->d_name, "..")) 
        {
            files_count += bypassdir(full_path, dest_dir, outfile, files_count, N1, N2);
        }

        // DT_REG - for files
        if (info->d_type == DT_REG) 
        {
            struct stat file_stat;
            if (stat(full_path, &file_stat) == -1) 
            {
                fprintf(stderr,"%s : can''t get stat's from file - %s, %s \n", prog_name, full_path, strerror(errno));
                continue;
            }

            files_count++;

            if ((file_stat.st_size >= N1) && (file_stat.st_size <= N2))
            {
                fprintf(outfile, "%s   %ld \n", full_path, file_stat.st_size);
            }
        }
    }

    free(full_path);
    closedir(curr);
    return files_count;
}

int main(int argc, char *argv[])
{
    // files of nessesary size
    // name of caalog - first argument
    // size from N1 to N2 
    // output results of search into the file - fourth argument such as a full path, name of the file, its size
    //fprintf(stderr, "%s:", prog_name, strerror(errno))

    prog_name = basename(argv[0]);
    if (argc != 5) 
    {
        fprintf(stderr, "Program need 4 arguments %d \n", argc);
        return 1;
    }

    char *src_dir = argv[1];
    int N1 = atoi(argv[2]);
    int N2 = atoi(argv[3]);

    if (isdigit(N1) && isdigit(N2))
    {
        fprintf(stderr, "Uncorrect N1 and N2 \n");
        return 1;
    }
    else
    {
        fprintf(stderr, "Input arguments are correct \n");
    }
    
    char *dest_dir = argv[4];
    unsigned long long files_count = 0;
    FILE *outfile = fopen(dest_dir, "w");
    
    if (!outfile)
    {
        fprintf(stderr,"%s : can''t create or open file to write into him - %s, %s \n", prog_name, dest_dir, strerror(errno));
    }
    files_count += bypassdir(src_dir, dest_dir, outfile, files_count, N1, N2);
    fprintf(stderr,"The quality of files is %lld \n", files_count);
    fclose(outfile);
    return 0;
}