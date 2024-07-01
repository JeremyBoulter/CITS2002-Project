# CITS2002-Project
Systems Programming Project 2 for 2023

The goal of this project is to design and develop a command-line utility program, named mysync, to synchronise the contents of two or more directories.

Program invocation

Your program will be invoked from the command-line with zero-or-more options (switches) and two-or-more directory names:
prompt> ./mysync  [options]  directory1  directory2  [directory3  ...]

The program's options are:

-a	By default, 'hidden' and configuration files (those beginning with a '.' character) are not synchronised because they do not change very frequently. The -a option requests that all files, regardless of whether they begin with a '.' or not, should be considered for synchronisation.

-i pattern	Filenames matching the indicated pattern should be ignored; all other (non-matching) filenames should be considered for synchronisation. The -i option may be provided multiple times.

-n	By default, mysync determines what files need to be synchronised and then silently performs the necessary copying. The -n option requests that any files to be copied are identified, but they are not actually synchronised. Setting the -n option also sets the -v option.

-o pattern	Only filenames matching the indicated pattern should be considered for synchronisation; all other (non-matching) filenames should be ignored. The -o option may be provided multiple times.

-p	By default, after a file is copied from one directory to another, the new file will have the current modification time and default file permissions. The -p option requests that the new copy of the file have the same modification time and the same file permissions as the old file.

-r	By default, only files found immediately within the named directories are synchronised. The -r option requests that any directories found within the named directories are recursively processed.

-v	By default, mysync performs its actions silently. No output appears on the stdout stream, although some errors may be reported on the stderr stream. The -v option requests that mysync be more verbose in its output, reporting its actions to stdout. There is no required format for the (your) debug printing (it will be ignored during marking).