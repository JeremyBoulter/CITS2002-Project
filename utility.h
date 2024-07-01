#ifndef UTILITY_H
#define UTILITY_H
#include "options.h"
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    char* name;      
    time_t timestamp; 
    mode_t permissions; 
    char* path;      
} FileInfo;

typedef struct {
    char* name;      
    time_t timestamp;
    mode_t permissions; 
    char* path;      
} DirInfo;

typedef struct {
    FileInfo* files;   
    int numFiles;       
    DirInfo* directories; 
    int numDirectories; 
} SyncedContent;

//Function prototypes
void usage();

void debugPrintOptions(ProgramOptions opts);

int validatePath(const char* path);

time_t getTimestamp(const char* filePath);

SyncedContent* readFiles(char** directories, int numDirectories, ProgramOptions opts);

void debugPrintSyncedContent(SyncedContent* content);

int copyFileWithMetadata(const char* sourcePath, const char* destinationPath);

int copyFileWithoutMetadata(const char* sourcePath, const char* destinationPath);

char* createDestinationPath(const char* directory, const char* filename);

int syncFiles(SyncedContent* content, ProgramOptions opts);

char *glob2regex(char *glob);

void debugPrintRegexPatterns(ProgramOptions opts);

int matchesRegex(const char *pattern, const char *string);

bool directoryContainsMatchingFiles(const char* directory, ProgramOptions opts);

#endif
