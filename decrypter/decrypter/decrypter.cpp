#include "stdafx.h"


using namespace System::IO;
using namespace System;
using namespace System::Runtime::InteropServices;



IPC_KEY_HANDLE      m_key = nullptr;
PIPC_BUFFER         m_pLicense = new IPC_BUFFER;
PIPC_TEMPLATE_INFO  m_pTemplateInfo = nullptr;
int MAX_LOADSTRING = 200;
unsigned int encryptedPackageSize;

MemoryStream ^initDecryption(array<Byte> ^);
System::Void readLicense(Stream ^);
MemoryStream ^decryptEncryptedContent(Stream ^);

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

System::Void readLicense(Stream ^encryptedStream)
{
	array<Byte>     ^bytesLength;
	array<Byte>     ^bytesLicense;
	int             bytesRead;
	pin_ptr<IPC_KEY_HANDLE>     pKey;
	HRESULT                     hr;

	bytesLength = gcnew array<Byte>(sizeof(DWORD));
	bytesRead = encryptedStream->Read(bytesLength, 0, bytesLength->Length);

	Marshal::Copy(bytesLength, 0, (IntPtr)(&m_pLicense->cbBuffer), sizeof(DWORD));

	bytesLicense = gcnew array<Byte>(m_pLicense->cbBuffer);
	bytesRead = encryptedStream->Read(bytesLicense, 0, bytesLicense->Length);

	m_pLicense->pvBuffer = (LPVOID) new unsigned char[bytesLicense->Length];
	Marshal::Copy(bytesLicense, 0, (IntPtr)(m_pLicense->pvBuffer), m_pLicense->cbBuffer);


	// get content key from read in license
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

}

MemoryStream^ initDecryption(array<Byte> ^encryptedBytes)
{
	MemoryStream    ^encryptedStream;
	MemoryStream                ^result;
	pin_ptr<IPC_KEY_HANDLE>     pKey;

	// convert byte array to stream
	encryptedStream = gcnew MemoryStream(encryptedBytes);

	// Lizenz einlesen
	readLicense(encryptedStream);

	result = decryptEncryptedContent(encryptedStream);
	result->Flush();
	result->Seek(0, SeekOrigin::Begin);

	encryptedStream->Close();
	encryptedStream = nullptr;

	return result;
}

MemoryStream^ decryptEncryptedContent(Stream ^encryptedStream)
{
	DWORD           *pcbBlockSize;
	DWORD           cbReadRemaining;
	DWORD           cbRead;
	DWORD           cbOutputBuffer;
	DWORD           cbDecrypted;
	array<Byte>     ^readBuffer;
	array<Byte>     ^writeBuffer;
	pin_ptr<Byte>   pbReadBuffer;
	pin_ptr<unsigned char>   pbWriteBuffer;
	int             cBlock;
	HRESULT         hr;
	MemoryStream    ^decryptedStream;
	std::ofstream in_datei;
	std::string dateiname;


	hr = IpcGetKeyProperty(m_key,
		IPC_KI_BLOCK_SIZE,
		nullptr,
		(LPVOID *)&pcbBlockSize);
	CheckAndHandleError(hr);

	try
	{

		readBuffer = gcnew array<Byte>(*pcbBlockSize);
		pbReadBuffer = &readBuffer[0];

		cbOutputBuffer = *pcbBlockSize;
		writeBuffer = gcnew array<Byte>(cbOutputBuffer);
		pbWriteBuffer = &writeBuffer[0];

		cbRead = encryptedStream->Read(readBuffer, 0, sizeof(DWORD));

		Marshal::Copy(readBuffer, 0, (IntPtr)&cbDecrypted, sizeof(DWORD));
		decryptedStream = gcnew MemoryStream(cbDecrypted);

		cBlock = 0;
		cbReadRemaining = (DWORD)(encryptedStream->Length - encryptedStream->Position);

		dateiname = "decrypted.docx";

		in_datei.open(dateiname.c_str(), std::ios_base::binary);

		while (cbReadRemaining > *pcbBlockSize)
		{
			cbRead = 0;
			encryptedStream->Read(readBuffer, 0, *pcbBlockSize);
			hr = IpcDecrypt(m_key,
				cBlock,
				false,
				pbReadBuffer,
				*pcbBlockSize,
				pbWriteBuffer,
				cbOutputBuffer,
				&cbRead);

			// entschluesselten Inhalt Byteweise in eine Datei schreiben

			for (int i = 0; i < 16; i++)
			{
				// TODO: Was ist der Sinn dieser If Abfrage?
				if (pbWriteBuffer[i] == '\0')
				{
					in_datei << '\0';
				}
				else
				{
					in_datei << pbWriteBuffer[i];
				}
			}

			cBlock++;

			decryptedStream->Write(writeBuffer, 0, cbRead);
			cbReadRemaining -= cbRead;
		}

		cbRead = 0;
		encryptedStream->Read(readBuffer, 0, cbReadRemaining);
		hr = IpcDecrypt(m_key,
			cBlock,
			true,
			pbReadBuffer,
			cbReadRemaining,
			pbWriteBuffer,
			cbOutputBuffer,
			&cbRead);

		// letzten Block byteweise in eine Datei schreiben

		for (int i = 0; i < 16; i++)
		{
			if (pbWriteBuffer[i] == '\0')
			{
				in_datei << '\0';
			}
			else
			{
				in_datei << pbWriteBuffer[i];
			}
		}

		decryptedStream->Write(writeBuffer, 0, cbRead);
	}
	finally
	{
		readBuffer = nullptr;
		writeBuffer = nullptr;
		IpcFreeMemory((LPVOID)pcbBlockSize);
		in_datei.close();
	}

	return decryptedStream;
}

