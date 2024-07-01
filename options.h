#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    int optionA; //hidden files
    int optionN; // don't copy (+v)
    int optionP; // preserve information
    int optionV; // verbose
    int optionR; // recursive
    int optionI; // Ignore matching
    int optionO; // Match with 
    
    char** ignorePatterns; // Stores regex data
    int numIgnorePatterns;
    char** considerPatterns; // Stores regex data
    int numConsiderPatterns;
    char** directories;
    int numDirectories;
    
} ProgramOptions;

ProgramOptions parseCommandLine(int argc, char* argv[]);

#endif
