# MS-RMS-Attacks 

For an overview see:
https://web-in-security.blogspot.de/2016/07/how-to-break-microsoft-rights.html

We present two different attacks on Microsoft RMS:

1. Removing the RMS protection from a protected Word document resulting in a totally unprotected document. (decrypter)
2. Content modification of a RMS protectedWord document. (modification-attack)

Both attacks require only the *view-only* access right on the RMS protected file.
This is the minimal right, which can be assigned to a group or user in Microsoft RMS environment.

## Attack 1: Removing the RMS protection

For the first attack, we split the protected document (OLE compound file) into its components (RMS License and EncryptedPackage). This can be achieved, for example, by using 7zip. 
We created an attack tool that can be executed by every user of the domain.
The tool removes the protection automatically, without any further interaction and creates a copy of the processed RMS protected file, which contains the same content, formatting, etc, but without the RMS protection.

The steps execute by the tool are as following:

1. The tool reads in the publishing license and client licensor certificate.
2. It uses the certificates from the previous step to request the content key (from the use license) from the RMS server or the client licensor cache.
3. It reads the encrypted content bytes and
4. uses the RMS API function IpcDecrypt to decrypt the content bytes with the previously acquired content key.
5. The decrypted content bytes are written into a new unprotected file, which can later be opened, for example, by using Microsoft Word.

We extended the first attack to an even more severe one: the second attack makes use of the first attack and goes one step further. After removing the protection (cf. attack 1), we modify the unprotected content of the file.
We then reprotect the file, so that it looks as it would have been created by the original author of the protected file, but contains the content that we have just modified.

## Attack 2: Content modification with *view-only* access right

This attack has the same requirements as the first attack.
Suppose we have removed the protection of one file. We then modify the content of the file and proceed as follows:

1. We use the original protected file and extract the contained RMS License file.
2. Our tool then reads the manipulated and unprotected file that we want to embed in the protected file.
3. The tool reads in the publishing license and client licensor certificate from the files extracted in Step 1.
4. By using these certificates, our too requests the content key from the RMS server or the client cache.
5. The tool pads the read bytes from the unprotected file to fit the 16 byte block size of the encryption algorithm.
6. It then uses the RMS API function IpcEncrypt to encrypt the content bytes with the previous acquired content key.
7. The encrypted content bytes are written into a new file.
8. We finally replace the previously encrypted content with those contained in the original protected RMS file.

The tampered protected document can not be distinguished from the original protected document. 
It will look as it would have been created by the original author and only show the correct view access right for the
attacker.
This basically neglects the idea of the view-only RMS protection.

## Demo

- See the examples dir
