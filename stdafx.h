// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#define _AFXDLL
#include <afxwin.h>

//#include <WinSock.h>
//#include <fstream>
#include <iostream>
//#include <sstream>

#include <vector>
#include <mutex>

using namespace std;

#define __CLIENT
#define __LOGGING

#define	safe_delete_array( p )	\
{	\
	delete[] (p);	\
}

#include "file.h"
#include "ScriptReader.h"





// TODO: reference additional headers your program requires here
