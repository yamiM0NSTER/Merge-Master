// Merge Master.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include "concol.h"
#include <list>
#include <wincrypt.h>
using namespace concolors;
#define _AFX_NO_DEBUG_CRT
struct ResourceFile
{
	char strVersion[8];
	byte byEncryption;
	bool bEncryption;
	char strName[260];
	std::vector<CString> lstFiles;
	std::vector<CString> lstWildCards;
	std::vector<CString> lstCounts;
};

struct FileOutput
{
	vector<concol>*  vecColors;
	vector<CString>* vecText;
	FileOutput()
	{
		vecColors = new vector<concol>();
		vecText   = new vector<CString>();
	}
};

ResourceFile g_ResFile;
HANDLE g_hMutex = NULL;
mutex mtxResFile;
CRITICAL_SECTION pCritSec;
int nThreads = 0;
int nLines = 0;
COORD buffSize = {80, 300};
HANDLE hConsole;
vector< CString > vecABFiles;
mutex mtxFileAB;
vector< FileOutput > vecConsoleOutput;
void md5( char* out, const BYTE* pbContent, DWORD cbContent )
{
	HCRYPTPROV hCryptProv;
	HCRYPTHASH hHash;
	BYTE bHash[0x7f];
	DWORD dwHashLen	= 16;	// The MD5 algorithm always returns 16 bytes.
	*out	= '\0';

	if( CryptAcquireContext( &hCryptProv, 
		NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET ) )
	{
		if( CryptCreateHash( hCryptProv,
			CALG_MD5,	// algorithm identifier definitions see: wincrypt.h
			0, 0, &hHash ) )
		{
			if( CryptHashData( hHash, pbContent, cbContent, 0 ) )
			{
				if( CryptGetHashParam( hHash, HP_HASHVAL, bHash, &dwHashLen, 0 ) )
				{
					// Make a string version of the numeric digest value
					char tmp[4];
					for( int i = 0; i < 16; i++ )
					{
						sprintf( tmp, "%02x", bHash[i] );
						strcat( out, tmp );
					}
				}
			}
		}
		CryptDestroyHash( hHash );
		CryptReleaseContext( hCryptProv, 0 );
	}
}
void md5( char* out, const char* in )
{
	DWORD cbContent		= lstrlen( in );
	BYTE* pbContent	= (BYTE*)in;
	md5( out, pbContent, cbContent );
}

