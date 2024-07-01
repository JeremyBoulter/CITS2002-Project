# CITS2002-Project
Systems Programming Project 2 for 2023

The goal of this project was to design and develop a command-line utility program in C to synchronise the contents of two or more directories.

Program invocation:
prompt> ./mysync  [options]  directory1  directory2  [directory3  ...]

The program's options are:
-v : Verbose output to stdout.

-r : Synchronise subdirectories (enable recursion).

-a : Synchronise hidden files (starting with .).

-n : Does't actually perform the sync (also enables -v).

-p : Preserve timestamp / permissions when synchronising.

-o $ : Only filenames matching the pattern $ will be synchronised.

-i $ : Filenames matching pattern $ will be ignored.

Note that, because the shell expands wildcards, that you'll need to enclose your file patterns within single-quotation characters. For example, the following command will (only) synchronise your C11 files:
prompt> ./mysync  -o  '*.[ch]'  ....

The project spec can be found in full at: https://teaching.csse.uwa.edu.au/units/CITS2002/past-projects/p2023-2/summary.php

This project recieved a final mark of 96%.
