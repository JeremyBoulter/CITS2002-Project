#include "mysync.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <regex.h>
#include <stdbool.h>

// Function to print standard useage
void usage() {
    printf("Usage: ./mysync [options] directory1 directory2 [directory3 ...]\n");
    printf("Options:\n");
    printf("  -a: Include hidden files\n");
    printf("  -n: Do not copy files (enables -v)\n");
    printf("  -p: Preserve metadata\n");
    printf("  -v: Verbose output\n");
    printf("  -r: Recursive (sync subdirectories)\n");
    printf("  -i [pattern]: Ignore matching\n");
    printf("  -o [pattern]: Only sync matching\n");
}

// Function to print debug information after parsing commandline arguements
void debugPrintOptions(ProgramOptions opts) {
    printf("=== DEBUG ENABLED ===\n");
    printf("=== Program Options ===\n");
    printf("  -a (Hidden Files): %s\n", opts.optionA ? "Enabled" : "Disabled");
    printf("  -n (NOT Copying (+v)): %s\n", opts.optionN ? "Enabled" : "Disabled");
    printf("  -p (Preserve Metadata): %s\n", opts.optionP ? "Enabled" : "Disabled");
    printf("  -v (Verbose Output): %s\n", opts.optionV ? "Enabled" : "Disabled");
    printf("  -r (Recursive): %s\n", opts.optionR ? "Enabled" : "Disabled");
    printf("  -i (Ignore Files): %s\n", opts.optionI ? "Enabled" : "Disabled");
    printf("  -o (Choose Files): %s\n", opts.optionO ? "Enabled" : "Disabled");

    printf("\n=== Directories To Sync ===\n");
    for (int i = 0; i < opts.numDirectories; i++) {
        printf("  %d: %s\n", i + 1, opts.directories[i]);
    }
    printf("=== Options Set ===\n\n");
}

// Function to validate a path and determine its type (1:directory 2:regular file)
int validatePath(const char* path) {
    struct stat statbuf;

    // Use stat to retrieve information about the path
    if (stat(path, &statbuf) == -1) {
        return 0; // Non-existent
    }

    if (S_ISDIR(statbuf.st_mode)) {
        return 1; // Directory
    }

    if (S_ISREG(statbuf.st_mode)) {
        return 2; // Regular file
    }

    return 0; // Non-existent (other file types or errors)
}

bool directoryContainsMatchingFiles(const char* directory, ProgramOptions opts) {
    DIR* dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", directory);
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Exclude files starting with "." unless optionA is set. 
        if ((entry->d_name[0] == '.' && !opts.optionA)) {
            continue;
        }

        char fullPath[PATH_MAX];
        snprintf(fullPath, PATH_MAX, "%s/%s", directory, entry->d_name);
        
        struct stat statbuf;
        if (stat(fullPath, &statbuf) == -1) {
            perror("Error getting file info");
            continue;
        }

        if (S_ISREG(statbuf.st_mode)) {
            if (opts.optionO) {
                for (int i = 0; i < opts.numConsiderPatterns; i++) {
                    if (matchesRegex(opts.considerPatterns[i], entry->d_name)) {
                        closedir(dir);
                        return true; // Found a matching file
                    }
                }
            } else {
                closedir(dir);
                return true; // If no pattern is set, any file is a match
            }
        } else if (S_ISDIR(statbuf.st_mode) && opts.optionR) {
            // Recursively check subdirectories
            if (directoryContainsMatchingFiles(fullPath, opts)) {
                closedir(dir);
                return true; // Found a matching file in a subdirectory
            }
        }
    }

    closedir(dir);
    return false; // No matching files found
}

// A function that returns the timestamp of a file at a given path
time_t getTimestamp(const char* filePath) {
    struct stat fileStat;

    if (stat(filePath, &fileStat) == 0) {
        return fileStat.st_mtime; // Modification timestamp
    }

    // If the stat operation fails, return a default timestamp (current time)
    return time(NULL);
}

