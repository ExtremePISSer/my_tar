#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
//https://www.gnu.org/software/tar/manual/html_node/Standard.html

//THIS PROJECT NEEDS TO SUBMITTED AS MAKEFILE!!!!
//DONT FORGET ABOUT README!
//format-hex archive.rar
//gcc my_tar.c -o my_tar
//./my_tar -c -f archive.tar file1.txt file2.txt

typedef struct Arguments{
    const char* mode;
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
  char pad[12];                 //padding to 512

}posix_header;

Arguments parse_arguments(int argc, char const *argv[]);
int create_archive(Arguments args);
int create_header(const char* filename, posix_header *header);
int write_header(int archiveFd, posix_header *header);
int write_file_contents(int archiveFd, const char* filename);
void calculate_checksum(posix_header *header);
int write_end_blocks(int archiveFd);
//HIII! :D :D Hello
int list_archive(const char *archiveName);
char *my_strcpy(char *dest, const char *src);
//int my_strlen(const char *string); For the future
void int_to_octal(char *dest, unsigned long value, int width);
int my_strcmp(const char *a, const char *b);
unsigned octal_to_int(char * dest);
int extract_archive(const char * archiveName);
int append_to_archive(const char * archiveName);
int update_archive(const char * archiveName, const char * filename);
int copy_file_contents(int oldFd, int newFd, unsigned size);
int copy_archive(const char *oldArchiveName, const char *newArchiveName);

int main(int argc, char const *argv[])
{
    Arguments args = parse_arguments(argc, argv);
    //./my_tar -cf archive.tar file1 file2
    //debug print:::::::::::::::::::::::::DELETE LATER
    //printf("Archive Name: %s \n",args.archiveName);
    //printf("Mode: %s \n",args.mode);
    //printf("Number of files: %d\n",args.numberOfFiles);
    
    int archiveFd;
    
    if(my_strcmp(args.mode, "CREATE") == 0) {
        archiveFd = create_archive(args);
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
        if((write_end_blocks(archiveFd)) == -1){
            exit(5);
        }
        close(archiveFd);
    } else if (my_strcmp(args.mode, "LIST") == 0) {
        archiveFd = list_archive(args.archiveName);
    } else if (my_strcmp(args.mode, "EXTRACT") == 0){
        archiveFd = extract_archive(args.archiveName);
    }else if (my_strcmp(args.mode, "APPEND")==0){
        archiveFd = append_to_archive(args.archiveName);

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
        if((write_end_blocks(archiveFd)) == -1){
            exit(5);
        }
        close(archiveFd);
    }else if(my_strcmp(args.mode, "UPDATE") == 0){
    for(int f = 0; f < args.numberOfFiles; f++){
        archiveFd = update_archive(
            args.archiveName,
            argv[args.indexFiles + f]
        );

        if(archiveFd == -1){
            exit(1);
        }
    }
}
    
    if(archiveFd == -1){
        exit(1);
    }

    //printf("%zu\n", sizeof(posix_header));
    return 0;
}