void __cdecl CreateResource( void* file )
{
	ResourceFile* resFile = (ResourceFile*)file;
	FileOutput resOutput;

	//cout << resFile.strName << endl;
	char szFileName[_MAX_FNAME];
	char szDir[_MAX_DIR];
	char szExt[_MAX_EXT];
	_splitpath( resFile->strName, NULL, szDir, szFileName, szExt );
	strcat_s( szFileName, sizeof(szFileName), szExt );

	resOutput.vecColors->push_back(concol::white);
	CString str;
	str.Format("Resource: %s - ", szFileName );
	resOutput.vecText->push_back(str);
	resOutput.vecColors->push_back(concol::white);
	resOutput.vecText->push_back("");
	// Get Existing Resource
	CResFile* resourceFileExt = new CResFile();
	strcpy( resourceFileExt->strVersion, resFile->strVersion );
	resourceFileExt->OpenResource( resFile->strName );

	// Count ==> JA SIE KURWA PRAWIE ZAJEBA£EM
	for( int i = 0; i < resFile->lstCounts.size(); i++ )
	{
		list<CString> lstStrings;
		lstStrings.push_back( resFile->lstCounts[0]);
		while( lstStrings.size() > 0 )
		{
			list<CString>::iterator it;
			while( lstStrings.size() > 0 )
			{
				it = lstStrings.begin();
				//CString str = *it;
				lstStrings.erase(it);
				int nPos;
				if( (nPos = str.Find( '(', 0 )) != -1 )
				{
					CString strtemp = str;
					strtemp = strtemp.Right( strtemp.GetLength() - nPos -1 );


					char* shit = new char[255];
					strcpy( shit, strtemp );
					char* pch = strtok( shit, "," ); 

					int amount = atoi(pch);
					pch = strtok( NULL, "," );
					int min = atoi( pch );
					pch = strtok( NULL, ",)" );
					int max = atoi( pch );

					CString strhelp;
					strhelp.Format( "%s%dd", "%0", amount );
					str.Delete( nPos, str.Find( ')', 0 ) - nPos +1 );
					str.Insert( nPos, strhelp );


					for( int k = min; k <= max; k++ )
					{
						CString strtemp1 = str;
						strtemp1.Format( strtemp1, k );

						lstStrings.push_back( strtemp1 );
					}
					delete[] shit;
				}
				else
				{
					_finddata_t c_countfile;
					if( _findfirst64i32( str, &c_countfile ) != -1 )
						resFile->lstFiles.push_back( str );

				}
			}
		}
	}

	vector<RESOURCENEW> vecResources;
	_finddata_t c_file;
	// Files & counts
	for( int i = 0; i < resFile->lstFiles.size(); i++ )
	{
		if( _findfirst64i32( resFile->lstFiles[i], &c_file ) != -1 )
		{
			RESOURCENEW resource;
			strcpy( resource.szFileName, c_file.name );
			resource.dwFileSize = c_file.size;
			resource.time_ = (int)c_file.time_write;
			char szExtAB[256];
			_splitpath( resFile->lstFiles[i], NULL, resource.szPath, NULL, szExtAB );
			//if( strlen(resource.szPath) == 0 )
			if( strcmpi(szExtAB, ".inc" ) == 0 ||
				strcmpi(szExtAB, ".h" ) == 0 ||
				strcmpi(szExtAB, ".txt" ) == 0 ||
				strcmpi(szExtAB, ".txt.txt" ) == 0 ||
				strcmpi(szExtAB, ".csv" ) == 0 ||
				strcmpi(szExtAB, ".prj" ) == 0 )
			{
				mtxFileAB.lock();
				vecABFiles.push_back( resFile->lstFiles[i] );
				mtxFileAB.unlock();
			}
			vecResources.push_back( resource );
		}
#ifdef __LOGGING
		else
		{
			resOutput.vecColors->push_back(concol::yellow);
			resOutput.vecText->push_back("  File Missing: " + resFile->lstFiles[i] + "\n");
		}
#endif // __LOGGING
	}
	// Wildcards
	for( int i = 0; i < resFile->lstWildCards.size(); i++ )
	{
		long hFile;
		if( ( hFile = _findfirst64i32( resFile->lstWildCards[i], &c_file )) != -1 )
		{
			do
			{
				if( strcmp( c_file.name ,".") == 0 || strcmp( c_file.name, ".." ) == 0 )
					continue;
				RESOURCENEW resource;
				strcpy( resource.szFileName, c_file.name );
				resource.dwFileSize = c_file.size;
				resource.time_ = (int)c_file.time_write;
				_splitpath( resFile->lstWildCards[i], NULL, resource.szPath, NULL, NULL );
				vecResources.push_back( resource );
			}
			while( _findnext( hFile, &c_file ) == 0 );
		}
	}

	if( vecResources.size() <= 0 )
	{
		if( _findfirst64i32( resFile->strName, &c_file ) != -1 )
		{
			DeleteFile(resFile->strName);
#ifdef __LOGGING
			resOutput.vecColors->at(1) = concol::dark_pink;
			CString str;
			str.Format("deleted( no longer contains any file )\n");
			resOutput.vecText->at(1) = str;
#endif // __LOGGING
		}
		else
		{
#ifdef __LOGGING
			resOutput.vecColors->at(1) = concol::red;
			CString str;
			str.Format("not created( 0 files)\n" );
			resOutput.vecText->at(1) = str;
#endif // __LOGGING
		}
		goto END_THREAD;
	}


	if( resourceFileExt->m_bOpened == true )
	{
		// Check sizes od resources
		if(vecResources.size() != resourceFileExt->m_vecResourceFiles.size())
		{
#ifdef __LOGGING
			resOutput.vecColors->at(1) = concol::cyan;
			CString str;
			str.Format("updated( %u files )\n", vecResources.size() );
			resOutput.vecText->at(1) = str;
#endif // __LOGGING
			goto CREATE_RESOURCE;
		}
		else
		{
			// Check if resources are the same.
			for( int i = 0; i < vecResources.size() && i < resourceFileExt->m_vecResourceFiles.size(); i++ )
			{
				if( strcmpi( vecResources[i].szFileName, resourceFileExt->m_vecResourceFiles[i].szFileName ) || // file name
					vecResources[i].time_ != resourceFileExt->m_vecResourceFiles[i].time_ || // file write time
					vecResources[i].dwFileSize != resourceFileExt->m_vecResourceFiles[i].dwFileSize) // file size
				{
#ifdef __LOGGING
					resOutput.vecColors->at(1) = concol::cyan;
					CString str;
					str.Format("updated( %u files )\n", vecResources.size() );
					resOutput.vecText->at(1) = str;
#endif // __LOGGING
					goto CREATE_RESOURCE;
				}
			}
		}
#ifdef __LOGGING
		resOutput.vecColors->at(1) = concol::gray;
		CString str;
		str.Format("skipped(No changes)\n");
		resOutput.vecText->at(1) = str;
#endif // __LOGGING
		goto END_THREAD;
	}
	else
	{
#ifdef __LOGGING
		resOutput.vecColors->at(1) = concol::green;
		CString str;
		str.Format("created( %u files )\n", vecResources.size() );
		resOutput.vecText->at(1) = str;
#endif // __LOGGING
		goto CREATE_RESOURCE;
	}

	// Create / Changes needed..
CREATE_RESOURCE:
	CResFile* resourceFileNew = new CResFile();
	if( resourceFileNew )
	{
		resourceFileNew->m_vecResourceFiles = vecResources;
		strcpy( resourceFileNew->strVersion, resFile->strVersion );
		strcpy( resourceFileNew->m_szFileName, resFile->strName );
		resourceFileNew->m_bEncryption = resFile->bEncryption;
		resourceFileNew->m_byEncryptionKey = resFile->byEncryption;
		resourceFileNew->Save();
	}

	delete resourceFileNew;
END_THREAD:
	mtxResFile.lock();
	/*SetConsoleScreenBufferSize(hConsole, buffSize );
	SetConsoleActiveScreenBuffer(hConsole);*/
	vecConsoleOutput.push_back(resOutput);
	mtxResFile.unlock();
	resFile->lstCounts.clear();
	resFile->lstFiles.clear();
	resFile->lstWildCards.clear();
	delete resFile;
	delete resourceFileExt;
	vecResources.clear();
	nThreads--;
	_endthread();
}

