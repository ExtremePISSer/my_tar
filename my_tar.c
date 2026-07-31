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
    const char* mode; //this will late probably become integer for simplicity(1=Create,2=somethingElse..) 
    const char* archiveName;
    int numberOfFiles;
    int indexFiles;
}Arguments;

Arguments parse_arguments(int argc, char const *argv[]);
int main(int argc, char const *argv[])
{
    Arguments args = parse_arguments(argc, argv);
    //./my_tar -cf archive.tar file1 file2
    //debug print:::::::::::::::::::::::::DELETE LATER
    printf("Archive Name: %s \n",args.archiveName);
    printf("Mode: %s \n",args.mode);
    printf("Number of files: %d\n",args.numberOfFiles);

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
                    printf("-> List archive contents to stdout\n");
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
