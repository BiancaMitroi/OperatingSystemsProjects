#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <fcntl.h>

void strmode(mode_t mode, char * buf) {
  const char chars[] = "rwxrwxrwx";
  for (int i = 0; i < 9; i++) {
    buf[i] = (mode & (1 << (8-i))) ? chars[i] : '-';
  }
  buf[9] = '\0';
}

int list(char* dirPath, bool recursive)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;
    dir = opendir(dirPath);
    if(dir == NULL)
    {
        printf("Could not open directory\n");
        return -1;
    }
    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", dirPath, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0)
            {
                if(S_ISDIR(statbuf.st_mode) && recursive == true)
                    list(fullPath, recursive);
                printf("%s\n", fullPath);
            }
        }

    }
    closedir(dir);
    return 0;
}

int listWithPermission(char* dirPath, bool recursive, char* permission)
{
    //printf("%s\n", dirPath);
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;
    dir = opendir(dirPath);
    if(dir == NULL)
    {
        printf("Could not open directory\n");
        return -1;
    }
    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", dirPath, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0)
            {
                char mode[10] = { 0 };
                strmode(statbuf.st_mode, mode);
                //printf("Permission of the current fine: %s\n", mode);
                if(S_ISDIR(statbuf.st_mode) && recursive == true)
                    listWithPermission(fullPath, recursive, permission);
                if(strcmp(mode, permission) == 0){
                    printf("%s\n", fullPath);
                }
            }
        }

    }
    closedir(dir);
    return 0;
}

int listWithSize(char* dirPath, bool recursive, long int size)
{
    //printf("%s\n", dirPath);
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512] = { 0 };
    struct stat statbuf;
    dir = opendir(dirPath);
    if(dir == NULL)
    {
        printf("Could not open directory\n");
        return -1;
    }
    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", dirPath, entry->d_name);
            //printf("%s\n", fullPath);
            if(lstat(fullPath, &statbuf) == 0)
            {
                if(S_ISDIR(statbuf.st_mode) && recursive == true)
                    listWithSize(fullPath, recursive, size);
                if(statbuf.st_size < size && !S_ISDIR(statbuf.st_mode)){
                    printf("%s\n", fullPath);
                }
            }
        }

    }
    closedir(dir);
    return 0;
}

int listWithPermissionsSize(char* dirPath, bool recursive, char* permissions, long int size)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;
    dir = opendir(dirPath);
    if(dir == NULL)
    {
        printf("Could not open directory\n");
        return -1;
    }
    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", dirPath, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0)
            {
                char mode[10] = { 0 };
                strmode(statbuf.st_mode, mode);
                if(S_ISDIR(statbuf.st_mode) && recursive == true)
                    listWithSize(fullPath, recursive, size);
                if(statbuf.st_size < size && strcmp(mode, permissions) == 0 && !S_ISDIR(statbuf.st_mode)){
                   printf("%s\n", fullPath);
                }
            }
        }

    }
    closedir(dir);
    return 0;
}

int parse(char* dirPath){

    int fd = open(dirPath + 5, O_RDONLY);
    lseek(fd, -4, SEEK_END);


    char magic[5] = { 0 };
    //char headerSizeChar[3] = { 0 };
    int headerSizeInt = 0;
    int version = 0;
    int sectionNumber = 0;


    read(fd, magic, 4);
    //printf("magic=%s\n", magic);

    //validare MAGIC
    if(strcmp(magic, "qGWQ") == 0){

        lseek(fd, -6, SEEK_END);
        read(fd, &headerSizeInt, 2);
        //printf("headerSize=%d\n", headerSizeInt);
        lseek(fd, -headerSizeInt, SEEK_END);
        read(fd, &version, 1);
       // printf("version=%d\n", version);

        //validare versiune
        if(45 <= version && version <= 145){

            read(fd, &sectionNumber, 1);
            //validare numar de sectiuni
            //printf("number of sections=%d\n", sectionNumber);
            if(2 <= sectionNumber && sectionNumber <= 15){

                bool ok = true;
                //int i = 0;
                for(int i = 0; i < sectionNumber; i++){

                    int sectionType = 0;
                    lseek(fd, 10, SEEK_CUR);
                    read(fd, &sectionType, 2);
                    //printf("section type=%d\n", sectionType);
                    if(sectionType != 31 && sectionType != 12 && sectionType != 47 && sectionType != 74 && sectionType != 48){
                        ok = false;
                        break;
                    }
                    lseek(fd, 8, SEEK_CUR);
                    //read(fd, &sectionType, 2);
                    //i++;
                }
                if(ok){
                    printf("SUCCESS\n");
                    printf("version=%d\n", version);
                    printf("nr_sections=%d\n", sectionNumber);
                    lseek(fd, -headerSizeInt + 2, SEEK_END);

                    for(int i = 0; i < sectionNumber; i++){
                        char sectionName[11] = { 0 };
                        read(fd, sectionName, 10);
                        int sectionType = 0;
                        read(fd, &sectionType, 2);
                        lseek(fd, 4, SEEK_CUR);
                        int sectionSize = 0;
                        read(fd, &sectionSize, 4);
                    printf("section%d: %s %d %d\n", i + 1, sectionName, sectionType, sectionSize);
                    //read(fd, &sectionType, 2);
                    //i++;
                }

                }else{
                    printf("ERROR\n wrong sect_types\n");
                    return -1;
                }

            }
            else{
                printf("ERROR\n wrong sect_nr\n");
                return -1;
            }
        }
        else{
            printf("ERROR\n wrong version\n");
            return -1;
        }
    }
    else{
        printf("ERROR\n wrong magic\n");
        return -1;
    }
    return 0;
}

