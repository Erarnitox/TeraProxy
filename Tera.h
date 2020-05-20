#pragma once
typedef void (__thiscall* InternalSend)(void* thisClass, const char* data, DWORD length);
InternalSend Send;
void* thisPTR;
wchar_t moduleName[] = L"TERA.exe";
//size_t sendFuncOffset = 0x1045270;
size_t toHookSend = 1;
int sendHookLen = 5;
size_t recvFuncOffset = 0x1045270;
size_t toHookRecv = 3;
int recvHookLen = 5;
DWORD sentLen;
char* sentBuffer;
char* tmpBuffer;

const char* internalSendPattern = "\x55\x8B\xEC\x53\x8B\xD9\x83\x7B\x0C\x00\x74\x54\x8B\x8B\x1C\x00\x02\x00\x85\xC9\x74\x2E\x8B\x01\x8B\x01\x8B\x40\x18\xFF\xD0";
const char* internalSendMask = "xxxxxxxxxx??xx????xxxxxxx";

const char* internalRecvPattern = "\xAB\xCD\xEF";
const char* internalRecvMask = "xxx";

bool logSentHook = false;
bool logRecvHook = false;

void* teax;
void* tebx;
void* tecx;
void* tedx;
void* tesi;
void* tedi;
void* tebp;
void* tesp;
struct MovementPacket {
	char data[0x18];
};

void printSendBufferToLog();

DWORD jmpBackAddrSend;
void __declspec(naked) sendHookFunc() {
    __asm {
        mov thisPTR, ecx
        mov teax, eax; backup
        mov tebx, ebx
        mov tecx, ecx
        mov tedx, edx
        mov tesi, esi
        mov tedi, edi
        mov tebp, ebp
        mov tesp, esp
        mov eax, [esp + 0x8]
        mov sentBuffer, eax
        mov eax, [esp + 0xC]
        mov sentLen, eax
    }
    if (logSentHook) {
        printSendBufferToLog();
    }
    __asm{
        mov eax, teax
        mov ebx, tebx
        mov ecx, tecx
        mov edx, tedx
        mov esi, tesi
        mov edi, tedi
        mov ebp, tebp
        mov esp, tesp; end of restore
        mov ebp, esp
        push ebx
        mov ebx, ecx
        jmp[jmpBackAddrSend]
    }
}