int __cdecl main()
{
	HRESULT     hr;
	hr = IpcInitialize();

	std::streampos size;
	char * license = new char[200000];
	char xml_size_byte1;
	char xml_size_byte2;
	char encrypted_content_size_byte1;
	char encrypted_content_size_byte2;
	char encrypted_content_size_byte3;
	char encrypted_content_size_byte4;
	char byte_read;
	char lastbyte;
	int xml_size;
	byte byte1 = 0xEF;
	byte byte2 = 0xBB;
	byte byte3 = 0xBF;
	int i, nullbytecounter = 0;
	array<Byte> ^contentBytes;

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

		unsigned int d = 1;
		while (lastbyte != '>')
		{
			nullbytecounter++;
			d++;
			file.seekg(xml_size - d);
			file.read(&lastbyte, 1);
		}

		file.seekg(0, std::ios::end);
		xml_size = file.tellg();
		xml_size = xml_size - 176 - nullbytecounter;

		// Beginn der eigentlichen Lizenz
		file.seekg(176);
		file.read(license, xml_size);
		license[xml_size] = '\0';

		file.close();
		// TODO: Wofür steht die vier?
		xml_size_byte1 = xml_size_byte1 + '\4';
	}

	// Datei, die die Laenge des verschluesselten Inhalts und den verschluesselten Inhalt selbst enthaelt
	char* path = "EncryptedPackage";
	std::ifstream file2(path, std::ios::binary);
	if (file2.is_open())
	{

		// TODO: Was passiert in diesem Block genau?
		// Bitte ein/zwei Zeilen Kommentare
		file2.seekg(0, std::ios::end);
		// get the total size, with the 8 bytes for the size of the decrypted content
		encryptedPackageSize = size = file2.tellg();
		file2.seekg(0, std::ios::beg);
		file2.read(&encrypted_content_size_byte1, 1);
		file2.read(&encrypted_content_size_byte2, 1);
		file2.read(&encrypted_content_size_byte3, 1);
		file2.read(&encrypted_content_size_byte4, 1);
		file2.seekg(8);
		//contentBytes = gcnew array<Byte>(6 + xml_size + 5 + (int)size);

		// 6 Bytes (xml_size_byte1-2 + static 0 Bytes and byte1-3)
		// + xml_size from the license
		// + 5 bytes for the size of the encrypted content and one 0 byte
		contentBytes = gcnew array<Byte>(6 + xml_size + 5 + (int)size);
		
		
		// Contentbytes vorbereiten, die nachher der entschluesselungsroutine uebergeben werden
		contentBytes[0] = xml_size_byte1;
		contentBytes[1] = xml_size_byte2;
		contentBytes[2] = '\0';
		contentBytes[3] = '\0';
		contentBytes[4] = byte1;
		contentBytes[5] = byte2;
		contentBytes[6] = byte3;

		// TODO: "Eleganter" waere k=0; k<xml_size; k++ und unten contentBytes[k+7] oder?
		for (int k = 7; k < xml_size + 7; k++)
			contentBytes[k] = license[k - 7];

		// TODO: Was bedeutet diese Zeile? Was bedeutet \4 ?
		//  encrypted_content_size_byte1 = encrypted_content_size_byte1 + '\4' + '\4' + '\4' + '\4' + '\4' + '\4';
		//encrypted_content_size_byte1 = encrypted_content_size_byte1 - 8;
		
		contentBytes[7 + xml_size] = encrypted_content_size_byte1;
		contentBytes[8 + xml_size] = encrypted_content_size_byte2;
		contentBytes[9 + xml_size] = encrypted_content_size_byte3;
		contentBytes[9 + xml_size] = encrypted_content_size_byte4;
		contentBytes[11 + xml_size] = '\0';


		for (i = 0; i <= (int)size - 9; i++)
		{
			file2.read(&byte_read, 1);

			// TODO: Was ist der Sinn dieser If Abfrage?
			if (byte_read == '\0')
			{
				contentBytes[i + 12 + xml_size] = '\0';
			}
			else
			{
				contentBytes[i + 12 + xml_size] = byte_read;
			}
		}
		contentBytes[i + 12 + xml_size + 1] = '\0';

		// vorbereitete Bytes entschluesseln
		initDecryption(contentBytes);
		delete[] license;
		file2.close();
	}
	return 0;
}

//MemoryStream^ decryptEncryptedContent(Stream ^encryptedStream)
//{
//	MemoryStream                ^result;
//	pin_ptr<IPC_KEY_HANDLE>     pKey;
//	HRESULT                     hr;
//
//	// Lizenz einlesen
//	readLicense(encryptedStream);
//
//	// Contentkey aus der Lizenz holen
//	pKey = &m_key;
//	hr = IpcGetKey(m_pLicense,
//		0,
//		nullptr,
//		nullptr,
//		pKey);
//	CheckAndHandleError(hr);
//
//	pin_ptr<PIPC_TEMPLATE_INFO>     ppTemplateInfo;
//	ppTemplateInfo = &m_pTemplateInfo;
//	hr = IpcGetSerializedLicenseProperty(m_pLicense,
//		IPC_LI_DESCRIPTOR,
//		m_key,
//		0,
//		(LPVOID *)ppTemplateInfo);
//	CheckAndHandleError(hr);
//
//	result = readDecryptedContent(encryptedStream);
//	result->Flush();
//	result->Seek(0, SeekOrigin::Begin);
//
//	return result;
//}

