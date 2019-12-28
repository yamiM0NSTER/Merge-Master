#include "stdafx.h"
#include "ScriptReader.h"


inline int iswhite_char(wchar_t c)
{
	if (c > 0 && c <= 0x20)
		return 1;
	return 0;
}

inline int isdelimiter_char(wchar_t c)
{
	if (strchr(" !:;,+-<>'/*%^=()&|\"{}", c) || c == 9 || c == '\r' || c == 0 || c == '\n')
		return 1;
	return 0;
}

WORD g_codePage = 1052;	// 949
const char*	CharNextEx(const char* strText, WORD codePage = g_codePage)
{
	return CharNextExA(g_codePage, strText, 0);
}

BOOL IsMultiByte(const char* pSrc)
{
	return (CharNextEx(pSrc) - pSrc) > 1;
}

int CopyChar(const char* pSrc, char* pDest)
{
	const char* pNext = CharNextEx(pSrc);

	memcpy(pDest, pSrc, pNext - pSrc);

	return pNext - pSrc;
}

int WideCharToMultiByteEx(
	UINT     CodePage,
	DWORD    dwFlags,
	LPCWSTR  lpWideCharStr,
	int      cchWideChar,
	LPSTR    lpMultiByteStr,
	int      cchMultiByte,
	LPCSTR   lpDefaultChar,
	LPBOOL   lpUsedDefaultChar)
{
	int nLength = WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpUsedDefaultChar);

	return nLength;
}

CScriptReader::CScriptReader()
{
	m_pProg = m_pBuf = NULL;
	m_bMemFlag = 1;
	token = m_mszToken;
	m_bErrorCheck = TRUE;
}


CScriptReader::~CScriptReader()
{
	if (!m_bMemFlag && m_pBuf)
		delete[](m_pBuf);
	m_pProg = m_pBuf = 0;
}

BOOL CScriptReader::Load(LPCTSTR lpszFileName, BOOL bMultiByte)
{
	CResFile file;
	CString fileName = lpszFileName;
	fileName.MakeLower();
	if (!file.Open(lpszFileName, "rb"))
		return 0;
	m_strFileName = lpszFileName;
	return Read(&file, bMultiByte);
}

BOOL CScriptReader::Read(CFileIO* pFile, BOOL)
{
	//m_pFile = pFile;
	m_bMemFlag = 0;
	m_nProgSize = pFile->GetLength();

	int nSize = m_nProgSize + 2;
	char* pProg = new char[nSize];
	if (!pProg)
		return 0;

	m_pProg = m_pBuf = pProg;
	pFile->Read(m_pBuf, m_nProgSize);
	
	*(pProg + m_nProgSize) = '\0';
	*(pProg + m_nProgSize + 1) = '\0';

	if ((BYTE)*(pProg + 0) == 0xff && (BYTE)*(pProg + 1) == 0xfe) // is unicode ?
	{
		char* lpMultiByte = new char[nSize];
		int nResult = WideCharToMultiByteEx(g_codePage, 0,
			(LPCWSTR)(pProg + 2), -1,
			lpMultiByte, nSize,
			NULL, NULL);
		if (nResult > 0)
		{
			lpMultiByte[nResult - 1] = 0;
			memcpy(pProg, lpMultiByte, nResult);
		}
		delete[](lpMultiByte);
	}
	return 1;
}

