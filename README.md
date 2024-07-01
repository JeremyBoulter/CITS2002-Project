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
