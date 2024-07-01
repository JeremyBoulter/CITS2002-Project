#include "mysync.h"
#include "options.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage();
        return 1;
    }

    ProgramOptions opts = parseCommandLine(argc, argv);

    if (opts.numDirectories < 2){
        usage();
        return 1;
    }
    
    if (opts.optionV == 1) {debugPrintOptions(opts);}

    if (opts.optionV && (opts.optionI || opts.optionO))  {debugPrintRegexPatterns(opts);}

    SyncedContent* content = readFiles(opts.directories, opts.numDirectories, opts);

    if (opts.optionV == 1){debugPrintSyncedContent(content);}
    
    return syncFiles(content, opts);
}