SYSTEM_INFO Initialize()
{
	//std::ios_base::sync_with_stdio(false);
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );

	// Get Cores
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	if( sys_info.dwNumberOfProcessors == 1 )
	{
		printf( ".GO GET BETTER COMPUTER.\n" );
		system("pause");
		exit(0);
	}

	char szTitle[128];
	sprintf(szTitle, "BETA Devesty Merge Master[Devesty.eu] Threads: %u", sys_info.dwNumberOfProcessors);
	SetConsoleTitleA(szTitle);

	// Console Colors
	concolinit();
	settextcolor(concol::aqua);

	return sys_info;
}

int _tmain(int argc, _TCHAR* argv[])
{
	SYSTEM_INFO sys_info = Initialize();

	// Get Starting Time
	ULONGLONG time1, time2;
	int nOutput = 0;
	time1 = GetTickCount64();
	
	//CScanner* scanner = new CScanner();
	CScriptReader* scanner = new CScriptReader();
	if( scanner->Load("resource.txt") )
	{
		char strVersion[8] = "";
		byte byEncryptionKey = 0;
		scanner->GetToken();
		while( scanner->tok != Toke::FINISHED )
		{
			scanner->Token.MakeLower();
			if( scanner->Token == "version" )
			{
				scanner->GetToken();
				CString str;
				str.Format( "\"%s\"", scanner->Token );
				strcpy( strVersion, str );
			}
			else if( scanner->Token == "encryptionkey" )
			{
				byEncryptionKey = scanner->GetHex();
			}
			else if( scanner->Token == "resource" )
			{
				ResourceFile* resFile = new ResourceFile;
				resFile->bEncryption = scanner->GetNumber(); // encryption
				scanner->GetToken(); // name
				strcpy_s( resFile->strName, sizeof(resFile->strName), scanner->token );
				resFile->byEncryption = byEncryptionKey;
				strcpy_s( resFile->strVersion, sizeof(resFile->strVersion) , strVersion );
				scanner->GetToken(); // {
				while( scanner->Token != "}" )
				{

					scanner->GetToken();
					if( scanner->Token == "file" )
					{
						scanner->GetToken();
						resFile->lstFiles.push_back( scanner->Token );
					}
					else if( scanner->Token == "wildcard" )
					{
						scanner->GetToken();
						resFile->lstWildCards.push_back( scanner->Token );
					}
					else if( scanner->Token == "count" )
					{
						scanner->GetToken();
						resFile->lstCounts.push_back( scanner->Token );
					}
				}

				while( nThreads >= sys_info.dwNumberOfProcessors-1 )
				{
					if( nOutput < vecConsoleOutput.size())
					{
						mtxResFile.lock();
						FileOutput resOutput = vecConsoleOutput[nOutput];
						mtxResFile.unlock();
						for(int k=0;k<resOutput.vecColors->size();k++)
						{
							buffSize.Y++;
							settextcolor(resOutput.vecColors->at(k));
							printf(resOutput.vecText->at(k));
						}
						nOutput++;
					}
					
					Sleep(10);
				}

				SetConsoleScreenBufferSize(hConsole, buffSize );
				SetConsoleActiveScreenBuffer(hConsole);

				nThreads++;
				_beginthread( CreateResource, 0, (void*)resFile );
			}
			scanner->GetToken();
		}
	}
	else
	{
		cout << "Resource.txt not found!" << endl;
	}

	delete scanner;
	while( nThreads > 0 )
	{
		if( nOutput < vecConsoleOutput.size())
		{
			mtxResFile.lock();
			FileOutput resOutput = vecConsoleOutput[nOutput];
			mtxResFile.unlock();
			for(int k=0;k<resOutput.vecColors->size();k++)
			{
				buffSize.Y++;
				settextcolor(resOutput.vecColors->at(k));
				printf(resOutput.vecText->at(k));
			}
			nOutput++;
		}
		Sleep(10);
	}

	while( nOutput < vecConsoleOutput.size())
	{
		FileOutput resOutput = vecConsoleOutput[nOutput];
		for(int k=0;k<resOutput.vecColors->size();k++)
		{
			buffSize.Y++;
			settextcolor(resOutput.vecColors->at(k));
			printf(resOutput.vecText->at(k));
		}
		nOutput++;
	}

	// Flyff.a creation
	CFile flyffa;
	if(flyffa.Open( "..\\flyff.a", CFile::modeCreate | CFile::modeReadWrite) )
	{
		for( int i = 0; i < vecABFiles.size(); i++ )
		{
			char sFile[100]	= { 0,};
			char sData[100]	= { 0,};

			CFile file;
			if(file.Open( vecABFiles[i], CFile::modeRead ))
			{

				char szFile[256],szExt[256];
				_splitpath( vecABFiles[i], NULL, NULL, szFile, szExt );
				strcat(szFile,szExt);
				md5( sFile, szFile );
				BYTE* ptr = new BYTE[file.GetLength()];
				file.Read( ptr, file.GetLength() );
				md5( sData, (BYTE*)ptr, file.GetLength() );

				flyffa.Write( sFile, 32 );
				flyffa.Write( sData, 32 );

				delete ptr;
				file.Close();
			}
		}

		flyffa.Flush();
		flyffa.Close();
		flyffa.Open( "..\\flyff.a", CFile::modeRead );
		BYTE* ptrb = new BYTE[flyffa.GetLength()];
		flyffa.Read( ptrb, flyffa.GetLength() );
		CFile flyffb;
		if(flyffb.Open( "..\\flyff.b", CFile::modeCreate | CFile::modeWrite) )
		{
			char sDataB[100]	= { 0,};
			md5( sDataB, ptrb, flyffa.GetLength() );
			flyffb.Write( sDataB, strlen(sDataB) );
			flyffb.Close();
		}
		else
			printf( "Couldn't Access: \"..\\Flyff.b\"\n" );
		
		delete ptrb;
		flyffa.Close();
	}
	else
	{
		printf( "Couldn't Access: \"..\\Flyff.a\"\n" );
	}

	/*printf("\n");
	for(int i = 0;i<vecConsoleOutput.size();i++)
	{
		FileOutput resOutput = vecConsoleOutput[i];
		for(int k=0;k<resOutput.vecColors->size();k++)
		{
			buffSize.Y++;
			settextcolor(resOutput.vecColors->at(k));
			printf(resOutput.vecText->at(k));
		}
	}
	printf("\n");*/

	settextcolor(concol::white);
	time2 = GetTickCount64();
	printf( "Time: %.3f seconds\n", (float)(time2 - time1)/1000.f);
	
	_tsystem("pause");
	return 0;
}

