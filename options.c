#include "options.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



ProgramOptions parseCommandLine(int argc, char* argv[]) {
    // Initialise options
    ProgramOptions opts = {0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0};
    int opt;
    
    // Initialise

    // Parse - Options
    while ((opt = getopt(argc, argv, "anpvri:o:")) != -1) {
        switch (opt) {
            case 'a':
                opts.optionA = 1;
                break;
            case 'n':
                opts.optionN = 1;
                opts.optionV = 1;
                break;
            case 'p':
                opts.optionP = 1;
                break;
            case 'v':
                opts.optionV = 1;
                break;
            case 'r':
                opts.optionR = 1;
                break;
            case 'i':
                opts.optionI = 1;
                char** newIgnorePatterns = realloc(opts.ignorePatterns, (opts.numIgnorePatterns + 1) * sizeof(char*));
                if (!newIgnorePatterns) {
                    perror("Memory allocation error for ignore patterns");
                    exit(EXIT_FAILURE);
                }
                opts.ignorePatterns = newIgnorePatterns;
                opts.ignorePatterns[opts.numIgnorePatterns] = glob2regex(optarg);  
                if (!opts.ignorePatterns[opts.numIgnorePatterns]) {
                    perror("Error converting glob to regex for ignore patterns");
                    exit(EXIT_FAILURE);
                }
                opts.numIgnorePatterns++;
                break;
            case 'o':
                opts.optionO = 1;
                char** newConsiderPatterns = realloc(opts.considerPatterns, (opts.numConsiderPatterns + 1) * sizeof(char*));
                if (!newConsiderPatterns) {
                    perror("Memory allocation error for consider patterns");
                    exit(EXIT_FAILURE);
                }
                opts.considerPatterns = newConsiderPatterns;
                opts.considerPatterns[opts.numConsiderPatterns] = glob2regex(optarg); 
                if (!opts.considerPatterns[opts.numConsiderPatterns]) {
                    perror("Error converting glob to regex for consider patterns");
                    exit(EXIT_FAILURE);
                }
                opts.numConsiderPatterns++;
                break;
            
        }
    }

    // Parse / Validate Directories
    int numDirectories = 0;
    char** directories = NULL;

    for (int i = optind; i < argc; i++) {
        int pathType = validatePath(argv[i]);
        if (pathType == 1) {
            // Valid directory path
            char* path_copy = strdup(argv[i]);
            if (path_copy == NULL) {
                // Handle memory allocation error
                perror("Memory allocation error");
                // Free previously allocated memory
                for (int j = 0; j < numDirectories; j++) {
                    free(directories[j]);
                }
                free(directories);
                exit(1); // Exit with an error
            }

            char** temp = realloc(directories, (numDirectories + 1) * sizeof(char*));
            if (temp == NULL) {
                // Handle memory reallocation error
                perror("Memory reallocation error");
                free(path_copy); // Free the path_copy
                // Free previously allocated memory
                for (int j = 0; j < numDirectories; j++) {
                    free(directories[j]);
                }
                free(directories);
                exit(1); // Exit with an error
            }

            directories = temp;
            directories[numDirectories] = path_copy;
            numDirectories++;
        } else {
            fprintf(stderr, "Error: %s is not a valid directory path.\n", argv[i]);
            exit(1); // Exit with an error
        }
    }

    // Update the 'opts' structure with the 'directories' and 'numDirectories'
    opts.directories = directories;
    opts.numDirectories = numDirectories;

    return opts;
}