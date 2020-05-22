#include "pch.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include "Tera.h"
#include "Hook.h"
#include "Scan.h"
#include <vector>

#define MYMENU_EXIT (WM_APP + 100)
#define SEND_BUTTON (WM_APP + 101)
#define LOG_SEND (WM_APP + 102)
//#define LOG_RECV (WM_APP + 103)
#define CLEAR_BUTTON (WM_APP + 104)

HMODULE inj_hModule;
HWND hCraftedPacket;
HWND hLog;

BOOL LogSend = 0;
BOOL LogRecv = 0;

HWND hLogSend;
HWND hLogRecv;

wchar_t craftedBuffer[533];
char bufferToSend[533];
char appedToLog[533];
char const hex_chars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

uintptr_t moduleBase;
std::vector<char> logText;

HMENU CreateDLLWindowMenu(){
    HMENU hMenu;
    hMenu = CreateMenu();
    HMENU hMenuPopup;
    if (hMenu == NULL)
        return FALSE;
    hMenuPopup = CreatePopupMenu();
    AppendMenuW(hMenuPopup, MF_STRING, MYMENU_EXIT, TEXT("Exit"));
    //AppendMenuW(hMenuPopup, MF_STRING, NULL, TEXT("Export Log"));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("File"));

    return hMenu;
}

LRESULT CALLBACK MessageHandler(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam) {
    switch (uMessage) {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case MYMENU_EXIT:
            PostQuitMessage(0);
            return 0;
            break;
        case SEND_BUTTON:
#ifdef _DEBUG
            std::cout << "Send pressed!" << std::endl;
#endif
            GetWindowText(hCraftedPacket, craftedBuffer, 533);
            size_t len;

            //convert buffer to chars:
            wcstombs_s(&len, bufferToSend, 533, craftedBuffer, 533);
#ifdef _DEBUG
            std::cout << bufferToSend << std::endl;
#endif
            //convert buffer to Bytes:
            size_t i;
            i = 0;
            for (size_t count = 0; count < len; ++i, count += 3) {
                if (bufferToSend[count] >= 'A') {
                    bufferToSend[count] -= 'A';
                    bufferToSend[count] += 10;
                }
                else {
                    bufferToSend[count] -= 48;
                }

                if (bufferToSend[count+1] >= 'A') {
                    bufferToSend[count+1] -= 'A';
                    bufferToSend[count+1] += 10;
                }
                else {
                    bufferToSend[count+1] -= 48;
                }

                bufferToSend[i] = (__int8)(((char)bufferToSend[count]) * (char)16);
                bufferToSend[i] += (__int8)bufferToSend[count + 1];
            }
            bufferToSend[i] = '\0';

#ifdef _DEBUG
            //Debug output
            for (size_t x = 0; x < i; ++x) {
                std::cout << std::hex << (((int)bufferToSend[x]) & 0xff) << " ";
            }
            std::cout << std::endl;

            //call internal send function
            std::cout << "Packet Lengh: " << i << std::endl;
#endif
            if (thisPTR != 0) {
                Send(thisPTR, bufferToSend, i);
            }
            
            break;
        case LOG_SEND:
            LogSend = IsDlgButtonChecked(hWindow, LOG_SEND);
            
            if (LogSend == BST_CHECKED) {
                CheckDlgButton(hWindow, LOG_SEND, BST_UNCHECKED);
                logSentHook = false;
            }
            else {
                CheckDlgButton(hWindow, LOG_SEND, BST_CHECKED);
                logSentHook = true;
            }
            break;
/*
        case LOG_RECV:
            LogRecv = IsDlgButtonChecked(hWindow, LOG_RECV);
#ifdef _DEBUG
            std::cout << "Recv Logging " << ((LogRecv != BST_CHECKED) ? "enabled" : "disabled") << std::endl;
#endif
            if (LogRecv == BST_CHECKED) {
                CheckDlgButton(hWindow, LOG_RECV, BST_UNCHECKED);
                logRecvHook = false;
            }
            else {
                CheckDlgButton(hWindow, LOG_RECV, BST_CHECKED);
                logRecvHook = true;
            }
            break;
*/
        case CLEAR_BUTTON:
            logText.erase(logText.begin(), logText.end());
            SetWindowTextA(hLog, "Cleared! :)\r\nFind Tutorials on Guidedhacking.com!");
        }
    }
    return DefWindowProc(hWindow, uMessage, wParam, lParam);
}

//Register our windows Class
BOOL RegisterDLLWindowClass(const wchar_t szClassName[]) {
    WNDCLASSEX wc;
    wc.hInstance = inj_hModule;
    wc.lpszClassName = (LPCWSTR)szClassName;
    wc.lpfnWndProc = MessageHandler;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hIcon = LoadIcon(NULL, IDI_SHIELD);
    wc.hIconSm = LoadIcon(NULL, IDI_SHIELD);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    if (!RegisterClassEx(&wc))
        return 0;
    return 1;
}

