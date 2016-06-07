#include "stdafx.h"


using namespace System::IO;
using namespace System::Text;
using namespace System;
using namespace System::Runtime::InteropServices;



IPC_KEY_HANDLE      m_key = nullptr;
PIPC_BUFFER         m_pLicense = new IPC_BUFFER;
PIPC_TEMPLATE_INFO  m_pTemplateInfo = nullptr;
int MAX_LOADSTRING = 200;
unsigned int contentSizeNoPadding;

#define IPCNP_ENC_TEMPLATE_ID_PARAM         L"The template id parameter is invalid." 
#define IPCNP_ENC_UNENCRYPTED_STREAM_PARAM  L"The unencrypted stream parameter is invalid." 

#using <mscorlib.dll>

System::Void Encrypt(array<Byte> ^);
System::Void EncryptStream(Stream ^);
System::Void readDecryptedContent(Stream ^);
System::Void encryptUnencryptedStream(Stream ^);


/// <summary>
/// Converts the HRESULT to message
/// </summary>
/// <param name="hr">hr - HRESULT</param>
String^ GetErrorMessageFromHRESULT(HRESULT hr)
{
	String^ errorString = nullptr;
	LPWSTR buffer = NULL;
	if (0 != FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&buffer,
		MAX_LOADSTRING,
		NULL))
	{
		errorString = gcnew String(buffer);
	}
	else
	{
		errorString = gcnew String("");
	}
	if (buffer = NULL)
	{
		LocalFree(buffer);
	}
	return errorString;
}

/// <summary>
/// Turn an HRESULT into an IpcException and throw it
/// </summary>
/// <param name="hr">hr - error code value</param>
System::Void CheckAndHandleError(HRESULT hr)
{
	if (FAILED(hr))
	{
		LPCWSTR pwszErrorMessage = nullptr;
		String^ message;
		HRESULT localHr;

		localHr = IpcGetErrorMessageText(hr,
			0,
			&pwszErrorMessage);

		if (FAILED(localHr))
		{
			message = GetErrorMessageFromHRESULT(hr);
		}
		else
		{
			message = gcnew String(pwszErrorMessage);
		}

		Console::WriteLine(message);

		if (pwszErrorMessage != nullptr)
		{
			IpcFreeMemory((LPVOID)pwszErrorMessage);
		}
	}
}


System::Void Encrypt(array<Byte> ^decryptedBytes)
{
	MemoryStream    ^decryptedStream = gcnew MemoryStream(decryptedBytes);

	EncryptStream(decryptedStream);

	decryptedStream->Close();
	decryptedStream = nullptr;

}


System::Void EncryptStream(Stream ^decryptedStream)
{
	pin_ptr<IPC_KEY_HANDLE>     pKey;
	HRESULT                     hr;

	// Contentkey aus der Lizenz holen

	pKey = &m_key;
	hr = IpcGetKey(m_pLicense,
		0,
		nullptr,
		nullptr,
		pKey);
	CheckAndHandleError(hr);

	pin_ptr<PIPC_TEMPLATE_INFO>     ppTemplateInfo;
	ppTemplateInfo = &m_pTemplateInfo;
	hr = IpcGetSerializedLicenseProperty(m_pLicense,
		IPC_LI_DESCRIPTOR,
		m_key,
		0,
		(LPVOID *)ppTemplateInfo);
	CheckAndHandleError(hr);
	encryptUnencryptedStream(decryptedStream);
}