// Function that returns the content to be synced between directories
SyncedContent* readFiles(char** directories, int numDirectories, ProgramOptions opts) {
    SyncedContent* content = (SyncedContent*)malloc(sizeof(SyncedContent));

    if (content == NULL) {
        perror("Memory allocation error");
        return NULL;
    }

    content->files = NULL;
    content->numFiles = 0;

    content->directories = NULL;
    content->numDirectories = 0;
    if(opts.optionV){printf("=== Reading Directories ===\n");}
    for (int i = 0; i < numDirectories; i++) {
        const char* path = directories[i];
        DIR* dir = opendir(path);

        if (dir == NULL) {
            fprintf(stderr, "Error opening directory: %s\n", path);
            continue;
        }

        if(opts.optionV){printf("Reading Directory: %s\n", path);}

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            //Ignore [.] and [..] outright
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".DS_Store") == 0){
                continue;}
            
            // Exclude files starting with "." unless optionA is set. 
            if ((entry->d_name[0] == '.' && !opts.optionA)) {
                continue;
            }
                
                   
            char fullPath[PATH_MAX];
            snprintf(fullPath, PATH_MAX, "%s/%s", path, entry->d_name);

            struct stat statbuf;
            if (stat(fullPath, &statbuf) == -1) {
                perror("Error getting file info");
                continue;
            }

            if (S_ISDIR(statbuf.st_mode)) {
                if(opts.optionV){printf("Found (SUB)directory: %s\n", entry->d_name);}
                // Handle subdirectories

                // Check if the directory already exists in the array
                int existingDirIndex = -1;
                for (int j = 0; j < content->numDirectories; j++) {
                    if (strcmp(content->directories[j].name, entry->d_name) == 0) {
                        existingDirIndex = j;
                        break;
                    }
                }

                if (existingDirIndex == -1) {
                    // Directory doesn't exist in the array, so add it
                    DirInfo dirInfo;
                    dirInfo.name = strdup(entry->d_name);
                    dirInfo.timestamp = statbuf.st_mtime;
                    dirInfo.permissions = statbuf.st_mode; // Store the directory permissions
                    dirInfo.path = strdup(fullPath);

                    // Increase the size of the directories array
                    DirInfo* temp = (DirInfo*)realloc(content->directories, (content->numDirectories + 1) * sizeof(DirInfo));
                    if (temp == NULL) {
                        perror("Memory allocation error");
                        free(dirInfo.name);
                        free(dirInfo.path);
                        break;
                    }

                    content->directories = temp;
                    content->directories[content->numDirectories] = dirInfo;
                    content->numDirectories++;
                } else {
                    // Directory with the same name already exists, replace it if more recent
                    if (statbuf.st_mtime > content->directories[existingDirIndex].timestamp) {
                        free(content->directories[existingDirIndex].name);
                        free(content->directories[existingDirIndex].path);
                        content->directories[existingDirIndex].name = strdup(entry->d_name);
                        content->directories[existingDirIndex].timestamp = statbuf.st_mtime;
                        content->directories[existingDirIndex].permissions = statbuf.st_mode;
                        content->directories[existingDirIndex].path = strdup(fullPath);
                    }
                }
            } else if (S_ISREG(statbuf.st_mode)) {
                if(opts.optionV){printf("Found file: %s\n", entry->d_name);}
                // Handle regular files

                // IF IGNORE FLAG (-i):
                // IF IGNORE_PATTERNS matches entry->d_name -> continue;
                if (opts.optionI) {
                    int shouldIgnore = 0;
                    for (int i = 0; i < opts.numIgnorePatterns; i++) {
                        if (matchesRegex(opts.ignorePatterns[i],entry->d_name)) {
                            shouldIgnore = 1;
                            if (opts.optionV) {
                                printf("Ignoring file %s due to matching pattern: %s\n", entry->d_name, opts.ignorePatterns[i]);
                            }
                            break;
                        }
                    }
                    if (shouldIgnore) continue;
                }
                // IF MATCH FLAG (-o):
                // IF MATCH_PATTERN does NOT match entry->d_name -> continue;
                if (opts.optionO) {
                    int shouldConsider = 0;
                    for (int i = 0; i < opts.numConsiderPatterns; i++) {
                        if (matchesRegex(opts.considerPatterns[i], entry->d_name)) {
                            shouldConsider = 1;
                            if (opts.optionV) {
                                printf("Selecting file %s due to matching pattern: %s\n", entry->d_name, opts.considerPatterns[i]);
                            }
                            break;
                        }
                    }
                    if (!shouldConsider) continue;
                }


                // Check if the file already exists in the array
                int existingFileIndex = -1;
                for (int j = 0; j < content->numFiles; j++) {
                    if (strcmp(content->files[j].name, entry->d_name) == 0) {
                        existingFileIndex = j;
                        break;
                    }
                }

                if (existingFileIndex == -1) {
                    // File doesn't exist in the array, so add it
                    FileInfo fileInfo;
                    fileInfo.name = strdup(entry->d_name);
                    fileInfo.timestamp = statbuf.st_mtime;
                    fileInfo.permissions = statbuf.st_mode; // Store the file permissions
                    fileInfo.path = strdup(fullPath);

                    // Increase the size of the files array
                    FileInfo* temp = (FileInfo*)realloc(content->files, (content->numFiles + 1) * sizeof(FileInfo));
                    if (temp == NULL) {
                        perror("Memory allocation error");
                        free(fileInfo.name);
                        free(fileInfo.path);
                        break;
                    }

                    content->files = temp;
                    content->files[content->numFiles] = fileInfo;
                    content->numFiles++;
                } else {
                    // File with the same name already exists, replace it if more recent
                    if (statbuf.st_mtime > content->files[existingFileIndex].timestamp) {
                        free(content->files[existingFileIndex].name);
                        free(content->files[existingFileIndex].path);
                        content->files[existingFileIndex].name = strdup(entry->d_name);
                        content->files[existingFileIndex].timestamp = statbuf.st_mtime;
                        content->files[existingFileIndex].permissions = statbuf.st_mode;
                        content->files[existingFileIndex].path = strdup(fullPath);
                    }
                }
            }
        }

        closedir(dir);
        if(opts.optionV){printf("\n");}
    }
    
    return content;
}

