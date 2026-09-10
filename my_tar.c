#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct Arguments {
    const char* mode;
    const char* archiveName;
    int numberOfFiles;
    int indexFiles;
} Arguments;

typedef struct posix_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} posix_header;


Arguments parse_arguments(int argc, char const *argv[]);
void parse_option_string(const char *opt, char const *argv[], int i, int argc, Arguments *args);
int create_archive(Arguments args);
int create_header(const char* filename, posix_header *header);
int write_header(int archiveFd, posix_header *header);
int write_file_contents(int archiveFd, const char* filename);
void calculate_checksum(posix_header *header);
int write_end_blocks(int archiveFd);
int list_archive(const char *archiveName);
char *my_strcpy(char *dest, const char *src);
int my_strlen(const char *string);
void int_to_octal(char *dest, unsigned long value, int width);
int my_strcmp(const char *a, const char *b);
unsigned octal_to_int(char *dest);
int extract_archive(const char * archiveName);
int extract_file_data(int fd, unsigned fileSize, int outputFd);
int append_to_archive(const char * archiveName);
int update_archive(const char * archiveName, const char * filename);
int copy_file_contents(int oldFd, int newFd, unsigned size);
int copy_archive(const char *oldArchiveName, const char *newArchiveName);
int run_create_or_append(Arguments args, char const *argv[], bool is_append);

int main(int argc, char const *argv[]) {
    Arguments args = parse_arguments(argc, argv);
    int status = 0;

    if (my_strcmp(args.mode, "CREATE") == 0) {
        status = run_create_or_append(args, argv, false);
    } else if (my_strcmp(args.mode, "LIST") == 0) {
        status = list_archive(args.archiveName);
    } else if (my_strcmp(args.mode, "EXTRACT") == 0) {
        status = extract_archive(args.archiveName);
    } else if (my_strcmp(args.mode, "APPEND") == 0) {
        status = run_create_or_append(args, argv, true);
    } else if (my_strcmp(args.mode, "UPDATE") == 0) {
        for (int f = 0; f < args.numberOfFiles; f++) {
            status = update_archive(args.archiveName, argv[args.indexFiles + f]);
            if (status == -1) break;
        }
    }

    if (status == -1) exit(1);
    return 0;
}

Arguments parse_arguments(int argc, char const *argv[]) {
    Arguments args = {0, NULL, 0, 0};
    if (argc == 1) exit(1);
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            parse_option_string(argv[i], argv, i, argc, &args);
        }
    }
    return args;
}

void parse_option_string(const char *opt, char const *argv[], int i, int argc, Arguments *args) {
    for (int y = 1; opt[y] != '\0'; y++) {
        switch (opt[y]) {
            case 'x': args->mode = "EXTRACT"; break;
            case 't': args->mode = "LIST"; break;
            case 'c': args->mode = "CREATE"; break;
            case 'r': args->mode = "APPEND"; break;
            case 'u': args->mode = "UPDATE"; break;
            case 'f':
                if (argv[i + 1] == NULL) exit(2);
                args->archiveName = argv[i + 1];
                args->indexFiles = i + 2;
                args->numberOfFiles = argc - args->indexFiles;
                break;
            default: exit(3);
        }
    }
}

int run_create_or_append(Arguments args, char const *argv[], bool is_append) {
    int archiveFd = is_append ? append_to_archive(args.archiveName) : create_archive(args);
    if (archiveFd == -1) return -1;

    for (int f = 0; f < args.numberOfFiles; f++) {
        posix_header header = {0};
        if (create_header(argv[args.indexFiles + f], &header) == -1) exit(2);
        if (write_header(archiveFd, &header) == -1) exit(3);
        if (write_file_contents(archiveFd, argv[args.indexFiles + f]) == -1) exit(4);
    }
    if (write_end_blocks(archiveFd) == -1) exit(5);
    close(archiveFd);
    return 0;
}

int create_archive(Arguments args) {
    int fd = open(args.archiveName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return (fd < 0) ? -1 : fd;
}

int create_header(const char *filename, posix_header *header) {
    struct stat sb;
    if (stat(filename, &sb) == -1) {
        write(2, "my_tar: ", 8);
        write(2, filename, my_strlen(filename));
        write(2, ": Cannot stat: No such file or directory\n", 43);
        return -1;
    }
    my_strcpy(header->name, filename);
    int_to_octal(header->mode, sb.st_mode, 8);
    int_to_octal(header->size, sb.st_size, 12);
    int_to_octal(header->mtime, sb.st_mtime, 12);
    int_to_octal(header->uid, sb.st_uid, 8);
    int_to_octal(header->gid, sb.st_gid, 8);
    my_strcpy(header->magic, "ustar");
    header->version[0] = '0';
    header->version[1] = '0';
    header->typeflag = '0';
    calculate_checksum(header);
    return 0;
}

int write_header(int archiveFd, posix_header *header) {
    return write(archiveFd, header, sizeof(posix_header));
}

int write_file_contents(int archiveFd, const char* filename) {
    char buffer[512];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) return -1;

    int bytesRead, bytesWritten = 0;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        write(archiveFd, buffer, bytesRead);
        bytesWritten = bytesRead;
    }
    close(fd);
    char padding[512] = {0};
    if (bytesWritten != 0 && bytesWritten < 512) {
        write(archiveFd, padding, 512 - bytesWritten);
    }
    return 0;
}