System::Void readDecryptedContent(Stream ^decryptedStream)
{
	DWORD           cbWritten;
	DWORD           *pcbBlockSize;
	DWORD           cbReadRemaining;
	DWORD           cbEncrypted;
	DWORD           cbOutputBuffer;
	FileStream      ^saveFile;
	array<Byte>     ^readBuffer;
	array<Byte>     ^writeBuffer;
	pin_ptr<Byte>   pbReadBuffer;
	pin_ptr<unsigned char>   pbWriteBuffer;
	int             cBlock;
	HRESULT         hr;
	MemoryStream    ^encryptedStream;
	std::ofstream outFile;
	String ^fileName;

	// Get Block Size
	hr = IpcGetKeyProperty(m_key,
		IPC_KI_BLOCK_SIZE,
		nullptr,
		(LPVOID *)&pcbBlockSize);

	CheckAndHandleError(hr);

//	pcbBlockSize = 16;
	fileName = "EncryptedPackage";
	saveFile = gcnew FileStream(fileName, FileMode::Create, FileAccess::ReadWrite, FileShare::None);
	//outFile.open(fileName.c_str(), std::ios_base::binary);
	
	try
	{

		readBuffer = gcnew array<Byte>(*pcbBlockSize);
		pbReadBuffer = &readBuffer[0];

//		cbOutputBuffer = *pcbBlockSize;
//		writeBuffer = gcnew array<Byte>(cbOutputBuffer);
//		pbWriteBuffer = &writeBuffer[0];

		hr = IpcEncrypt(m_key,
			0,
			true,
			pbReadBuffer,
			*pcbBlockSize,
			nullptr,
			0,
			&cbOutputBuffer);

		CheckAndHandleError(hr);

		writeBuffer = gcnew array<Byte>(cbOutputBuffer);
		pbWriteBuffer = &writeBuffer[0];

		// allocate space for encrypted stream
		cbReadRemaining = (DWORD)decryptedStream->Length;
		encryptedStream = gcnew MemoryStream();

		Marshal::Copy((IntPtr)(&cbReadRemaining), writeBuffer, 0, sizeof(DWORD));
		encryptedStream->Write(writeBuffer, 0, sizeof(DWORD));

		cBlock = 0;
		while (cbReadRemaining > *pcbBlockSize)
		{
			cbWritten = 0;
			decryptedStream->Read(readBuffer, 0, *pcbBlockSize);
			hr = IpcEncrypt(m_key,
				cBlock,
				false,
				pbReadBuffer,
				*pcbBlockSize,
				pbWriteBuffer,
				cbOutputBuffer,
				&cbWritten);
			CheckAndHandleError(hr);

			cBlock++;

			encryptedStream->Write(writeBuffer, 0, cbWritten);
			cbReadRemaining -= *pcbBlockSize;
		}

		cbWritten = 0;

		readBuffer = gcnew array<Byte>(cbReadRemaining);
		pbReadBuffer = &readBuffer[0];

		// get required size for the last block with padding
		hr = IpcEncrypt(m_key,
			cBlock,
			true,
			pbReadBuffer,
			*pcbBlockSize,
			nullptr,
			cbOutputBuffer,
			&cbWritten);

		writeBuffer = gcnew array<Byte>(cbOutputBuffer);
		pbWriteBuffer = &writeBuffer[0];

		cbWritten = 0;
		decryptedStream->Read(readBuffer, 0, cbReadRemaining);
		hr = IpcEncrypt(m_key,
			cBlock,
			true,
			pbReadBuffer,
			*pcbBlockSize,
			pbWriteBuffer,
			cbOutputBuffer,
			&cbWritten);
		CheckAndHandleError(hr);

		encryptedStream->Write(writeBuffer, 0, cbWritten);
		encryptedStream->Flush();
		encryptedStream->Seek(0, SeekOrigin::Begin);
		saveFile->Write(encryptedStream->GetBuffer(), 0, (int)encryptedStream->Length);
		saveFile->Flush();
		saveFile->Close();
		//outFile.close();
	}
	finally
	{
		readBuffer = nullptr;
		writeBuffer = nullptr;
		IpcFreeMemory((LPVOID)pcbBlockSize);
	}

}