// Function to print debug information about the content to be synced
void debugPrintSyncedContent(SyncedContent* content) {
    if (content->numFiles == 0 && content->numDirectories == 0) {
        printf("No synced content found.\n");
        return;
    }

    printf("=== Items To Sync (Most Recent) ===\n");

    printf("Files:\n");
    for (int i = 0; i < content->numFiles; i++) {
        printf("%s (Timestamp: %ld, Permissions: %o)\n",
               content->files[i].path, content->files[i].timestamp,
               content->files[i].permissions);
    }
    printf("\n");
    printf("Directories:\n");
    for (int i = 0; i < content->numDirectories; i++) {
        printf("%s (Timestamp: %ld, Permissions: %o)\n",
               content->directories[i].path, content->directories[i].timestamp,
               content->directories[i].permissions);
    }
    printf("\n");
}

// Function to copy a file from source to destination while preserving metadata
int copyFileWithMetadata(const char* sourcePath, const char* destinationPath) {
    // Open the source file for reading
    int sourceFile = open(sourcePath, O_RDONLY);
    if (sourceFile == -1) {
        perror("Error opening source file");
        return 1;
    }

    // Create or open the destination file for writing
    int destinationFile = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (destinationFile == -1) {
        perror("Error opening destination file");
        close(sourceFile);
        return 1;
    }

    // Define a buffer to read and write data
    char buffer[1024];
    ssize_t bytesRead;

    // Copy data from the source file to the destination file
    while ((bytesRead = read(sourceFile, buffer, sizeof(buffer))) > 0) {
        if (write(destinationFile, buffer, bytesRead) == -1) {
            perror("Error writing to destination file");
            close(sourceFile);
            close(destinationFile);
            return 1;
        }
    }

    // Close the files
    close(sourceFile);
    close(destinationFile);

    // Retrieve the source file's metadata
    struct stat sourceInfo;
    if (stat(sourcePath, &sourceInfo) == -1) {
        perror("Error getting source file metadata");
        return 1;
    }

    // Set the destination file's metadata to match the source
    if (chmod(destinationPath, sourceInfo.st_mode) == -1) {
        perror("Error setting destination file permissions");
        return 1;
    }

    struct utimbuf ut;
    ut.actime = sourceInfo.st_atime;
    ut.modtime = sourceInfo.st_mtime;

    if (utime(destinationPath, &ut) == -1) {
    perror("Error setting file timestamp");
    }


    return 0;
}

// Function to copy a file from source to destination without preserving metadata
int copyFileWithoutMetadata(const char* sourcePath, const char* destinationPath) {
    // Open the source file for reading
    int sourceFile = open(sourcePath, O_RDONLY);
    if (sourceFile == -1) {
        perror("Error opening source file");
        return 1;
    }

    // Create or open the destination file for writing
    int destinationFile = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (destinationFile == -1) {
        perror("Error opening destination file");
        close(sourceFile);
        return 1;
    }

    // Define a buffer to read and write data
    char buffer[1024];
    ssize_t bytesRead;

    // Copy data from the source file to the destination file
    while ((bytesRead = read(sourceFile, buffer, sizeof(buffer))) > 0) {
        if (write(destinationFile, buffer, bytesRead) == -1) {
            perror("Error writing to destination file");
            close(sourceFile);
            close(destinationFile);
            return 1;
        }
    }

    // Close the files
    close(sourceFile);
    close(destinationFile);

    return 0;
}