void calculate_checksum(posix_header *header) {
    for (unsigned int i = 0; i < 8; i++) {
        header->chksum[i] = ' ';
    }
    int sum = 0;
    unsigned char *p = (unsigned char *)header;
    for (unsigned int i = 0; i < sizeof(posix_header); i++) {
        sum += p[i];
    }
    int_to_octal(header->chksum, sum, 7);
    header->chksum[7] = ' ';
}

int write_end_blocks(int archiveFd) {
    char zero[512] = {0};
    if (write(archiveFd, zero, 512) != 512) return -1;
    if (write(archiveFd, zero, 512) != 512) return -1;
    return 0;
}

int list_archive(const char *archiveName) {
    int fd = open(archiveName, O_RDONLY);
    if (fd == -1) exit(1);

    while (1) {
        posix_header header;
        int bytesRead = read(fd, &header, sizeof(header));
        if (bytesRead == 0 || header.name[0] == '\0') break;

        printf("%s\n", header.name);
        int size = octal_to_int(header.size);
        size = 512 * ((size + 511) / 512);
        lseek(fd, size, SEEK_CUR);
    }
    close(fd);
    return 0;
}

char *my_strcpy(char *dest, const char *src) {
    char *start = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return start;
}

int my_strlen(const char *string) {
    int result = 0;
    while (*string++) result++;
    return result;
}

void int_to_octal(char *dest, unsigned long value, int width) {
    char temp[32] = {0};
    int i = 0;
    if (value == 0) {
        for (int w = 0; w < width - 1; w++) dest[w] = '0';
        dest[width - 1] = '\0';
        return;
    }
    while (value > 0) {
        temp[i++] = (value % 8) + '0';
        value /= 8;
    }
    int padding = width - 1 - i;
    for (int w = 0; w < padding; w++) dest[w] = '0';
    for (int j = 0; i > 0; j++, i--) {
        dest[j + padding] = temp[i - 1];
    }
    dest[width - 1] = '\0';
}

unsigned octal_to_int(char *dest) {
    unsigned long result = 0;
    int index = 0;
    while (dest[index] != '\0' && dest[index] != ' ') {
        result = result * 8 + (dest[index] - '0');
        index++;
    }
    return result;
}

int my_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0') ? 0 : 1;
}

int extract_file_data(int fd, unsigned fileSize, int outputFd) {
    char buffer[512];
    unsigned remaining = fileSize;
    while (remaining > 0) {
        int bytesToRead = (remaining >= 512) ? 512 : remaining;
        int bytesRead = read(fd, buffer, bytesToRead);
        if (bytesRead <= 0) return -1;
        write(outputFd, buffer, bytesRead);
        remaining -= bytesRead;
    }
    return 0;
}

int extract_archive(const char *archiveName) {
    int fd = open(archiveName, O_RDONLY);
    if (fd == -1) exit(1);

    while (1) {
        posix_header header;
        int bytesRead = read(fd, &header, sizeof(header));
        if (bytesRead == 0 || header.name[0] == '\0') break;
        if (bytesRead != sizeof(header)) { close(fd); return -1; }

        unsigned fileSize = octal_to_int(header.size);
        int outputFd = open(header.name, O_WRONLY | O_CREAT | O_TRUNC, octal_to_int(header.mode));
        if (outputFd == -1 || extract_file_data(fd, fileSize, outputFd) == -1) {
            if (outputFd != -1) close(outputFd);
            close(fd);
            return -1;
        }
        close(outputFd);
        lseek(fd, (512 - (fileSize % 512)) % 512, SEEK_CUR);
    }
    close(fd);
    return 0;
}

int append_to_archive(const char *archiveName) {
    int fd = open(archiveName, O_RDWR);
    if (fd == -1) exit(1);
    posix_header header;
    while (1) {
        read(fd, &header, sizeof(header));
        if (header.name[0] == '\0') {
            lseek(fd, -512, SEEK_CUR);
            break;
        }
        int size = octal_to_int(header.size);
        lseek(fd, size + ((512 - (size % 512)) % 512), SEEK_CUR);
    }
    return fd;
}

int update_archive(const char *archiveName, const char *filename) {
    int fd = open(archiveName, O_RDONLY);
    int newFd = open("temp.tar", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1 || newFd == -1) exit(1);

    posix_header header;
    struct stat sb;
    stat(filename, &sb);
    char zeroBuffer[512] = {0};

    while (read(fd, &header, sizeof(header)) > 0) {
        int size = octal_to_int(header.size);
        unsigned padding = (512 - (size % 512)) % 512;
        if (my_strcmp(header.name, filename) == 0 && sb.st_mtime > octal_to_int(header.mtime)) {
            create_header(filename, &header);
            write_header(newFd, &header);
            write_file_contents(newFd, filename);
            lseek(fd, size + padding, SEEK_CUR);
        } else {
            write_header(newFd, &header);
            copy_file_contents(fd, newFd, size);
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

int copy_file_contents(int oldFd, int newFd, unsigned size) {
    unsigned remaining = size;
    char buffer[512];
    while (remaining > 0) {
        int bytesToRead = (remaining > 512) ? 512 : remaining;
        int bytesRead = read(oldFd, buffer, bytesToRead);
        write(newFd, buffer, bytesRead);
        remaining -= bytesRead;
    }
    return 0;
}

int copy_archive(const char *oldArchiveName, const char *newArchiveName) {
    int oldFd = open(oldArchiveName, O_RDONLY);
    int newFd = open(newArchiveName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buffer[512];
    int bytesRead;
    while ((bytesRead = read(oldFd, buffer, 512)) > 0) {
        write(newFd, buffer, bytesRead);
    }
    close(oldFd);
    close(newFd);
    return 0;
}
