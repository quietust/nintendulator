// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#pragma warning(disable:4100)	// "unreferenced formal parameter" - functions which don't use every parameter (mostly controllers)
#pragma warning(disable:4127)	// "conditional expression is constant" - do { ... } while (false);
#pragma warning(disable:4201)	// "nonstandard extension used : nameless struct/union" - used everywhere in DirectX
#pragma warning(disable:4244)	// "conversion from 'foo' to 'bar', possible loss of data" - I/O handlers all pass 'int' values and get crammed into bytes/shorts

// Windows Header Files:
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Local Header Files

#include <stdio.h>
#include <commdlg.h>
#include <time.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif /. !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