System::Void encryptUnencryptedStream(Stream ^unencryptedStream)
{
	DWORD           cbWritten;
	DWORD           *pcbBlockSize;
	DWORD           cbReadRemaining;
	DWORD           cbEncrypted;
	DWORD           cbOutputBuffer;
	FileStream      ^saveFile;
	array<Byte>     ^readBuffer;
	array<Byte>     ^writeBuffer, ^staticBytes;
	pin_ptr<Byte>   pbReadBuffer;
	pin_ptr<unsigned char>   pbWriteBuffer;
	int             cBlock;
	HRESULT         hr;
	MemoryStream    ^encryptedStream;
	std::ofstream outFile;
	String ^fileName;

	encryptedStream = gcnew MemoryStream();

	// set encryption mode
	//DWORD dwEncryptionMode = IPC_ENCRYPTION_PACKAGE_AES128_CBC4K;
	/*DWORD dwEncryptionMode = IPC_ENCRYPTION_PACKAGE_AES128_ECB;
	hr = IpcSetLicenseProperty(hLicense,
		false,
		IPC_LI_PREFERRED_ENCRYPTION_PACKAGE,
		&dwEncryptionMode);

	*/

	// Get Block Size
	hr = IpcGetKeyProperty(m_key,
		IPC_KI_BLOCK_SIZE,
		nullptr,
		(LPVOID *)&pcbBlockSize);

	CheckAndHandleError(hr);

	//	pcbBlockSize = 16;
	fileName = "EncryptedPackage";
	saveFile = gcnew FileStream(fileName, FileMode::Create, FileAccess::ReadWrite, FileShare::None);
	//outFile.open(fileName.c_str(), std::ios_base::binary);

	try
	{
		// allocate read & write buffers for encrypting a block at a time,
		// and pinned pointers for passing to the IPC API

		readBuffer = gcnew array<Byte>(*pcbBlockSize);
		pbReadBuffer = &readBuffer[0];

		// MSIPC will handle the last block padding for us.  When the
		// input buffer is exactly a blocksize in length, the encrypted data will
		// be larger than the clear text data, as a result of padding, so we query 
		// IpcEncrypt for this case to determine the maximum output buffer size 
		// needed, to avoid a reallocate later

		hr = IpcEncrypt(m_key,
			0,
			true,
			pbReadBuffer,
			*pcbBlockSize,
			nullptr,
			0,
			&cbOutputBuffer);

		// now we know the maximum output buffer size

		writeBuffer = gcnew array<Byte>(cbOutputBuffer);
		pbWriteBuffer = &writeBuffer[0];

		// how many bytes are we going to encrypt?
		cbReadRemaining = (DWORD)unencryptedStream->Length;
		

		
		//Marshal::Copy((IntPtr)(&cbReadRemaining), writeBuffer, 0, sizeof(DWORD));
		// write the actual size of the unprotected file in the first 4 byte of the Encrypted Package
		Marshal::Copy((IntPtr)(&contentSizeNoPadding), writeBuffer, 0, sizeof(DWORD));
		encryptedStream->Write(writeBuffer, 0, sizeof(DWORD));

		// the length field is 8 byte long, so we have to add 4 bytes.
		staticBytes = gcnew array<Byte>(4);
		for (unsigned int i = 0; i < 4; i++)
			staticBytes[i] = 0;
		encryptedStream->Write(staticBytes, 0, 4);

		// encrypt one block at a time, handling the last block to allow for padding
		// as necessary

		cBlock = 0;
		while (cbReadRemaining > *pcbBlockSize)
		{
			cbWritten = 0;
			unencryptedStream->Read(readBuffer, 0, *pcbBlockSize);
			hr = IpcEncrypt(m_key,
				cBlock,
				false,
				pbReadBuffer,
				*pcbBlockSize,
				pbWriteBuffer,
				cbOutputBuffer,
				&cbWritten);
			CheckAndHandleError(hr);

			cBlock++;

			encryptedStream->Write(writeBuffer, 0, cbWritten);
			cbReadRemaining -= *pcbBlockSize;
		}

		// final block, so pass in exactly as many bytes as remain
		// and let MSIPC do the padding.  The output buffer has been
		// preallocated to handle the largest possible size so 
		// no need to query here

		cbWritten = 0;
		unencryptedStream->Position;
		unencryptedStream->Read(readBuffer, 0, cbReadRemaining);

		for (unsigned int i = 0; i < 16; i++)
		{
			Console::Write((unsigned int)readBuffer[i]);
			Console::Write(" ");
		}
		Console::WriteLine(" ");

		hr = IpcEncrypt(m_key,
			cBlock,
			true,
			pbReadBuffer,
			cbReadRemaining,
			pbWriteBuffer,
			cbOutputBuffer,
			&cbWritten);
		CheckAndHandleError(hr);

		for (unsigned int i = 0; i < 16; i++)
		{
			Console::Write((unsigned int)writeBuffer[i]);
			Console::Write(" ");
		}
		Console::WriteLine(" ");
		encryptedStream->Write(writeBuffer, 0, cbWritten);
		encryptedStream->Flush();
		encryptedStream->Seek(0, SeekOrigin::Begin);
		saveFile->Write(encryptedStream->GetBuffer(), 0, (int)encryptedStream->Length);
		saveFile->Flush();
		saveFile->Close();
		//outFile.close();
	}
	finally
	{
		readBuffer = nullptr;
		writeBuffer = nullptr;
		IpcFreeMemory((LPVOID)pcbBlockSize);
	}
}