// Function to create the destination path
char* createDestinationPath(const char* directory, const char* filename) {
    char* destinationPath = (char*)malloc(PATH_MAX);

    if (destinationPath == NULL) {
        perror("Memory allocation error");
        return NULL;
    }

    if (snprintf(destinationPath, PATH_MAX, "%s/%s", directory, filename) >= PATH_MAX) {
        fprintf(stderr, "Destination path exceeds PATH_MAX\n");
        free(destinationPath);
        return NULL;
    }

    return destinationPath;
}

// The main function to sync the selected content, with given options, on given directories
int syncFiles(SyncedContent* content, ProgramOptions opts) {
    int i, j;
    if (opts.optionN) {printf("=== Not Syncing ===\n");}
    if (!opts.optionN && opts.optionV) {printf("=== Syncing ===\n");}
    // Iterate through each directory specified in ProgramOptions
    for (i = 0; i < opts.numDirectories; i++) {
        const char* directory = opts.directories[i];
        if (opts.optionV) {printf("Syncing directory: %s\n", directory);}

        // Iterate through each unique/most recent file in SyncedContent
        for (j = 0; j < content->numFiles; j++) {
            FileInfo sourceFile = content->files[j];
            const char* destinationFilePath = createDestinationPath(directory, sourceFile.name);

            // If the file already exists in the directory
            if (validatePath(destinationFilePath) == 2) {

                // If the file is outdated 
                if (sourceFile.timestamp > getTimestamp(destinationFilePath)) {
                    if (!opts.optionN) {
                        if (opts.optionP) {
                            copyFileWithMetadata(sourceFile.path, destinationFilePath);
                        } else {
                            copyFileWithoutMetadata(sourceFile.path, destinationFilePath);
                        }
                    }
                    // Print syncing (updating) output
                    printf("Syncing %s to %s\n", sourceFile.path, directory);
                } else {

                    // Skip the file if it's not newer
                    if (opts.optionV) {
                        //printf("File %s is up to date.\n", sourceFile.name);
                    }
                }
            } else {
                // File doesn't exist, create it and copy the source file
                if (!opts.optionN) {
                    if (opts.optionP) {
                        copyFileWithMetadata(sourceFile.path, destinationFilePath);
                    } else {
                        copyFileWithoutMetadata(sourceFile.path, destinationFilePath);
                    }
                }
                // Print syncing (copying) output
                if (opts.optionV) {printf("Copying %s to %s\n", sourceFile.path, directory);}
            }

            
        }
        if (opts.optionV) {printf("\n");}
    }

    // Handle subdirectories if -r is set
    if (opts.optionR && content->numDirectories > 0) {
        if (opts.optionV) {printf("=== Recursing ===\n");}

        // For each subdirectory
        for (i = 0; i < content->numDirectories; i++) {
            char* subDirectoryPath = createDestinationPath(opts.directories[i], content->directories[i].name);
    
            // Add this check:
            if (!directoryContainsMatchingFiles(subDirectoryPath, opts)) {
                // This directory (or its subdirectories) doesn't contain any matching files
                // So we skip syncing it
                continue;
            }

            ProgramOptions newOpts;
            newOpts.optionA = opts.optionA; 
            newOpts.optionN = opts.optionN;
            newOpts.optionP = opts.optionP;
            newOpts.optionR = opts.optionR;
            newOpts.optionV = opts.optionV;
            newOpts.optionI = opts.optionI;
            newOpts.optionO = opts.optionO;
            newOpts.ignorePatterns = opts.ignorePatterns;
            newOpts.considerPatterns = opts.considerPatterns;
            newOpts.numIgnorePatterns = opts.numIgnorePatterns;
            newOpts.numConsiderPatterns = opts.numConsiderPatterns;

            newOpts.numDirectories = 1;
            newOpts.directories = malloc(sizeof(char*));
            newOpts.directories[0] = content->directories[i].path;

            const char* subDirectoryName = content->directories[i].name;
            if (opts.optionV) {printf("Syncing Subdirectory: %s\n\n", content->directories[i].path);}
            // For each parent directory
            for (j = 0; j < opts.numDirectories; j++) {
                const char* destinationDirectory = opts.directories[j];
                
                // Construct the path to the subdirectory (to be made if needed)
                char* subDirectoryPath = createDestinationPath(destinationDirectory, subDirectoryName);

                int subDirType = validatePath(subDirectoryPath);

                // If the subdirectory doesn't exist, create it
                if(subDirType == 0){
                    if (!opts.optionN){
                            // Make the subdirectory with default permissions
                            if (mkdir(subDirectoryPath, 0777) != 0) {
                                perror("Error creating directory");
                            }
                            
                            // Preserve metadata if -p is set
                            if (opts.optionP){
            
                                // Get source file info
                                struct stat sourceInfo;
                                if (stat(content->directories[i].path, &sourceInfo) == -1) {
                                    perror("Error getting source file metadata");
                                    return 1;
                                }

                                // Set the directory permissions to match the source
                                if (chmod(subDirectoryPath, sourceInfo.st_mode) == -1) {
                                    perror("Error setting destination directory permissions");
                                    return 1;
                                }

                                struct utimbuf ut;
                                ut.actime = sourceInfo.st_atime;
                                ut.modtime = sourceInfo.st_mtime;

                                // Set the directory timestamp to match the source
                                if (utime(subDirectoryPath, &ut) == -1) {
                                    perror("Error setting file timestamp");
                                }

                        }
                    }
                                                
                    if (opts.optionV) {printf("Could not find %s. Making Directory.\n", subDirectoryPath);}
                    
                    // Add the new subdirectory to newOpts
                    newOpts.numDirectories++;
                    newOpts.directories = realloc(newOpts.directories, newOpts.numDirectories * sizeof(char*));
                    newOpts.directories[newOpts.numDirectories - 1] = strdup(subDirectoryPath);
                }

                // Already exists (but is outdated)
                if(subDirType == 1 && content->directories[i].timestamp > getTimestamp(subDirectoryPath)){
                    // Add the new subdirectory to newOpts
                    newOpts.numDirectories++;
                    newOpts.directories = realloc(newOpts.directories, newOpts.numDirectories * sizeof(char*));
                    newOpts.directories[newOpts.numDirectories - 1] = strdup(subDirectoryPath);
                }

                if(subDirType == 2){
                    if (opts.optionV) {printf("Error: %s is a file, could not make a directory\n", subDirectoryPath);}
                }
            }

            // Debug message to print the subdirectories
            if(opts.optionV){
                printf("=== Subdirectories to be Synced ===\n");
                for (int i = 0; i < newOpts.numDirectories; i++) {
                    printf("%s\n", newOpts.directories[i]);
                }
            }
            // Call readFiles with updated opts to get the content of subdirectories
            SyncedContent* subdirContent = readFiles(newOpts.directories, newOpts.numDirectories, newOpts);

            // Call syncFiles to synchronize the subdirectories
            syncFiles(subdirContent, newOpts);

        }

    

    }

    return 0;
}


