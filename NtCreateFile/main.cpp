#include <Windows.h>
#include <stdio.h>
#include "windows_struct.h"

int createTestFile()
{
    HANDLE hCurtProc = GetCurrentProcess();

#ifdef _WIN64
    BYTE byteNtCreateFile[0x1000]{
        0x4C, 0x8B, 0xD1,                                   // mov r10, rcx
        0xB8, 0x55, 0x00, 0x00, 0x00,                       // mov eax, 55h
        0xF6, 0x04, 0x25, 0x08, 0x03, 0xFE, 0x7F, 0x01,     // test byte ptr [7FFE0308h], 1
        0x75, 0x03,                                         // jne  ELSE  
        0x0F, 0x05,                                         // syscall
        0xC3,                                               // retn
        // ELSE:
        0xCD, 0x2E,                                         // int 2Eh
        0xC3                                                // retn
    };
#else
    BYTE byteNtCreateFile[0x1000]{
        0xB8, 0x55, 0x00, 0x00, 0x00,                       // mov eax, 55h
        0xBA, 0x40, 0x8D, 0x50, 0x77,                       // mov edx, offset j_Wow64Transition
        0xFF, 0xD2,                                         // syscall
        0xC2, 0x2C, 0x00,                                   // retn
        0x90                                                // nop
    };
#endif


    void* pShellcode = VirtualAllocEx(hCurtProc, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (pShellcode == NULL)
    {
        printf("VirtualAlloc failed 0x%X\n", GetLastError());
        return false;
    }

    BOOL bWriteMem = WriteProcessMemory(hCurtProc, pShellcode, byteNtCreateFile, sizeof(byteNtCreateFile), nullptr);
    if (bWriteMem == 0)
    {
        printf("WriteProcessMemory failed 0x%X\n", GetLastError());
        VirtualFreeEx(hCurtProc, pShellcode, 0, MEM_RELEASE);
        return false;
    }

    HANDLE hFile = NULL;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatusBlock;

    // Initalize Unicode String
    UNICODE_STRING ucFilepath;
    WCHAR wcFilepath[100] = L"\\??\\\\C:\\dev\\testFile.txt";
    RtlInitUnicodeString(&ucFilepath, wcFilepath);

    // Initialize object 
    InitializeObjectAttributes(&objAttr, &ucFilepath, OBJ_CASE_INSENSITIVE, NULL, NULL);


    NtCreateFile funcNtCreateFile = (NtCreateFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtCreateFile");
    ///NtCreateFile funcNtCreateFile = (NtCreateFile)pShellcode;

    NTSTATUS stat = funcNtCreateFile(&hFile, FILE_GENERIC_WRITE, &objAttr, &ioStatusBlock, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE, FILE_OVERWRITE_IF,
        FILE_RANDOM_ACCESS | FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (!NT_SUCCESS(stat))
    {
        printf("NtCreateFile failed 0x%X\n", GetLastError());
        VirtualFreeEx(hCurtProc, pShellcode, 0, MEM_RELEASE);
        return false;
    }

    BOOL bClose = CloseHandle(hFile);
    if (bClose == FALSE)
    {
        printf("CloseHandle failed 0x%X\n", GetLastError());
        VirtualFreeEx(hCurtProc, pShellcode, 0, MEM_RELEASE);
        return false;
    }

    bool bFreeMem = VirtualFreeEx(hCurtProc, pShellcode, 0, MEM_RELEASE);
    if (bFreeMem == 0)
    {
        printf("VirutalFreeEx failed 0x%X\n", GetLastError());
        return false;
    }

    return true;
}


int main()
{
    if (createTestFile() == true)
        printf("createTestFile sucess\n");

	return 0;
}