int __cdecl main()
{
	HRESULT     hr;
	hr = IpcInitialize();

	std::streampos unencryptedContentSize;
	char * license;
	char xml_size_byte1;
	char xml_size_byte2;
	char byte_read;
	char lastbyte;
	int xml_size;
	byte byte1 = 0xEF;
	byte byte2 = 0xBB;
	byte byte3 = 0xBF;
	int i, nullbytecounter = 0;
	array<Byte> ^unencryptedContentBytes;
	String ^output;

	// size of the license
	int licenseSize;

	// Pfad der Datei, die die Lizenz enthaelt
	std::ifstream file("[6]DataSpaces\\TransformInfo\\DRMEncryptedTransform\\[6]Primary", std::ios::in, std::ios::binary);
	if (file.is_open())
	{
		// Metadaten ueberspringen
		file.seekg(172);
		// Laenge der Lizenz einlesen (2Bytes)
		file.read(&xml_size_byte1, 1);
		file.read(&xml_size_byte2, 1);


		file.seekg(0, std::ios::end);
		xml_size = file.tellg();

		// Die Datei mit der Lizenz kann manchmal Nullbytes enthalten
		// TODO: Warum? Warum nur manchmal?
		file.seekg(xml_size - 1);
		file.read(&lastbyte, 1);
		// TODO: Könnte man das nicht eleganter in eine Schleife stecken?
		unsigned int d = 1;
		while (lastbyte != '>')
		{
			nullbytecounter++;
			d++;
			file.seekg(xml_size - d);
			file.read(&lastbyte, 1);
		}
	/*	if (lastbyte != '>')
		{
			nullbytecounter++;
			file.seekg(xml_size - 2);
			file.read(&lastbyte, 1);
			if (lastbyte != '>')
			{
				nullbytecounter++;
				file.seekg(xml_size - 3);
				file.read(&lastbyte, 1);
				if (lastbyte != '>')
				{
					nullbytecounter++;
					file.seekg(xml_size - 4);
					file.read(&lastbyte, 1);
					if (lastbyte != '>')
					{
						nullbytecounter++;
					}
				}
			}

		}*/


		file.seekg(0, std::ios::end);
		xml_size = file.tellg();
		xml_size = xml_size - 176 - nullbytecounter;

		// Beginn der eigentlichen Lizenz
		// we need 8 additional bytes
		// Byte 0 - 3 for the static bytes
		// Byte 4 for the last \0
		license = new char[xml_size + 4];
		file.seekg(176);
		file.read(&license[3], xml_size);
		license[xml_size+3] = '\0';

		file.close();
		// TODO: Wofür steht die vier?
		xml_size_byte1 = xml_size_byte1 + '\4';


	}
	//unsigned char t = xml_size_byte1;
	// copy the license information
	unsigned char licenseSizeInBytes[4];
	licenseSizeInBytes[0] = xml_size_byte1;
	licenseSizeInBytes[1] = xml_size_byte2;
	licenseSizeInBytes[2] = '\0';
	licenseSizeInBytes[3] = '\0';
	//licenseSize = std::atoi(reinterpret_cast<char(&)[sizeof(licenseSizeInBytes)]>(licenseSizeInBytes));
	licenseSize = (licenseSizeInBytes[3] << 24) | (licenseSizeInBytes[2] << 16) | (licenseSizeInBytes[1] << 8) | (licenseSizeInBytes[0]);
	//licenseSize = atoi(licenseSizeInBytes);

	license[0] = byte1;
	license[1] = byte2;
	license[2] = byte3;

	//prepare coying to a managed object from an unmenged char array
	//http://geekswithblogs.net/cdpcodingblog/archive/2011/08/18/how-to-copy-from-char-managed-byte-array.aspx
	System::IntPtr licensePtr((int)license);
	//create new managed licenseByte structure
	array<Byte> ^ licenseBytes = gcnew array<Byte>(licenseSize);
	//copy the data
	Marshal::Copy(licensePtr, licenseBytes, 0, licenseSize);
	m_pLicense->cbBuffer = licenseSize;
	m_pLicense->pvBuffer = (LPVOID)new unsigned char[licenseSize];
	Marshal::Copy(licenseBytes, 0, (IntPtr)(m_pLicense->pvBuffer), m_pLicense->cbBuffer);


	// File, we decrypted previously, modified and want to re-encrypt
	std::ifstream manipulatedUnencryptedFile("decrypted.docx", std::ios::binary);
	if (manipulatedUnencryptedFile.is_open())
	{

		// TODO: Was passiert in diesem Block genau?
		// Bitte ein/zwei Zeilen Kommentare
		manipulatedUnencryptedFile.seekg(0, std::ios::end);
		unencryptedContentSize = manipulatedUnencryptedFile.tellg();
		manipulatedUnencryptedFile.seekg(0, std::ios::beg);
		//manipulatedUnencryptedFile.read(&encrypted_content_size_byte1, 1);
		//manipulatedUnencryptedFile.read(&encrypted_content_size_byte2, 1);
		//manipulatedUnencryptedFile.read(&encrypted_content_size_byte3, 1);
		//manipulatedUnencryptedFile.read(&encrypted_content_size_byte4, 1);
		//manipulatedUnencryptedFile.seekg(8);

		unsigned int padLength = (unsigned int)(16 - unencryptedContentSize % 16);
		contentSizeNoPadding = ( unsigned int ) unencryptedContentSize;
		unencryptedContentSize = ((unencryptedContentSize / 16) + 1) * 16;
		unencryptedContentBytes = gcnew array<Byte>((int)unencryptedContentSize);

		for (unsigned int i = contentSizeNoPadding; i < contentSizeNoPadding + padLength; i++)
			unencryptedContentBytes[i] = (unsigned char)padLength;
				
		// Contentbytes vorbereiten, die nachher der entschluesselungsroutine uebergeben werden
		for (i = 0; i < contentSizeNoPadding; i++)
		{
			manipulatedUnencryptedFile.read(&byte_read, 1);
			unencryptedContentBytes[i] = byte_read;
		}
		//contentBytes[i + 12 + xml_size + 1] = '\0';

		// vorbereitete Bytes entschluesseln
		Encrypt(unencryptedContentBytes);
		delete[] license;
		manipulatedUnencryptedFile.close();
	}

	return 0;
}
