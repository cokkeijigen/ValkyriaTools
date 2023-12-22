#include "../includes.h"

/**
    Game: プレイ！プレイ！プレイ！ロック！
    Version: VAL-0064
    https://vndb.org/v3884
*/

namespace Hook::Mem {

    bool MemWrite(uintptr_t Addr, void* Buf, size_t Size) {
        DWORD OldPro = NULL; SIZE_T wirteBytes = NULL;
        if (VirtualProtect((VOID*)Addr, Size, PAGE_EXECUTE_READWRITE, &OldPro)) {
            WriteProcessMemory(INVALID_HANDLE_VALUE, (VOID*)Addr, Buf, Size, &wirteBytes);
            VirtualProtect((VOID*)Addr, Size, OldPro, &OldPro);
            return Size == wirteBytes;
        }
        return false;
    }

    bool JmpWrite(uintptr_t orgAddr, uintptr_t tarAddr) {
        uint8_t jmp_write[5] = { 0xE9, 0x0, 0x0, 0x0, 0x0 };
        tarAddr = tarAddr - orgAddr - 5;
        memcpy(jmp_write + 1, &tarAddr, 4);
        return MemWrite(orgAddr, jmp_write, 5);
    }
}

namespace Hook::Type {
    typedef HFONT(WINAPI* CreateFontIndirectA)(LOGFONTA* lplf);
    typedef int(WINAPI* MultiByteToWideChar)(UINT, DWORD, LPCCH, int, LPWSTR, int);
}

namespace Hook {

    const char* chsFilePath = ".\\cn_Data\\";
    const char* wdTitleName = "プレイ！プレイ！プレイ！ロック！ Test";
    const char* newFontName = "黑体";
    const int32_t nCodePage = 936;
    DWORD BaseAddr, GDI32_DLL, KERNEL32_DLL;

    Type::CreateFontIndirectA OldCreateFontIndirectA;
    HFONT WINAPI NewCreateFontIndirectA(LOGFONTA* lplf) {
        strcpy(lplf->lfFaceName, newFontName);
        return OldCreateFontIndirectA(lplf);
    }

    Type::MultiByteToWideChar OldMultiByteToWideChar;
    int WINAPI NewMultiByteToWideChar(UINT CodePage, DWORD dwF, LPCCH lpMBS, int cbMB, LPWSTR lpWCS, int cchWC) {
        return OldMultiByteToWideChar(CodePage == 932 ? nCodePage : CodePage, dwF, lpMBS, cbMB, lpWCS, cchWC);
    }
  
    void __stdcall hook_script_read(DWORD esi, HANDLE hFile, DWORD fileInfo, LONG offset) {
        //printf("hook_script_read:\n");
        //printf("esi: 0x%p hFile: 0x%p fileInfo: 0x%p offse: 0x%X\n", (void*)esi, (void*)hFile, (void*)fileInfo, (int)offset);
        char* fileName = (char*)(fileInfo + 0x04);
        uint32_t* bufsz = (uint32_t*)(esi + 0x0C);
        intptr_t* bfptr = (intptr_t*)(esi + 0x08);
        std::string filePath(chsFilePath);
        filePath.append(fileName);
        //printf("fileName: %s\n", fileName);
        if (GetFileAttributesA(filePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            FILE* fp = fopen(filePath.c_str(), "rb");
            if (!fp) goto __original;
            fseek(fp, 0x00, SEEK_END);
            *bufsz = ftell(fp);
            fseek(fp, 0x00, SEEK_SET);
            *bfptr = (intptr_t)(new char[*bufsz]);
            fread((char*)*bfptr, 0x01, *bufsz, fp);
            fclose(fp);
            printf("replace file: %s\n", fileName);
        }
        else {
        __original:
            DWORD lpNumberOfBytesRead = NULL;
            *bufsz = *(int32_t*)fileInfo;
            //printf("fileSize: 0x%X\n", (int)*bufsz);
            *bfptr = (intptr_t)(new char[*bufsz]);
            SetFilePointer(hFile, offset, 0x00, 0x00);
            ReadFile(hFile, (LPVOID)*bfptr, *bufsz, &lpNumberOfBytesRead, 0x00);
        }
        CloseHandle(hFile);
    }

    static void Init() {
        if (BaseAddr = (DWORD)GetModuleHandleA(NULL)) {
            Mem::MemWrite(BaseAddr + 0x9BE88, &wdTitleName, 0x04); // 修改窗口标题
            uint8_t code[] = {
                0xFF, 0x75, 0xFC,       // push dword ptr ss:[ebp-0x04] <- offset
                0xFF, 0x74, 0x24, 0x1C, // push dword ptr ss:[esp+0x1C] <- fileInfo
                0x53, // push ebx   <- hFile
                0x56, // push esi   <- esi
                0xE8, 0x00, 0x00, 0x00, 0x00, // call hook_script_read
                0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                0x90, 0x90 // ...nop
            };
            intptr_t orgAddr = BaseAddr + 0x1984B;
            intptr_t tarAddr = (intptr_t)hook_script_read;
            intptr_t rvaAddr = tarAddr - (orgAddr + 0x09) - 0x05;
            Mem::MemWrite((intptr_t)&code[0x0A], &rvaAddr, 0x04);
            Mem::MemWrite((intptr_t)orgAddr, code, sizeof(code)); // hook脚本读取
            Mem::MemWrite(BaseAddr + 0x19967, &code[0x0E], 0x06); // 去除校验判断
        }
        if (GDI32_DLL = (DWORD)GetModuleHandleA("gdi32.dll")) {
            OldCreateFontIndirectA = (Type::CreateFontIndirectA)GetProcAddress(
                (HMODULE)GDI32_DLL, "CreateFontIndirectA"
            );
        }
        if (KERNEL32_DLL = (DWORD)GetModuleHandleA("kernel32.dll")) {
            OldMultiByteToWideChar = (Type::MultiByteToWideChar)GetProcAddress(
                (HMODULE)KERNEL32_DLL, "MultiByteToWideChar"
            );
        }
    }

    static void ApiHook() {
        DetourTransactionBegin();
        if (OldCreateFontIndirectA != NULL) {
            DetourAttach((void**)&OldCreateFontIndirectA, NewCreateFontIndirectA);
        }
        if (OldMultiByteToWideChar != NULL) {
            DetourAttach((void**)&OldMultiByteToWideChar, NewMultiByteToWideChar);
        }
        DetourUpdateThread(GetCurrentThread());
        DetourTransactionCommit();
    }

}

extern "C" __declspec(dllexport) void hook(void) { }
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
    #ifdef _DEBUG
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        //SetConsoleOutputCP(936);
    #endif
        Hook::Init();
        Hook::ApiHook();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}