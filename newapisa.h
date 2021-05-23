/*
    New APIs A
    Drop-in replacement for Windows SDK <include/newapis.h> header.

    (c) 2021 Ai4rei/AN
    Licensed under the Non-Profit Open Software License version 3.0
*/

/*
    This implements a fallback mechanism for APIs that did not exist
    on Windows 95 or Windows NT version prior 4.0 SP3.

    Usage:

    To enable a specific wrapper, define one or more following:

    Function              | Define
    ----------------------+-----------------------------------------
    GetDiskFreeSpaceEx    | WANT_GETDISKFREESPACEEX_WRAPPER
    GetFileAttributesEx   | WANT_GETFILEATTRIBUTESEX_WRAPPER
    GetLongPathName       | WANT_GETLONGPATHNAME_WRAPPER (not implemented)
    IsDebuggerPresent     | WANT_ISDEBUGGERPRESENT_WRAPPER

    In addition one source file has to define all used defines above
    and in addition COMPILE_NEWAPIS_STUBS.

    All defines must be specified before including this file.
*/

#ifndef GITBITS_NEWAPIS_NEWAPISA_H
#define GITBITS_NEWAPIS_NEWAPISA_H

/* on Win64 this is not needed (newapis.h does not exist there in
the first place) */
#ifndef _WIN64

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*
    GetDiskFreeSpaceEx
*/
#ifdef WANT_GETDISKFREESPACEEX_WRAPPER

#undef GetDiskFreeSpaceEx
#define GetDiskFreeSpaceEx _GetDiskFreeSpaceEx

typedef BOOL (WINAPI* LPFNGETDISKFREESPACEEX)
(
    LPCTSTR         lpszDirectory,
    PULARGE_INTEGER lpuliFreeBytesAvailable,
    PULARGE_INTEGER lpuliTotalNumberOfBytes,
    PULARGE_INTEGER lpuliTotalNumberOfFreeBytes
);
extern LPFNGETDISKFREESPACEEX GetDiskFreeSpaceEx;

#ifdef COMPILE_NEWAPIS_STUBS

/* This kicks in, when running on a system before the introduction
of FAT32, where disk sizes are up to 2GB (FAT16). */
static BOOL WINAPI NewApisA_GetDiskFreeSpaceEx_Fallback
(
    LPCTSTR         lpszDirectory,
    PULARGE_INTEGER lpuliFreeBytesAvailable,
    PULARGE_INTEGER lpuliTotalNumberOfBytes,
    PULARGE_INTEGER lpuliTotalNumberOfFreeBytes
)
{
    DWORD dwSectorsPerCluster = 0, dwBytesPerSector = 0, dwNumberOfFreeClusters = 0, dwTotalNumberOfClusters = 0;

    if(GetDiskFreeSpace(lpszDirectory, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
        DWORD const dwBytesPerCluster = dwBytesPerSector*dwSectorsPerCluster;

        if(lpuliFreeBytesAvailable!=NULL)
        {
            lpuliFreeBytesAvailable->QuadPart = UInt32x32To64(dwBytesPerCluster, dwNumberOfFreeClusters);
        }
        if(lpuliTotalNumberOfBytes!=NULL)
        {
            lpuliTotalNumberOfBytes->QuadPart = UInt32x32To64(dwBytesPerCluster, dwTotalNumberOfClusters);
        }
        if(lpuliTotalNumberOfFreeBytes!=NULL)
        {
            lpuliTotalNumberOfFreeBytes->QuadPart = UInt32x32To64(dwBytesPerCluster, dwNumberOfFreeClusters);
        }

        return TRUE;
    }

    return FALSE;
}

static BOOL WINAPI NewApisA_GetDiskFreeSpaceEx_Loader
(
    LPCTSTR         lpszDirectory,
    PULARGE_INTEGER lpuliFreeBytesAvailable,
    PULARGE_INTEGER lpuliTotalNumberOfBytes,
    PULARGE_INTEGER lpuliTotalNumberOfFreeBytes
)
{
    LPFNGETDISKFREESPACEEX const Func = (LPFNGETDISKFREESPACEEX)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
#ifndef UNICODE
        "GetDiskFreeSpaceExA"
#else  /* UNICODE */
        "GetDiskFreeSpaceExW"
#endif  /* UNICODE */
    );

    if(Func!=NULL)
    {
        BOOL const bResult = Func(lpszDirectory, lpuliFreeBytesAvailable, lpuliTotalNumberOfBytes, lpuliTotalNumberOfFreeBytes);

        if(bResult || GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED)
        {
            GetDiskFreeSpaceEx = Func;
            return bResult;
        }
    }

    GetDiskFreeSpaceEx = &NewApisA_GetDiskFreeSpaceEx_Fallback;

    return NewApisA_GetDiskFreeSpaceEx_Fallback(lpszDirectory, lpuliFreeBytesAvailable, lpuliTotalNumberOfBytes, lpuliTotalNumberOfFreeBytes);
}

LPFNGETDISKFREESPACEEX GetDiskFreeSpaceEx = &NewApisA_GetDiskFreeSpaceEx_Loader;