// Function to print debug information about pattern matching / ignoring
void debugPrintRegexPatterns(ProgramOptions opts) {
    printf("=== Regex Patterns ===\n");

    if (opts.optionI && opts.numIgnorePatterns > 0) {
        printf("=== Ignore Patterns ===\n");
        for (int i = 0; i < opts.numIgnorePatterns; i++) {
            printf("Regex -> %s\n", opts.ignorePatterns[i]);
        }
        printf("\n");
    }

    if (opts.optionO && opts.numConsiderPatterns > 0) {
        printf("=== Match Patterns ===\n");
        for (int i = 0; i < opts.numConsiderPatterns; i++) {
            printf("Regex -> %s\n", opts.considerPatterns[i]);
        }
        printf("\n");
    }
}

//Helper function to match regex 
int matchesRegex(const char *pattern, const char *string) {
    regex_t regex;
    int ret;
    
    ret = regcomp(&regex, pattern, REG_NOSUB | REG_EXTENDED);
    if (ret) {
        perror("Could not compile regex");
        exit(EXIT_FAILURE); // Compilation failed, treat as no match
    }
    
    ret = regexec(&regex, string, 0, NULL, 0);
    regfree(&regex);
    
    if (!ret) {
        return 1; // Match
    } else if (ret == REG_NOMATCH) {
        return 0; // No match
    } else {
        fprintf(stderr, "Regex match failed for pattern: %s\n", pattern); //No file has sequence
        exit(EXIT_FAILURE);
    }
}