Arguments parse_arguments(int argc, char const *argv[]){
    Arguments args;
    args.numberOfFiles=0;
    args.mode = NULL;
    args.archiveName = NULL;
    args.indexFiles = 0;
    if(argc==1){
        //printf("erorr: no arguments present, please enter valid arguments\n");
        exit(1);
    }

    for(int i = 0;i<argc;i++){
        if(argv[i][0]=='-'){
            for(int y = 1;argv[i][y]!='\0';y++){
                switch (argv[i][y])
                {
                case 'x':
                    args.mode = "EXTRACT";
                    break;
                case 't':
                    args.mode = "LIST";
                    break;
                case 'c':
                    args.mode = "CREATE";
                    break;
                case 'r':
                    args.mode = "APPEND";
                    break;
                case 'u':
                    args.mode = "UPDATE";
                    break;
                case 'f':
                if(argv[i+1]==NULL){
                    //printf("erorr: no archive name provided\n");
                    exit(2);
                }
                    args.indexFiles= i+2;
                    args.numberOfFiles = argc - args.indexFiles;
                    args.archiveName = argv[i+1];
                    break;
                
                default:
                //printf("option was not recognized\n");
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
    my_strcpy(header->name,filename);
    int_to_octal(header->mode, sb.st_mode,8);
    int_to_octal(header->size, sb.st_size,12);
    int_to_octal(header->mtime, sb.st_mtime,12);
    int_to_octal(header->uid, sb.st_uid,8);
    int_to_octal(header->gid, sb.st_gid,8);
    my_strcpy(header->magic, "ustar");
    header->version[0] = '0';
    header->version[1] = '0';
    header->typeflag = '0';
    calculate_checksum(header);

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
void calculate_checksum(posix_header *header){
    for (int i = 0; i < 8; i++) {
        header->chksum[i] = ' ';
    }
    int sum = 0;
    
    unsigned char *p = (unsigned char *)header;
    for (int i = 0; i < sizeof(posix_header); i++) {
    sum += p[i];
    }
    int_to_octal(header->chksum, sum,7);
    header->chksum[7]=' ';
}
int write_end_blocks(int archiveFd){
    char zero[512] = {0};
    if((write(archiveFd,zero,512)) != 512){
        return -1;
    }
    if((write(archiveFd,zero,512)) != 512){
        return -1;
    }
    return 0;
}

int list_archive(const char *archiveName) {
    
    int fd = open(archiveName, O_RDONLY);
    
    if(fd == -1){
        exit(1);
    }
    
    while(1) {
        
        posix_header header;
        
        int bytesRead = read(fd, &header, sizeof(header));
        
        if(bytesRead == 0) break;
        
        if(header.name[0] == '\0') break;
        
        //printf("%s\n", header.name);
        
        int size = octal_to_int(header.size);
        
        size = 512 * ((size + 511) / 512);
        
        lseek(fd, size, SEEK_CUR);
        
    }
    
    
    close(fd);
    return 0;
}

char *my_strcpy(char *dest, const char *src) {
    char *start = dest;
    
    while(*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    
    return start;
}
/*
int my_strlen(const char *string) {
    int result = 0;

    while(*string) {
        result++;
        string++;
    }
    
    return result
}
*/

void int_to_octal(char *dest, unsigned long value, int width) {
    char temp[32] = {0};
    int i = 0;
    int j = 0;
    
    if(value == 0) {
        for (int w = 0; w < width - 2; w++) {
        dest[w] = '0';
        }
        dest[width - 2] = '0';
        dest[width - 1] = '\0';
        return;
    }
    
    while(value > 0) {
        temp[i] = value % 8 + '0';
        value /= 8;
        i++;
    }
    int padding = width - 1 - i;
    for (int w = 0; w < width - 1 - i; w++) {
        dest[w] = '0';
    }

    while(i > 0) {
        dest[j+padding] = temp[i - 1];
        i--;
        j++;
    }
    
    dest[j+padding] = '\0';
}

unsigned octal_to_int(char* dest){
    unsigned long result = 0;
    int index = 0;
    while(dest[index]!='\0'){
        result = result*8 + (dest[index] - '0');
        index++;
    }
    return result;
}

int my_strcmp(const char *a, const char *b) {
    
    while(*a && *b && *a == *b) {
        a++;
        b++;
    }
    if(*a == '\0' && *b == '\0') return 0;
    
    return 1;
}
int extract_archive(const char * archiveName){
    //will return 0 success, -1 failure
    int fd = open(archiveName, O_RDONLY);
    
    if(fd == -1){
        exit(1);
    }
    
    while (1) {
                
        /*
         archive
         ↓
         read header → header variable
         */
        posix_header header;

        int bytesRead = read(fd, &header, sizeof(header));
        if (bytesRead == 0) {
            break;
        }

        if (bytesRead != sizeof(header)) {
            close(fd);
            return -1;
        }

        if (header.name[0] == '\0') {
            break;
        }
         /*
         ↓
         get name + size
         */
        unsigned fileSize = octal_to_int(header.size);
        
        //printf("%s\n", header.name);
        
        //printf("size: %u\n", fileSize);
         /*
         ↓
         create file
         */
        unsigned mode = octal_to_int(header.mode);
        
        int outputFd = open(header.name, O_WRONLY | O_CREAT | O_TRUNC, mode);
                             
        if (outputFd == -1) {
            close(fd);
            return -1;
        }
         /*
         ↓
         read file data → write to disk
         */
        char buffer[512];
        unsigned remaining = fileSize;
        
        while (remaining > 0) {
            int bytesToRead;
            if (remaining >= 512) {
                bytesToRead = 512;
            } else {
                bytesToRead = remaining;
            }
            
            int bytesRead = read(fd, buffer, bytesToRead);
            
            if (bytesRead <= 0) {
                close(outputFd);
                close(fd);
                return -1;
            }
            
            write(outputFd, buffer, bytesRead);
            remaining -= bytesRead;
        }

         /*
         ↓
         skip padding
         */
        unsigned padding = (512 - (fileSize % 512)) % 512;
        lseek(fd, padding, SEEK_CUR);
         /*
         ↓
         read next header
         */
        
        
    }
    
    close(fd);
    return 0;
    
}
int append_to_archive(const char * archiveName){
    int fd = open(archiveName, O_RDWR);
    if(fd == -1){
        exit(1);
    }
    posix_header header;
    while(1){

        int bytesRead = read(fd,&header,sizeof(header));
                if(header.name[0] == '\0'){
                lseek(fd,-512,SEEK_CUR);
                break;
                }

        int size = octal_to_int(header.size);
        unsigned padding = (512 - (size % 512)) % 512;
        lseek(fd,size+padding,SEEK_CUR);
    }
    return fd;    
}
int update_archive(const char * archiveName, const char * filename){
    int fd = open(archiveName, O_RDONLY);
    int newFd = open("temp.tar", O_WRONLY | O_CREAT | O_TRUNC);
    
    if(fd == -1){
        exit(1);
    }
    posix_header header;
    struct stat sb;
    int statInt;
    statInt = stat(filename,&sb);
    char zeroBuffer[512] = {0};
    
    while(1){
        int bytesRead = read(fd,&header,sizeof(header));
        if(bytesRead==0){
            break;
        }
        if(my_strcmp(header.name,filename)==0){
            if(sb.st_mtime>octal_to_int(header.mtime)){
                int oldSize = octal_to_int(header.size);
                create_header(filename, &header);
                write_header(newFd, &header);
                write_file_contents(newFd, filename);
                unsigned padding = (512 - (oldSize % 512)) % 512;
                lseek(fd,oldSize+padding,SEEK_CUR);
            }else{
                write_header(newFd, &header);
                copy_file_contents(fd, newFd, octal_to_int(header.size));
                int size = octal_to_int(header.size);
        unsigned padding = (512 - (size % 512)) % 512;
        write(newFd, zeroBuffer, padding);
        lseek(fd, padding, SEEK_CUR);
            }
        }else{
        write_header(newFd, &header);
        copy_file_contents(fd, newFd, octal_to_int(header.size));
        int size = octal_to_int(header.size);
        unsigned padding = (512 - (size % 512)) % 512;
        write(newFd, zeroBuffer, padding);
        lseek(fd, padding, SEEK_CUR);
        }
        

    }
    write_end_blocks(newFd);
    close(fd);
    close(newFd);
    unlink(archiveName);
    copy_archive("temp.tar", archiveName);
    unlink("temp.tar");
    return 0;
}
int copy_file_contents(int oldFd, int newFd, unsigned size){
    int remaining = size;
    char buffer[512];
    int bytesToRead;

    while(remaining>0){
        if(remaining>512){
            bytesToRead = 512;
        }else{
            bytesToRead = remaining;
        }
        int bytesRead = read(oldFd,buffer,bytesToRead);
        write(newFd,buffer,bytesRead);
        remaining-=bytesRead;
    }
    return 0;
}
int copy_archive(const char *oldArchiveName, const char *newArchiveName){
    int oldFd = open(oldArchiveName, O_RDONLY);
    int newFd = open(newArchiveName, O_WRONLY | O_CREAT | O_TRUNC);
    char buffer[512];
    int bytesRead = read(oldFd,buffer,512);
    while(bytesRead>0){
        write(newFd,buffer,bytesRead);
        bytesRead = read(oldFd,buffer,512);
    }
    close(oldFd);
    close(newFd);
    return 0;
}