int validate(char* dirPath){
    int fd = open(dirPath, O_RDONLY);
    lseek(fd, -4, SEEK_END);


    char magic[5] = { 0 };
    //char headerSizeChar[3] = { 0 };
    int headerSizeInt = 0;
    int version = 0;
    int sectionNumber = 0;


    read(fd, magic, 4);
    //printf("magic=%s\n", magic);

    //validare MAGIC
    if(strcmp(magic, "qGWQ") == 0){

        lseek(fd, -6, SEEK_END);
        read(fd, &headerSizeInt, 2);
        //printf("headerSize=%d\n", headerSizeInt);
        lseek(fd, -headerSizeInt, SEEK_END);
        read(fd, &version, 1);
       // printf("version=%d\n", version);

        //validare versiune
        if(45 <= version && version <= 145){

            read(fd, &sectionNumber, 1);
            //validare numar de sectiuni
            //printf("number of sections=%d\n", sectionNumber);
            if(2 <= sectionNumber && sectionNumber <= 15){

                bool ok = true;
                //int i = 0;
                for(int i = 0; i < sectionNumber; i++){

                    int sectionType = 0;
                    lseek(fd, 10, SEEK_CUR);
                    read(fd, &sectionType, 2);
                    //printf("section type=%d\n", sectionType);
                    if(sectionType != 31 && sectionType != 12 && sectionType != 47 && sectionType != 74 && sectionType != 48){
                        ok = false;
                        break;
                    }
                    lseek(fd, 8, SEEK_CUR);
                    //read(fd, &sectionType, 2);
                    //i++;
                }
                if(ok){
                    return 1;
                    // printf("SUCCESS\n");
                    // printf("version=%d\n", version);
                    // printf("nr_sections=%d\n", sectionNumber);
                    // lseek(fd, -headerSizeInt + 2, SEEK_END);

                    // for(int i = 0; i < sectionNumber; i++){
                    //     char sectionName[11] = { 0 };
                    //     read(fd, sectionName, 10);
                    //     int sectionType = 0;
                    //     read(fd, &sectionType, 2);
                    //     lseek(fd, 4, SEEK_CUR);
                    //     int sectionSize = 0;
                    //     read(fd, &sectionSize, 4);
                    // printf("section%d: %s %d %d\n", i + 1, sectionName, sectionType, sectionSize);
                    //read(fd, &sectionType, 2);
                    //i++;
                }else{
                    //printf("ERROR\n wrong sect_types\n");
                    return -1;
                }

            }
            else{
                //printf("ERROR\n wrong sect_nr\n");
                return -1;
            }
        }
        else{
            //printf("ERROR\n wrong version\n");
            return -1;
        }
    }
    else{
       // printf("ERROR\n wrong magic\n");
        return -1;
    }
    return 0;
}

int extract(char* path, int section, int line){

    int fd = open(path, O_RDONLY);
    lseek(fd, -6, SEEK_END);
    int headerSize = 0;
    read(fd, &headerSize, 2);
    //printf("headerSize=%d\n", headerSize);
    lseek(fd, -headerSize + 1, SEEK_END);
    int sectionNumber = 0;
    read(fd, &sectionNumber, 1);
    //printf("sectionNumber=%d\n", sectionNumber);
    lseek(fd, -headerSize + 2, SEEK_END);
    unsigned int offset = 0;
    unsigned int size = 0;
    for(int i = 0; i < sectionNumber; i++)
    {
        lseek(fd, 12, SEEK_CUR);
        if(i + 1 == section){
            read(fd, &offset, 4);
            read(fd, &size, 4);
            break;
        }
        else
            lseek(fd, 8, SEEK_CUR);
    }
    //printf("offset=%d\n", offset);
    //printf("size=%d\n", size);
    lseek(fd, offset, SEEK_SET);
    int i = 0;
    int currentLine = 1;
    char currentChar[3] = { 0 };
    printf("SUCCESS\n");
    while(i != size){

        read(fd, currentChar, 2);
        if(currentLine == line){
            if(currentChar[0] == 0x0d && currentChar[1] == 0x0a)
                break;
            printf("%c", currentChar[0]);
        }
        else if(currentChar[0] == 0x0d && currentChar[1] == 0x0a){
            currentLine++;
            lseek(fd, 1, SEEK_CUR);
            i++;
        }
        lseek(fd, -1, SEEK_CUR);
        i++;
    }
    return 0;
}

