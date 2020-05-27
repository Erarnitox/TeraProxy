#include "pch.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include "Tera.h"
#include "Hook.h"
#include "Scan.h"
#include <vector>
#include <string>

#define MYMENU_EXIT (WM_APP + 100)
#define SEND_BUTTON (WM_APP + 101)
#define LOG_SEND (WM_APP + 102)
#define LOG_FILTER (WM_APP + 105)
//#define LOG_RECV (WM_APP + 103)
#define CLEAR_BUTTON (WM_APP + 104)

#define SEND_SEQ (WM_APP + 106)
#define SEND_LOOP (WM_APP + 107)
#define STOP_LOOP (WM_APP + 108)
#define LOAD_SEQ (WM_APP + 109)
#define EXP_SEQ (WM_APP + 110)

#define INC_FNT (WM_APP + 111)
#define DEC_FNT (WM_APP + 112)

HMODULE inj_hModule;
HWND hCraftedPacket;
HWND hLog;

BOOL LogSend = FALSE;
BOOL LogRecv = FALSE;
BOOL FilterLog = FALSE;

HWND hLogSend;
HWND hLogRecv;
HWND hFilterLog;

wchar_t craftedBuffer[533];
char bufferToSend[533];
char appedToLog[533];
char const hex_chars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

uintptr_t moduleBase;

HMENU CreateDLLWindowMenu(){
    HMENU hMenu;
    hMenu = CreateMenu();
    HMENU hFileMenuPopup;
    HMENU hToolMenuPopup;
    HMENU hFontMenuPopup;

    if (hMenu == NULL)
        return FALSE;
    hFileMenuPopup = CreatePopupMenu();
    hToolMenuPopup = CreatePopupMenu();
    hFontMenuPopup = CreatePopupMenu();

    AppendMenuW(hFileMenuPopup, MF_STRING, MYMENU_EXIT, TEXT("Exit"));
    AppendMenuW(hFileMenuPopup, MF_STRING, LOAD_SEQ, TEXT("Load Sequenz"));
    AppendMenuW(hFileMenuPopup, MF_STRING, NULL, TEXT("Export Log as Sequenz"));

    AppendMenuW(hToolMenuPopup, MF_STRING, CLEAR_BUTTON, TEXT("Clear"));
    AppendMenuW(hToolMenuPopup, MF_STRING, SEND_SEQ, TEXT("Send Sequenz"));
    AppendMenuW(hToolMenuPopup, MF_STRING, SEND_LOOP, TEXT("Start Sequenz Loop"));
    AppendMenuW(hToolMenuPopup, MF_STRING, STOP_LOOP, TEXT("Stop Sequenz Loop"));

    AppendMenuW(hFontMenuPopup, MF_STRING, INC_FNT, TEXT("Increase Font Size"));
    AppendMenuW(hFontMenuPopup, MF_STRING, DEC_FNT, TEXT("Decrease Font Size"));

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenuPopup, TEXT("File"));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hToolMenuPopup, TEXT("Tools"));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFontMenuPopup, TEXT("Font"));

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

                if (bufferToSend[count] >= 'a') {
                    bufferToSend[count] -= 0x20;
                }

                if (bufferToSend[count+1] >= 'a') {
                    bufferToSend[count+1] -= 0x20;
                }
               
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
            SetWindowTextA(hLog, "");
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
    
#ifdef _DEBUG
    std::cout << "Sent Packet len: " << std::dec << sentLen << std::endl;
#endif

    if (FilterLog) {
        //filter packets
    }

    int index = GetWindowTextLength(hLog);
    SendMessageA(hLog, EM_SETSEL, (WPARAM)index, (LPARAM)index); // set selection - end of text
    std::string buffer = "[SEND]";
    
    for (DWORD i = 0; i < sentLen; ++i) {
        buffer += (hex_chars[((sentBuffer)[i] & 0xF0) >> 4]);
        buffer += (hex_chars[((sentBuffer)[i] & 0x0F) >> 0]);
        buffer += ' ';
    }
    buffer += "\r\n";
    std::cout << buffer << std::endl;
    SendMessageA(hLog, EM_REPLACESEL, 0, (LPARAM)buffer.c_str());
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
    HFONT hLogFont = CreateFontA(14,0,0,0,0,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE | DEFAULT_PITCH,"Lucida Console");

    RegisterDLLWindowClass(L"InjectedDLLWindowClass");
    HWND hwnd = CreateWindowEx(0, L"InjectedDLLWindowClass", L"Erarnitox's Tera Proxy | GuidedHacking.com", WS_EX_LAYERED /*| WS_EX_APPWINDOW*/, CW_USEDEFAULT, CW_USEDEFAULT, 1020, 885, NULL, hMenu, inj_hModule, NULL);
    hLog = CreateWindowEx(0, L"edit", L"", WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_NOHIDESEL, 5, 5, 1005, 700, hwnd, NULL, hModule, NULL);
    SendMessage(hLog,WM_SETFONT,(WPARAM)hLogFont,0);

    hClearButton = CreateWindowEx(0, L"button", L"Clear Log", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 710, 100, 30, hwnd, (HMENU)CLEAR_BUTTON, hModule, NULL);
    hSendButton = CreateWindowEx(0, L"button", L"Send", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 800, 100, 30, hwnd, (HMENU)SEND_BUTTON, hModule, NULL);
    hCraftedPacket = CreateWindowEx(0, L"edit", L"<Packet Data>", WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_BORDER, 110, 730, 900, 100, hwnd, NULL, hModule, NULL);
    SendMessage(hCraftedPacket, WM_SETFONT, (WPARAM)hLogFont, 0);

    hLogSend = CreateWindowEx(0, L"button", L"Log Send", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 110, 705, 100, 25, hwnd, (HMENU)LOG_SEND, hModule, NULL);
    //hLogRecv = CreateWindowEx(0, L"button", L"Log Recv", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 210, 705, 100, 25, hwnd, (HMENU)LOG_RECV, hModule, NULL);

    hFilterLog = CreateWindowEx(0, L"button", L"Filter Log", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 910, 705, 100, 25, hwnd, (HMENU)LOG_FILTER, hModule, NULL);

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

