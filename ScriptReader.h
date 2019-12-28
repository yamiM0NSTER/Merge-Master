#ifndef __SCRIPTREADER_H
#define __SCRIPTREADER_H

#ifndef NULL_ID
#define NULL_ID 0xffffffff
#endif

#define MAX_TOKENSTR 2048

inline BOOL isdigit_char(CHAR c)
{
	if (c >= '0' && c <= '9')
		return TRUE;
	else
		return FALSE;
}

enum Toke
{
	NONE_, ARG, VAR, IF, ELSE, FOR, DO, WHILE, BREAK, SWITCH, ANSWER, SELECT, YESNO,
	CASE, DEFAULT, GOTO, RETURN, EOL, DEFINE, INCLUDE, ENUM, FINISHED, END
};

enum TokenType
{
	TEMP, DELIMITER, IDENTIFIER, NUMBER, HEX, KEYWORD, STRING, BLOCK
};

class CScriptReader
{
public:
	CScriptReader();
	~CScriptReader();

	CHAR*		m_pProg;
	CHAR*		m_pBuf;
	CString 	m_strFileName;
	BYTE		m_bMemFlag;
	int			m_nProgSize;
	DWORD		m_dwDef;

	char		m_mszToken[MAX_TOKENSTR];
	int			tokenType;
	int			tok;
	CString		Token;
	char*		token;
	BOOL		m_bErrorCheck;

	BOOL Load(LPCTSTR lpszFileName, BOOL bMultiByte = TRUE);
	BOOL Read(CFileIO* pFile, BOOL);

	virtual int		GetLineNum(LPVOID lpProg = NULL);
	virtual	int		GetToken(BOOL bComma = FALSE);
	int				GetNumber(BOOL bComma = FALSE);
	DWORD			GetHex(BOOL bComma = FALSE);
};

#endif 