int findall(char* path){

    //printf("%s\n", path);
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512] = {0};
    struct stat statbuf;
    dir = opendir(path);
    if(dir == NULL)
    {
        printf("Could not open directory\n");
        return -1;
    }
    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0)
            {
                if(S_ISDIR(statbuf.st_mode))
                    findall(fullPath);
                else if(S_ISREG(statbuf.st_mode) && validate(fullPath) == 1){
                    int fd = open(fullPath, O_RDONLY);
                    lseek(fd, -6, SEEK_END);
                    int headerSize = 0;
                    read(fd, &headerSize, 2);
                    //printf("headerSize=%d\n", headerSize);
                    lseek(fd, -headerSize + 1, SEEK_END);
                    int sectionNumber = 0;
                    read(fd, &sectionNumber, 1);
                    //printf("sectionNumber=%d\n", sectionNumber);
                    lseek(fd, -headerSize + 2, SEEK_END);
                    unsigned int* offset = (unsigned int*)calloc(sectionNumber, sizeof(unsigned int));
                    unsigned int* size = (unsigned int*)calloc(sectionNumber, sizeof(unsigned int));
                    for(int i = 0; i < sectionNumber; i++)
                    {
                        lseek(fd, 12, SEEK_CUR);
                        read(fd, offset + i, 4);
                        read(fd, size + i, 4);
                    }
                    //printf("offset=%d\n", offset[1]);
                    //printf("size=%d\n", size[0]);
                    //bool ok = false;
                    for(int j = 0; j < sectionNumber; j++){
                        lseek(fd, offset[j], SEEK_SET);
                        int i = 0;
                        int currentLine = 1;
                        char currentChar[3] = { 0 };
                        //printf("SUCCESS\n");
                        while(i != size[j]){

                            read(fd, currentChar, 2);
                            if(currentChar[0] == 0x0D && currentChar[1] == 0x0A){
                                currentLine++;
                                lseek(fd, 1, SEEK_CUR);
                                i++;
                            }
                            lseek(fd, -1, SEEK_CUR);
                            i++;
                        }
                        if(currentLine == 13){
                            printf("%s\n", fullPath);
                            break;
                        }
                    }
                    free(offset);
                    free(size);
                    close(fd);
                }
            }
        }

    }
    
    closedir(dir);
    return 0;
}

int main(int argc, char **argv){
    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("25990\n");
            return 0;
        }
        else if(strcmp(argv[1], "list") == 0){
            bool recursive = false;
            char *path = NULL;
            long int sizeSmaller = 0;
            char permission[10] = { 0 };
            bool isPermissions = false, isSize = false;

            for(int i = 2; i < argc; i++){
                if(strstr(argv[i], "path=") == argv[i]){
                    path = (char*)calloc(strlen(argv[i] + 5), sizeof(char));
                    strcpy(path, argv[i] + 5);
                    //printf("%s\n", path);
                }
                else if(strcmp(argv[i], "recursive") == 0)
                    recursive = true;

                    else if(strstr(argv[i], "permissions=") == argv[i]){
                            isPermissions = true;
                            strcpy(permission, argv[i] + 12);
                            //printf("Permission of the main file: %s\n", permission);
                        }else if(strstr(argv[i], "size_smaller=") == argv[i]){
                            isSize = true;
                            sizeSmaller = atoi(argv[i] + 13);
                            //printf("size=%ld\n", sizeSmaller);
                        }
            }
            printf("SUCCESS\n");
            if(argc <= 4 && !isPermissions && !isSize)
                list(path, recursive);
                else if(argc >= 4){
                        if(isPermissions && !isSize)
                            listWithPermission(path, recursive, permission);
                        else if(isSize && !isPermissions)
                            listWithSize(path, recursive, sizeSmaller);
                            else if(isSize && isPermissions)
                                listWithPermissionsSize(path, recursive, permission, sizeSmaller);
                    }
            free(path);
        }
        else if(strcmp(argv[1], "parse") == 0){
            parse(argv[2]);
        }
        else if(strcmp(argv[1], "extract") == 0){
            extract(argv[2] + 5, atoi(argv[3] + 8), atoi(argv[4] + 5));
        }
        else if(strcmp(argv[1], "findall") == 0){
            printf("SUCCESS\n");
            findall(argv[2] + 5);
        }
    }
    else 
    {
        printf("Eroare: nu s-au scris parametrii de executie\n");
        return -1;
    }
    return 0;
}