inline void printSendBufferToLog() {
    char sendID[] = "[SEND] ";
#ifdef _DEBUG
    std::cout << "Sent Packet len: " << std::dec << sentLen << std::endl;
#endif
    while (logText.size() > 4096) {
        logText.erase(logText.begin(), logText.begin() + 400);
    }
    if (logText.size() > 1) {
        logText.pop_back();
        logText.push_back('\r');
        logText.push_back('\n');
    }
    
    for (DWORD i = 0; i < sentLen + 7; ++i) {
        if (i < 7) {
            logText.push_back(sendID[i]);
        }
        else {
            logText.push_back(hex_chars[((sentBuffer)[i - 7] & 0xF0) >> 4]);
            logText.push_back(hex_chars[((sentBuffer)[i - 7] & 0x0F) >> 0]);
            logText.push_back(' ');
        }
    }
    logText.push_back('\0');
    SetWindowTextA(hLog, &logText[0]);
}

/*
//Might have to use Semapores if Recv and Send run in different threads
inline void printRecvBufferToLog() {
    char recvID[] = "[RECV] ";
#ifdef _DEBUG
    std::cout << "Recieved Packet len: " << std::dec << recvLen << std::endl;
#endif
    while (logText.size() > 4096) {
        logText.erase(logText.begin(), logText.begin() + 400);
    }
    if (logText.size() > 1) {
        logText.pop_back();
        logText.push_back('\r');
        logText.push_back('\n');
    }

    for (DWORD i = 0; i < recvLen + 7; ++i) {
        if (i < 7) {
            logText.push_back(recvID[i]);
        }
        else {
            logText.push_back(hex_chars[((recvBuffer)[i - 7] & 0xF0) >> 4]);
            logText.push_back(hex_chars[((recvBuffer)[i - 7] & 0x0F) >> 0]);
            logText.push_back(' ');
        }
    }
    logText.push_back('\0');
    SetWindowTextA(hLog, &logText[0]);
}*/

DWORD WINAPI WindowThread(HMODULE hModule){

#ifdef _DEBUG
    //Create Console
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "DLL got injected!" << std::endl;
#endif 

    logText = std::vector<char>();

    moduleBase = (uintptr_t)GetModuleHandle(moduleName);
    Send = (InternalSend)(ScanInternal(internalSendPattern, internalSendMask, (char*)(moduleBase+ 0x0500000), 0x3000000));
    //void* toHookRecv = (void*)(moduleBase+0x10097D6);//(ScanInternal(internalRecvPattern, internalRecvMask, (char*)(moduleBase + 0x0500000), 0x3000000));

#ifdef _DEBUG
    std::cout << "send function location:" << std::hex << (int)Send << std::endl;
#endif // _DEBUG

    toHookSend += (size_t)Send;
    jmpBackAddrSend = toHookSend + sendHookLen;
    //jmpBackAddrRecv = (size_t)toHookRecv + recvHookLen;

#ifdef _DEBUG
    std::cout << "[Send Jump Back Addy:] 0x" << std::hex << jmpBackAddrSend << std::endl;
#endif

    Hook* sendHook = new Hook((void*)toHookSend, (void*)sendHookFunc, sendHookLen);
    //Hook* recvHook = new Hook(toHookRecv, recvHookFunc, recvHookLen);

    MSG messages;
    HMENU hMenu = CreateDLLWindowMenu();
    HWND hSendButton;
    HWND hClearButton;
    
    RegisterDLLWindowClass(L"InjectedDLLWindowClass");
    HWND hwnd = CreateWindowEx(0, L"InjectedDLLWindowClass", L"Erarnitox's Tera Proxy | GuidedHacking.com", WS_EX_LAYERED, CW_USEDEFAULT, CW_USEDEFAULT, 1020, 885, NULL, hMenu, inj_hModule, NULL);
    hLog = CreateWindowEx(0, L"edit", L"Tera Proxy made by Erarnitox\r\n!!! visit GuidedHacking.com !!!", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER | ES_READONLY, 5, 5, 1005, 700, hwnd, NULL, hModule, NULL);

    hClearButton = CreateWindowEx(0, L"button", L"Clear Log", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 710, 100, 30, hwnd, (HMENU)CLEAR_BUTTON, hModule, NULL);
    hSendButton = CreateWindowEx(0, L"button", L"Send", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 800, 100, 30, hwnd, (HMENU)SEND_BUTTON, hModule, NULL);
    hCraftedPacket = CreateWindowEx(0, L"edit", L"<Packet Data>", WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_BORDER, 110, 730, 900, 100, hwnd, NULL, hModule, NULL);

    hLogSend = CreateWindowEx(0, L"button", L"Log Send", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 110, 705, 100, 25, hwnd, (HMENU)LOG_SEND, hModule, NULL);
    //hLogRecv = CreateWindowEx(0, L"button", L"Log Recv", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 210, 705, 100, 25, hwnd, (HMENU)LOG_RECV, hModule, NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd); // redraw window;

    while (GetMessage(&messages, NULL, 0, 0)){
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    //exit:
    //delete recvHook;
    delete sendHook;
    

#ifdef _DEBUG
    if (f != 0) {
        fclose(f);
    }   
    FreeConsole();
#endif 

    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
    switch (ul_reason_for_call){
        case DLL_PROCESS_ATTACH:
            inj_hModule = hModule;
            HANDLE ThreadHandle = CreateThread(0, NULL, (LPTHREAD_START_ROUTINE)WindowThread, hModule, NULL, NULL);
            
            if (ThreadHandle != NULL) {
                CloseHandle(ThreadHandle);
            }
        break;
    }
    return TRUE;
}

