#### Additional Information ####
- the executables are compiled for x86
- can be recompiled with the sources files available

#### Prerequirements ####
- The RMS SDK 2.1 must be installed
    - https://www.microsoft.com/de-de/download/details.aspx?id=38396
- MS C++ Redistributable 2015 must be installed

## Unprotect Attack: automatically

- To unprotect a file you need at least the view right
- To be able to unprotect the document 7Zip must be installed on the client machine
- The User must be able to open RMS files on the client machine the attack is executed on
- Rename the protected file into "protected.docx" and place it in the "examples" directory
- execute the "run-unprotect-attack.bat"
- MS Office should open with the unprotected document, store it somewhere you like
    - if MS Office does not opens the file (decrypted.docx) it should at least be stored in the "unprotected" directory
    - if the file is not available something went wrong *sorry*

## Unprotect Attack: manual

- Split the protected file (e.g. foobar.docx) in its parts (can be done on every computer must not be the company computer)
    - EncryptedPackage
    - [6]DataSpaces
    - [5]SummaryInformation
    - [5]DocumentSummaryInformation
- place the splitted files and directories in the "unprotected" directory (the dir with unprotect.exe in it)
- store the "unprotected" dir on a client machine with ledigt access to the RMS server
    - The company pc where you normaly access (open) your RMS documents
- execute unprotect.exe in the "unprotected" directory
- A file (decrypted.docx) should be created in the same directory

## Manipulation Attack: automatically

- Do the unprotect Attack
    - if the unprotect attack was successful go one
    - otherwise do the manual way for unprotect and manipulation attack
- Manipulate the document
    - if done via Office program the modification will be spotted via "Last Changes by User XYZ", etc. in Office
    - there are other ways we will not describe here
- place the manipulated document in the examples dir with filename "unprotected.docx"
    - if the file exist from unprotect attack manipulate this file or replace it
- execute "run-modification-attack.bat"
- A file with the name EncryptedPackage should show up in the "protected" dir
- Copy the original protected file (before you unprotect and modified it) and the newly generated EncryptedPackage file somewhere where you can execute another program
- download OpenMcdf
    - http://sourceforge.net/projects/openmcdf/files/OpenMcdf%202.x/OpenMcdf%202.0.zip/download
- Extract the dir and go into \bin\StructuredStorageeXplorer\
- Execute StructuredStorageExplorer.exe
- Open the original protected unmodified document
    - File -> Open
    - Change view to see all files, necessary to open docx files
- Uncollapse Root and right click on EncryptedPackae -> Import Data
- Choose the newly generated EncryptedPackage in the protected directory
- Now save the file under arbitrary name
- You should now be able to open the modified but again protected file with your original rights.

## Manipulation Attack: manual

- Do the unprotect Attack
    - if the unprotect attack was successful go one
    - otherwise do the manual way for unprotect
- Store the [6]DataSpaces dir from the manual unprotect attack in the "protected" dir
- Manipulate the document
    - if done via Office program the modification will be spotted via "Last Changes by User XYZ", etc. in Office
    - there are other ways we will not describe here
- Copy the modified unprotected docx in the "protected" dir under the name "decrypted.docx"
- Execute the "manipulation-attack.exe" file
- A file with the name EncryptedPackage should show up in the "protected" dir
- Copy the original protected file (before you unprotect and modified it) and the newly generated EncryptedPackage file somewhere where you can execute another program
- download OpenMcdf
    - http://sourceforge.net/projects/openmcdf/files/OpenMcdf%202.x/OpenMcdf%202.0.zip/download
- Extract the dir and go into \bin\StructuredStorageeXplorer\
- Execute StructuredStorageExplorer.exe
- Open the original protected unmodified document
    - File -> Open
    - Change view to see all files, necessary to open docx files
- Uncollapse Root and right click on EncryptedPackae -> Import Data
- Choose the newly generated EncryptedPackage in the protected directory
- Now save the file under arbitrary name
- You should now be able to open the modified but again protected file with your original rights.