#endif  /* COMPILE_NEWAPIS_STUBS */

#endif  /* WANT_GETDISKFREESPACEEX_WRAPPER */

/*
    GetFileAttributesEx
*/
#ifdef WANT_GETFILEATTRIBUTESEX_WRAPPER

/* In case the SDK is not even aware of this function. */
#ifndef GetFileAttributesEx

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;
typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

#endif  /* GetFileAttributesEx */

#undef GetFileAttributesEx
#define GetFileAttributesEx _GetFileAttributesEx

typedef BOOL (WINAPI* LPFNGETFILEATTRIBUTESEX)
(
    LPCTSTR                 lpszFileName,
    GET_FILEEX_INFO_LEVELS  nInfoLevelId,
    LPVOID                  lpFileInformation
);
extern LPFNGETFILEATTRIBUTESEX GetFileAttributesEx;

#ifdef COMPILE_NEWAPIS_STUBS

static BOOL WINAPI NewApisA_GetFileAttributesEx_Fallback
(
    LPCTSTR                 lpszFileName,
    GET_FILEEX_INFO_LEVELS  nInfoLevelId,
    LPVOID                  lpFileInformation
)
{
    if(nInfoLevelId==GetFileExInfoStandard)
    {
        if(GetFileAttributes(lpszFileName)!=INVALID_FILE_ATTRIBUTES)
        {
            WIN32_FIND_DATA Wfd;
            HANDLE const hFind = FindFirstFile(lpszFileName, &Wfd);

            if(hFind!=INVALID_HANDLE_VALUE)
            {
                LPWIN32_FILE_ATTRIBUTE_DATA const lpWfad = lpFileInformation;

                FindClose(hFind);

                lpWfad->dwFileAttributes = Wfd.dwFileAttributes;
                lpWfad->ftCreationTime   = Wfd.ftCreationTime;
                lpWfad->ftLastAccessTime = Wfd.ftLastAccessTime;
                lpWfad->ftLastWriteTime  = Wfd.ftLastWriteTime;
                lpWfad->nFileSizeHigh    = Wfd.nFileSizeHigh;
                lpWfad->nFileSizeLow     = Wfd.nFileSizeLow;

                return TRUE;
            }
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}

static BOOL WINAPI NewApisA_GetFileAttributesEx_Loader
(
    LPCTSTR                 lpszFileName,
    GET_FILEEX_INFO_LEVELS  nInfoLevelId,
    LPVOID                  lpFileInformation
)
{
    LPFNGETFILEATTRIBUTESEX const Func = (LPFNGETFILEATTRIBUTESEX)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
#ifndef UNICODE
        "GetFileAttributesExA"
#else  /* UNICODE */
        "GetFileAttributesExW"
#endif  /* UNICODE */
    );

    if(Func!=NULL)
    {
        BOOL const bResult = Func(lpszFileName, nInfoLevelId, lpFileInformation);

        if(bResult || GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED)
        {
            GetFileAttributesEx = Func;
            return bResult;
        }
    }

    GetFileAttributesEx = &NewApisA_GetFileAttributesEx_Fallback;

    return NewApisA_GetFileAttributesEx_Fallback(lpszFileName, nInfoLevelId, lpFileInformation);
}

LPFNGETFILEATTRIBUTESEX GetFileAttributesEx = &NewApisA_GetFileAttributesEx_Loader;

#endif  /* COMPILE_NEWAPIS_STUBS */

#endif  /* WANT_GETFILEATTRIBUTESEX_WRAPPER */

/*
    GetLongPathName
*/
#ifdef WANT_GETLONGPATHNAME_WRAPPER
#endif  /* WANT_GETLONGPATHNAME_WRAPPER */

/*
    IsDebuggerPresent
*/
#ifdef WANT_ISDEBUGGERPRESENT_WRAPPER

#define IsDebuggerPresent _IsDebuggerPresent

typedef BOOL (WINAPI* LPFNISDEBUGGERPRESENT)(VOID);
extern LPFNISDEBUGGERPRESENT IsDebuggerPresent;

#ifdef COMPILE_NEWAPIS_STUBS

static BOOL WINAPI NewApisA_IsDebuggerPresent_Fallback(VOID)
{
    return FALSE;  /* no way to tell, assume no */
}

static BOOL WINAPI NewApisA_IsDebuggerPresent_Loader(VOID)
{
    LPFNISDEBUGGERPRESENT const Func = (LPFNISDEBUGGERPRESENT)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "IsDebuggerPresent");

    if(Func!=NULL)
    {
        IsDebuggerPresent = Func;
    }
    else
    {
        IsDebuggerPresent = &NewApisA_IsDebuggerPresent_Fallback;
    }

    return IsDebuggerPresent();
}

LPFNISDEBUGGERPRESENT IsDebuggerPresent = &NewApisA_IsDebuggerPresent_Loader;

#endif  /* COMPILE_NEWAPIS_STUBS */

#endif  /* WANT_ISDEBUGGERPRESENT_WRAPPER */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _WIN64 */

#endif  /* GITBITS_NEWAPIS_NEWAPISA_H */
