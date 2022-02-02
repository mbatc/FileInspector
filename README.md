# FileInspector
Inspect your HDD for duplicate files.

> For my mother who has many duplicate files

Is currently very naive. It only finds duplicate filenames. I will extend it to check for
  - Similar filenames (e.g. my file.txt and my file - copy (1).txt could be considered a match)
  - Exact contents (easy enough to do by hashing the file contents, will need to detected and handle collisions though)
  - Similar contents (not sure what an efficient way of checking similarity would be, possibly something like frequency vectors? idk they are used for genetic sequences so could be something there)
