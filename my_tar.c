#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
//https://www.gnu.org/software/tar/manual/html_node/Standard.html

//THIS PROJECT NEEDS TO SUBMITTED AS MAKEFILE!!!!
//DONT FORGET ABOUT README!
//gcc my_tar.c -o my_tar
//./my_tar

typedef struct Arguments{
    const char* mode; //this will late probably become integer for simplicity(1=Create,2=somethingElse..) 
    const char* archiveName;
    int numberOfFiles;
    int indexFiles;
}Arguments;

typedef struct posix_header{    /*byte offset*/
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
}posix_header;

Arguments parse_arguments(int argc, char const *argv[]);
int create_archive(Arguments args);
int create_header(const char* filename, posix_header *header);
int write_header(int archiveFd, posix_header *header);
int write_file_contents(int archiveFd, const char* filename);


int main(int argc, char const *argv[])
{
    Arguments args = parse_arguments(argc, argv);
    //./my_tar -cf archive.tar file1 file2
    //debug print:::::::::::::::::::::::::DELETE LATER
    //printf("Archive Name: %s \n",args.archiveName);
    //printf("Mode: %s \n",args.mode);
    //printf("Number of files: %d\n",args.numberOfFiles);
    int archiveFd = create_archive(args);
    if(archiveFd == -1){
        exit(1);
    }
    //for loop for files:
    for(int f = 0; f<args.numberOfFiles;f++){
        posix_header header = {0}; //Initialize every byte of the struct to zero
        if((create_header(argv[args.indexFiles+f], &header))==-1){
        exit(2);
        }
        if((write_header(archiveFd, &header))==-1){
        exit(3);
        }
        if((write_file_contents(archiveFd, argv[args.indexFiles+f]))==-1){
            exit(4);
        }
    }
    
    close(archiveFd);

    return 0;
}


Arguments parse_arguments(int argc, char const *argv[]){
    Arguments args;
    args.numberOfFiles=0;
    args.mode = NULL;
    args.archiveName = NULL;
    args.indexFiles = 0;
    if(argc==1){
        printf("erorr: no arguments present, please enter valid arguments\n");
        exit(1);
    }

    for(int i = 0;i<argc;i++){
        if(argv[i][0]=='-'){
            for(int y = 1;argv[i][y]!='\0';y++){
                switch (argv[i][y])
                {
                case 'x':
                    printf("Extract to disk from the archive. If a file with the same name appears more than once in the archive, each copy will be extracted, with later copies overwriting (replacing) earlier copies.\n");
                    break;
                case 't':
                    printf(" -> List archive contents to stdout\n");
                break;
                case 'c':
                    args.mode = "CREATE";
                    break;
                case 'r':
                    printf("Like -c, but new entries are appended to the archive. The -f option is required.\n");
                    break;
                case 'u':
                    printf("Like -r, but new entries are added only if they have a modification date newer than the corresponding entry in the archive. The -f option is required.\n");
                    break;
                case 'f':
                if(argv[i+1]==NULL){
                    printf("erorr: no archive name provided\n");
                    exit(2);
                }
                    args.indexFiles= i+2;
                    args.numberOfFiles = argc - args.indexFiles;
                    args.archiveName = argv[i+1];
                    break;
                
                default:
                printf("option was not recognized\n");
                exit(3);
                    break;
                }
            }
        }
            
    }
    return args;
}

int create_archive(Arguments args){
    int fd = open(args.archiveName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd<0){
        return -1;
    }
    return fd;
}
int create_header(const char * filename, posix_header *header){
    struct stat sb;
    int statInt;
    statInt = stat(filename,&sb);
    if(statInt==-1){
        return -1;
    }
    //not sure if these are allowed, we may have to create our own variations of helper functions strcpy and sprintf
    strcpy(header->name,filename);
    sprintf(header->mode,"%o",sb.st_mode);
    printf("mode = %s\n", header->mode);
    return 0;
}
int write_header(int archiveFd, posix_header *header){
    return write(archiveFd, header, sizeof(posix_header));
}
int write_file_contents(int archiveFd, const char* filename){
    char buffer[512];
    int fd = open(filename,O_RDONLY);
    if(fd==-1){
        return -1;
    }
    int bytesRead;
    int bytesWritten=0;
    while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
        write(archiveFd, buffer, bytesRead);
            bytesWritten = bytesRead;
    }
    close(fd);
    char padding[512]={0};
    if(bytesWritten!=0 && bytesWritten < 512){
        write(archiveFd,padding,512-bytesWritten);
    }
    return 0;
}