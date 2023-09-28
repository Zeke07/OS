# P1 - Unix Utilities
### CSL: zkhan
### ID: 9083145236 
### Email: zakhan5@wisc.edu
### Status: Passes all given tests

## **wman**
- If one other argument is provided, the implementation will traverse all sub-directories of man_pages manually to check 
for an entry file matching the user argument.  If found, the file contents are outputted.  Otherwise, a fail message
will be printed and the program will exit succesfully.

- If two (or more, in which case we ignore dangling arguments) other arguments are provided, ensure the section number is 
within the correct range and that the filename format is valid, exit with code 1 if neither is valid.  If so, manually open the directory of specified section 
number and search for the file entry.  If it exists, output the contents, otherwise fail message as before. Successfully
exit with code 0.

## **wapropos**
- Safely exit if no argument specified 
- Otherwise, iterate manually over man_page sub-directories and the respective file contents.  The dirent.h C library 
is used to accomplish this goal.  We attempt to open the man<number> directory.  If successfully, read each known file of
the format <filename>.<section> to ensure we are reading only manual pages.  Search any line contained within the name 
and description for the presence of keyword.  If found, list the file and proceed to the next one.  If no file contains 
the keyword, output a single message.  Exit successfully with code 0
unless a file open arbitrarily fails.

## **wgroff**
- Safely exit if no argument specified
- Attempt to open file otherwise, proceed if it exists.
Read the first header line.  Proceed in stages using a switch-case to ensure 
we have read a '.TH' header, a filename, a valid section number, and a valid date with the format YYYY-MM-DD with values
in the correct range.  Upon success, load the date into a buffer and format the first line of the converted file using.
We left-right justify the header by padding with spaces based on 80-column alignment requirement.
Continue to read the remainder of the file we are reading.  Ignore and commented lines (starting with #), add any line
that begins with .SH as a header in the newfile with the desired bold foramt.  For any plain-text line
  (assumed to be contained within a section), indent using 7 spaces as padding, replace any special characters (such as "\fB") 
as needed.  To achieve this effectively, plain-text lines are added character-by-character.
Once every line is read into the file, add the date, left-right justified in the same manner as before.  
- For any formatting errors caught, return an error message indicating the line and exit with code 0


 