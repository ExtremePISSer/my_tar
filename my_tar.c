#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
//https://www.gnu.org/software/tar/manual/html_node/Standard.html

//THIS PROJECT NEEDS TO SUBMITTED AS MAKEFILE!!!!
//DONT FORGET ABOUT README!
//gcc my_tar.c -o my_tar
//./my_tar

/*Your program must support:
-c Create archive
-r Append files
-u Update newer files
-t List contents
-x Extract
The specification also says:
-r and -u require -f.*/

typedef struct Arguments{
    const char* mode;
    const char* archiveName;
    int numberOfFiles;
    int indexFiles;
}Arguments;

Arguments parse_arguments(int argc, char const *argv[]);
int main(int argc, char const *argv[])
{
    Arguments args = parse_arguments(argc, argv);
    //./my_tar -cf archive.tar file1 file2
    //debug print:::::::::::::::::::::::::
    printf("Archive Name: %s \n",args.archiveName);
    printf("Mode: %s \n",args.mode);
    printf("Number of files: %d\n",args.numberOfFiles);

    return 0;
}


Arguments parse_arguments(int argc, char const *argv[]){
    Arguments args;
    args.numberOfFiles=0;
    args.mode;
    args.archiveName;
    args.indexFiles = 0;
    for(int i = 0;i<argc;i++){
        if(argv[i][0]=='-'){
            for(int y = 1;argv[i][y]!='\0';y++){
                switch (argv[i][y])
                {
                case 'c':
                    args.mode = "CREATE";
                    break;
                case 'f':
                    args.indexFiles= i+2;
                    args.numberOfFiles = argc - args.indexFiles;
                    args.archiveName = argv[i+1];
                    break;
                
                default:
                printf("mode was not recognized");
                    break;
                }
            }
        }
            
    }
    return args;
}
