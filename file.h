#ifndef __CFileIO_H
#define __CFileIO_H

#include <io.h>

class CFileIO
{
public:
	FILE* fp;

	CFileIO( LPCTSTR lpszFileName, TCHAR* mode ) { Open( lpszFileName, mode ); }
	CFileIO() { fp = NULL; } 

	virtual ~CFileIO() { Close(); }
	virtual BOOL  Open( LPCTSTR lpszFileName, TCHAR* mode );
	virtual BOOL Close();
	virtual int	 Flush()  { return fflush( fp ); }
	virtual char  GetC()  { return (char)getc( fp ); }
	virtual WORD  GetW()  { return getwc( fp ); } 
	virtual DWORD GetDW() { DWORD dw; fread( &dw, sizeof( dw ), 1, fp ); return dw; }
	virtual long Tell()   { return ftell(fp); }
	virtual int  Seek( long offset, int whence ) { return fseek( fp, offset, whence ); }
	virtual long GetLength() { return _filelength( Handle() ); }
	virtual size_t Read( void *ptr, size_t size, size_t n = 1 )	{ return fread( ptr, size, n, fp ); }
	virtual LPVOID Read();

	int  Handle() { return _fileno( fp ); }
	int  Error()  { return ferror( fp ); }
	int  Eof() { return feof( fp ); }
	int  PutC( char c ) { return putc( c, fp ); }
	int  PutW( WORD w ) { return _putw( w, fp ); }
	int  PutDW( DWORD dw ) { return fwrite( &dw, sizeof( dw ), 1, fp ); }
	size_t Write( LPVOID ptr, size_t size, size_t n = 1 ) { return fwrite( ptr, size, n, fp ); }
	int PutString( LPCTSTR lpszString ) { return _ftprintf( fp, lpszString ); }
	int PutWideString( LPCTSTR lpszString );
};

static byte eTable[256] = { 255, 
	239, 223, 207, 191, 175, 159, 143, 127, 111, 95, 
	79, 63, 47, 31, 15, 254, 238, 222, 206, 190, 
	174, 158, 142, 126, 110, 94, 78, 62, 46, 30, 
	14, 253, 237, 221, 205, 189, 173, 157, 141, 125, 
	109, 93, 77, 61, 45, 29, 13, 252, 236, 220, 
	204, 188, 172, 156, 140, 124, 108, 92, 76, 60, 
	44, 28, 12, 251, 235, 219, 203, 187, 171, 155, 
	139, 123, 107, 91, 75, 59, 43, 27, 11, 250, 
	234, 218, 202, 186, 170, 154, 138, 122, 106, 90, 
	74, 58, 42, 26, 10, 249, 233, 217, 201, 185, 
	169, 153, 137, 121, 105, 89, 73, 57, 41, 25, 
	9, 248, 232, 216, 200, 184, 168, 152, 136, 120, 
	104, 88, 72, 56, 40, 24, 8, 247, 231, 215, 
	199, 183, 167, 151, 135, 119, 103, 87, 71, 55, 
	39, 23, 7, 246, 230, 214, 198, 182, 166, 150, 
	134, 118, 102, 86, 70, 54, 38, 22, 6, 245, 
	229, 213, 197, 181, 165, 149, 133, 117, 101, 85, 
	69, 53, 37, 21, 5, 244, 228, 212, 196, 180, 
	164, 148, 132, 116, 100, 84, 68, 52, 36, 20, 
	4, 243, 227, 211, 195, 179, 163, 147, 131, 115, 
	99, 83, 67, 51, 35, 19, 3, 242, 226, 210, 
	194, 178, 162, 146, 130, 114, 98, 82, 66, 50, 
	34, 18, 2, 241, 225, 209, 193, 177, 161, 145, 
	129, 113, 97, 81, 65, 49, 33, 17, 1, 240, 
	224, 208, 192, 176, 160, 144, 128, 112, 96, 80, 
	64, 48, 32, 16, 0 };

#if defined( __CLIENT )
struct RESOURCE
{
	TCHAR szResourceFile[ 128 ];
	DWORD dwOffset;
	DWORD dwFileSize;
	BYTE  byEncryptionKey;
	bool  bEncryption;
}; 

