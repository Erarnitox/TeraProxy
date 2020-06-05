#pragma once

#define USE_SEND_HOOK 1
#define USE_RECV_HOOK 0
#define USE_SEND_BUTTON 1

namespace game {
    typedef void(__thiscall* InternalSend)(void* thisClass, const char* data, DWORD length); //Send function signature
    InternalSend Send;
    wchar_t moduleName[]{ L"TERA.exe" };
    wchar_t title[]{ L"Erarnitox's Tera Proxy | GuidedHacking.com" }; //title displayed on the window

    //Send Hook Info:
    size_t toHookSend{ 1 }; //offset from the pattern scan position
    int sendHookLen{ 5 };

    //Recv Hook Info:
    size_t toHookRecv{ 1 }; //offset from the pattern scan position
    int recvHookLen{ 5 };
    
    //Send Hook pattern:
    const char* internalSendPattern{ "\x55\x8B\xEC\x53\x8B\xD9\x83\x7B\x0C\x00\x74\x54\x8B\x8B\x1C\x00\x02\x00\x85\xC9\x74\x2E\x8B\x01\x8B\x01\x8B\x40\x18\xFF\xD0" };
    const char* internalSendMask{ "xxxxxxxxxx??xx????xxxxxxx" };

    //Recv Hook pattern:
    const char* internalRecvPattern{ "\x55\x8B\xEC\x53\x8B\xD9\x83\x7B\x0C\x00\x74\x54\x8B\x8B\x1C\x00\x02\x00\x85\xC9\x74\x2E\x8B\x01\x8B\x01\x8B\x40\x18\xFF\xD0" };
    const char* internalRecvMask{ "xxxxxxxxxx??xx????xxxxxxx" };

    /*
    This is the code that will be placed at the hooked send position. 
    Place a char* to the buffer in the variable sentBuffer
    and store it's length in sentLength.
    We back up all registers in variables so we can guarantee to be able to restore them later.
    */
    void __declspec(naked) sendHookFunc() {
        __asm {
            mov thisPTR, ecx
            mov teax, eax; backup registers
            mov tebx, ebx
            mov tecx, ecx
            mov tedx, edx
            mov tesi, esi
            mov tedi, edi
            mov tebp, ebp
            mov tesp, esp; end of backup
            mov eax, [esp + 0x8]
            mov sentBuffer, eax
            mov eax, [esp + 0xC]
            mov sentLen, eax
        }
        if (logSentHook) {
            //This call will print the contents of sentBuffer to the Log window:
            printSendBufferToLog();
        }
        /*In the next block we restore our registers and execute the overwritten code where our hook got placed.
        Then we jump back to normal execution. 
        The jmpBackAddrSend variable contains the correct address! DONT WRITE TO IT!*/
        __asm {
            mov eax, teax; start to restore registers
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

    /*
    This is the code that will be placed at the hooked recv position.
    Place a char* to the buffer in the variable recvBuffer
    and store it's length in recvLength.
    We back up all registers in variables so we can guarantee to be able to restore them later.
    */
    void __declspec(naked) recvHookFunc() {
        __asm {
            mov rebx, ebx; backup registers
            mov recx, ecx
            mov redx, edx
            mov resi, esi
            mov redi, edi
            mov rebp, ebp
            mov resp, esp; end backup
            mov eax, [esp + 0x8]
            mov recvBuffer, eax
            mov eax, [esp + 0xC]
            mov recvLen, eax
        }
        if (logRecvHook) {
            //This call will print the contents of recvBuffer to the Log window:
            printRecvBufferToLog();
        }
        /*In the next block we restore our registers and execute the overwritten code where our hook got placed.
        Then we jump back to normal execution.
        The jmpBackAddrRecv variable contains the correct address! DONT WRITE TO IT!*/
        __asm {
            mov eax, reax; restore registers
            mov ebx, rebx
            mov ecx, recx
            mov edx, redx
            mov esi, resi
            mov edi, redi
            mov ebp, rebp
            mov esp, resp; end restore
            mov ebp, esp
            push ebx
            mov ebx, ecx
            jmp[jmpBackAddrRecv]
        }
    }
}