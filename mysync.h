#ifndef MYSYNC_H
#define MYSYNC_H
#include "options.h"

// Function prototypes
void usage();

void debugPrintOptions(ProgramOptions opts);

ProgramOptions parseCommandLine(int argc, char* argv[]);

#endif