int CScriptReader::GetToken(BOOL bComma)
{
	char* pszCur = m_mszToken;
	*pszCur = '\0';
	tokenType = TokenType::TEMP;
	tok = Toke::NONE_;

	BOOL bLoop;
	do
	{
		bLoop = FALSE;

		// white spaces
		while (iswhite_char(*m_pProg) && *m_pProg) 
			++m_pProg;

		// Comments
		while (*m_pProg == '/')
		{
			++m_pProg;
			// Line Comment
			if (*m_pProg == '/')
			{
				++m_pProg;
				while (*m_pProg != '\r' && *m_pProg != '\0')
					++m_pProg;
				if (*m_pProg == '\r')
					m_pProg += 2;
			}
			// /* */ Comment
			else if (*m_pProg == '*')
			{
				++m_pProg;
				do
				{
					while (*m_pProg != '*' && *m_pProg != '\0')
						++m_pProg;

					if (*m_pProg == '\0')
					{
						tok = Toke::FINISHED;
						tokenType = TokenType::DELIMITER;
						return tokenType;
					}
					++m_pProg;
				} while (*m_pProg != '/');

				++m_pProg;
				while (iswhite_char(*m_pProg) && *m_pProg)
					++m_pProg;
			}
			// Not a comment, just a slash
			else
			{
				--m_pProg;
				break;
			}
			bLoop = TRUE;
		}
	} while (bLoop);

	// EOF
	if (*m_pProg == '\0')
	{
		tok = Toke::FINISHED;
		tokenType = TokenType::DELIMITER;
		goto EXIT;
	}

	// Delimiters
	if (strchr("+-*^/%=;(),':{}.", *m_pProg))
	{
		*pszCur = *m_pProg;
		++m_pProg;
		++pszCur;
		tokenType = TokenType::DELIMITER;
		goto EXIT;
	}

	// Strings
	if (*m_pProg == '"')
	{
		++m_pProg;
		while (*m_pProg != '"' && *m_pProg != '\r' && *m_pProg != '\0' && (pszCur - token) < MAX_TOKENSTR)
		{
			int count = CopyChar(m_pProg, pszCur);
			m_pProg += count;
			pszCur += count;
		}
		++m_pProg;
		tokenType = TokenType::STRING;
		if (*(m_pProg - 1) != '"')
		{
			if (*(m_pProg - 1) == '\0')
				--m_pProg;
			if (m_bErrorCheck)
			{
				CString string;
				if ((pszCur - token) >= MAX_TOKENSTR)
					string.Format("line(%d) the length of string in file \"%s\" exceeds %d bytes.", GetLineNum(), m_strFileName, MAX_TOKENSTR);
				else
					string.Format("line(%d) string in file \"%s\" does not end in quotes", GetLineNum(), m_strFileName);
			}
		}
		goto EXIT;
	}

	// Hexadecimals
	if (*m_pProg == '0' && *(m_pProg + 1) == 'x')
	{
		m_pProg += 2;
		while (!isdelimiter_char(*m_pProg))
			*pszCur++ = *m_pProg++;
		tokenType = TokenType::HEX;
		goto EXIT;
	}

	// Numbers
	if (isdigit_char(*m_pProg) && !IsMultiByte(m_pProg))
	{
		while (!isdelimiter_char(*m_pProg))
		{
			int count = CopyChar(m_pProg, pszCur);
			m_pProg += count;
			pszCur += count;
		}

		tokenType = TokenType::NUMBER;
		goto EXIT;
	}

	if( isalpha( *m_pProg ) || IsMultiByte( m_pProg )
		|| *m_pProg == '#' || *m_pProg == '_' || *m_pProg == '@' || *m_pProg=='$' || *m_pProg == '?' ) 
	{
		while( !isdelimiter_char( *m_pProg ) ) 
		{
			int count = CopyChar( m_pProg, pszCur );
			m_pProg += count;
			pszCur += count;
		}
	}
	else
	{
		*pszCur++ = *m_pProg++;
	}

	tokenType = TokenType::TEMP;

EXIT:
	*pszCur = '\0';
	Token = token;
	return tokenType;
}

int CScriptReader::GetNumber(BOOL bComma)
{
	m_dwDef = 1;
	if (GetToken(bComma) == HEX)
	{
		Token.MakeLower();
		DWORDLONG dwlNumber = 0;
		DWORD dwMulCnt = 0;
		CHAR cVal;
		for (int i = Token.GetLength() - 1; i >= 0; i--)
		{
			cVal = Token[i];
			if (cVal >= 'a')
				cVal = (cVal - 'a') + 10;
			else cVal -= '0';
			dwlNumber |= (DWORDLONG)cVal << dwMulCnt;
			dwMulCnt += 4;
		}
		m_dwDef = 0;
		return (DWORD)dwlNumber;
	}
	else if (!Token.IsEmpty())
	{
		switch (Token[0])
		{
		case '=':
			m_dwDef = 0;
			return NULL_ID;
		case '-':
			if (bComma == FALSE)
			{
				GetToken();
				m_dwDef = 0;
				return -atoi(Token);
			}
			else
			{
				m_dwDef = 0;
				return atoi(Token);
			}
		case '+':
			if (bComma == FALSE)
				GetToken();
		}
		m_dwDef = 0;
		return atoi(Token);
	}
	m_dwDef = 0;
	return 0;
}

DWORD CScriptReader::GetHex(BOOL bComma)
{
	m_dwDef = 1;
	if (GetToken(bComma) == HEX)
	{
		Token.MakeLower();
		DWORDLONG dwlNumber = 0;
		DWORD dwMulCnt = 0;
		CHAR cVal;
		for (int i = Token.GetLength() - 1; i >= 0; i--)
		{
			cVal = Token[i];
			if (cVal >= 'a')
				cVal = (cVal - 'a') + 10;
			else cVal -= '0';
			dwlNumber |= (DWORDLONG)cVal << dwMulCnt;
			dwMulCnt += 4;
		}
		m_dwDef = 0;
		return (DWORD)dwlNumber;
	}
	m_dwDef = 0;
	return 0;
}

int CScriptReader::GetLineNum(LPVOID lpProg)
{
	int nLine = 1;
	if (lpProg == NULL)
		lpProg = m_pProg;

	CHAR* pCur = m_pBuf;
	while (lpProg != pCur)
	{
		if (*pCur == '\r')
			nLine++;
		pCur++;
	}
	return nLine;
}