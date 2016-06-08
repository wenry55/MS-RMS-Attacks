#### Prerequirement ####
- The RMS SDK 2.1 must be installed
    - https://www.microsoft.com/de-de/download/details.aspx?id=38396
- MS C++ Redistributable 2015 must be installed

#### Automatically unprotect ####
- To unprotect a file you need at least the view right
- To be able to unprotect the document 7Zip must be installed on the client machine
- The User must be able to open RMS files on the client machine the attack is executed on
- Rename the protected file into "protected.docx" and place it in the "examples" directory
- execute the "run-unprotect-attack.bat"

#### Manual unprotect ####
- Split the protected file (e.g. foobar.docx) in its parts (can be done on every computer must not be the company computer)
    - EncryptedPackage
    - [6]DataSpaces
    - [5]SummaryInformation
    - [5]DocumentSummaryInformation

- place the splitted files and directories in the "unprotected" directory (the dir with unprotect.exe in it)
- store the "unprotected" dir on a client machine with ledigt access to the RMS server
    - The company pc where you normaly access (open) your RMS documents
- execute unprotect.exe in the "unprotected" directory