struct RESOURCENEW
{
	//TCHAR szResourceFile[ 128 ];
	DWORD dwOffset;
	DWORD dwFileSize;
	BYTE  byEncryptionKey;
	bool  bEncryption;
	TCHAR szFileName[260];
	int time_;
	char szPath[260];
}; 
class CResFile : public CFileIO
{
public:
	CResFile( LPCTSTR lpszFileName, TCHAR *mode );
	CResFile();

	vector<RESOURCENEW> m_vecResourceFiles;

	void OpenResource( LPCSTR lpszResName );

public:
	CFile m_File;
	bool m_bResouceInFile;
	char m_szFileName[_MAX_FNAME];
	DWORD m_nFileSize;
	DWORD m_nFileBeginPosition; // 시작 위치
	DWORD m_nFileCurrentPosition; // 현재 위치
	DWORD m_nFileEndPosition; // 끝 위치
	bool m_bEncryption;
	BYTE m_byEncryptionKey;
	static void AddResource( TCHAR* lpResName );

	bool m_bOpened;

public:

#ifdef __SECURITY_0628
	static	char	m_szResVer[100];
	static	map<string, string>	m_mapAuth;
	static	void	LoadAuthFile( void );
#endif	// __SECURITY_0628

	static CMapStringToPtr m_mapResource;

	static BYTE Encryption( BYTE byEncryptionKey, BYTE byData )
	{
		//	byData = ( byData << 4 ) | ( byData >> 4 );
		//	return ( ~byData ) ^ byEncryptionKey;

		return eTable[byData] ^ byEncryptionKey;
	}
	static BYTE Decryption( BYTE byEncryptionKey, BYTE byData )
	{
		byData = ~byData ^ byEncryptionKey;
		return ( byData << 4 ) | ( byData >> 4 );

		//return eTable[byData] ^ byEncryptionKey;
	}
	
	void Save();
	char strVersion[8];

	inline void DecryptionNew( BYTE byEncryptionKey, BYTE *byData, int nStartPos, int nLength )
	{
		int nEnd = nStartPos + nLength;
		for( int i = nStartPos; i < nEnd; i++ )
		{
			//byData[i] = ~byData[i] ^ byEncryptionKey;
			//byData[i] = ( byData[i] << 4 ) | ( byData[i] >> 4 );
			byData[i] = eTable[byData[i] ^ byEncryptionKey];
		}
	}

	inline void EncryptionNew( BYTE byEncryptionKey, BYTE *byData, int nStartPos, int nLength )
	{
		int nEnd = nStartPos + nLength;
		for( int i = nStartPos; i < nEnd; i++ )
		{
			//	byData[i] = ( byData[i] << 4 ) | ( byData[i] >> 4 );
			//	byData[i] = ( ~byData[i] ) ^ byEncryptionKey;
			byData[i] = eTable[byData[i]] ^ byEncryptionKey;
		}
	}

	BOOL IsEncryption() { return m_bEncryption; }
	virtual ~CResFile() { Close(); }
	virtual BOOL Open( LPCTSTR lpszFileName, TCHAR *mode );
	virtual BOOL Close( void );

	virtual LPVOID Read();
	virtual size_t Read( void *ptr, size_t size, size_t n = 1 );
	virtual long GetLength();
	virtual int  Seek( long offset, int whence );
	virtual long Tell();

	virtual char  GetC();
	virtual WORD  GetW();
	virtual DWORD GetDW();

	virtual int	 Flush();

	//BOOL FindFile(char *szSerchPath, LPCTSTR lpszFileName, TCHAR *mode, int f );
	//BOOL FindFileFromResource( char *filepath, LPCTSTR lpszFileName );

	static void ScanResource( LPCTSTR lpszRootPath = "");
	static void FreeResource();
};
#else
// 클라이언트가 아니면 
#define CResFile CFileIO
#endif


class CFileFinder
{
	POSITION m_pos;
	long m_lHandle;
	CHAR m_szFilespec[ MAX_PATH ];
	BOOL m_bResFile;
public:
	CFileFinder();
	~CFileFinder();
	BOOL WildCmp( LPCTSTR lpszWild, LPCTSTR lpszString );

	BOOL FindFirst( LPCTSTR lpFilespec, struct _finddata_t *fileinfo );
	BOOL FindNext( struct _finddata_t *fileinfo );
	void FindClose();
};


#endif
