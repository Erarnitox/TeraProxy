#include "pch.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include "Tera.h"
#include "Hook.h"
#include "Scan.h"
#include <vector>
#include <string>
#include <commdlg.h>
#include <fstream>
#include <thread>

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
#define LOAD_FILTER (WM_APP + 113)
#define EXPORT_LOG (WM_APP + 114)

HMODULE inj_hModule;
HWND hCraftedPacket;
HWND hLog;

BOOL LogSend = FALSE;
BOOL LogRecv = FALSE;
BOOL FilterLog = FALSE;

bool filterLog = false;
bool running = true;

HWND hwnd;
HWND hLogSend;
HWND hLogRecv;
HWND hFilterLog;
HFONT hLogFont;

wchar_t craftedBuffer[533];
char bufferToSend[533];
char appedToLog[533];
char const hex_chars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

uintptr_t moduleBase;
size_t fntSize = 14;

std::vector<std::string> filter;
std::vector<std::string> sequence;

void OpenFile(char* filename, bool save, bool filter = false);
HMENU CreateDLLWindowMenu();
BOOL RegisterDLLWindowClass(const wchar_t szClassName[]);
LRESULT CALLBACK MessageHandler(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL RegisterDLLWindowClass(const wchar_t szClassName[]);
DWORD WINAPI WindowThread(HMODULE hModule);
void hotKeys();
void exitLogger();

void OpenFile(char* filename, bool save, bool filter) {
    filename[0] = 0;
    OPENFILENAMEA saveDialog = {};
    saveDialog.lStructSize = sizeof(OPENFILENAMEA);
    saveDialog.hwndOwner = 0;
    saveDialog.hInstance = 0;

    if (save) {
        saveDialog.lpstrFilter = "Log Files\0*.log";
        saveDialog.lpstrDefExt = ".log";
        saveDialog.lpstrTitle = "Export Log File";
        saveDialog.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    }else if(!filter){
        saveDialog.lpstrFilter = "Sequence Files\0*.seq";
        saveDialog.lpstrDefExt = ".seq";
        saveDialog.lpstrTitle = "Load Sequnce file to replay";
        saveDialog.Flags = OFN_NOCHANGEDIR;
    }else {
        saveDialog.lpstrFilter = "Filter Files\0*.filter";
        saveDialog.lpstrDefExt = ".filter";
        saveDialog.lpstrTitle = "Load Filter File";
        saveDialog.Flags = OFN_NOCHANGEDIR;
    }

    saveDialog.lpstrCustomFilter = 0;
    saveDialog.nMaxCustFilter = 0;
    saveDialog.nFilterIndex = 1;
    saveDialog.lpstrFile = filename;
    saveDialog.nMaxFile = 256;
    saveDialog.lpstrFileTitle = 0;
    saveDialog.nMaxFileTitle = 0;
    saveDialog.lpstrInitialDir = 0;
    saveDialog.nFileOffset = 0;
    saveDialog.nFileExtension = 1;
    saveDialog.lCustData = 0;
    saveDialog.lpfnHook = 0;
    saveDialog.lpTemplateName = 0;
    saveDialog.pvReserved = 0;
    saveDialog.dwReserved = 0;
    saveDialog.FlagsEx = 0;
    GetSaveFileNameA(&saveDialog);
}
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
    AppendMenuW(hFileMenuPopup, MF_STRING, LOAD_FILTER, TEXT("Load Filter"));
    AppendMenuW(hFileMenuPopup, MF_STRING, EXPORT_LOG, TEXT("Export Log"));

    AppendMenuW(hToolMenuPopup, MF_STRING, CLEAR_BUTTON, TEXT("Clear (F9)"));
    AppendMenuW(hToolMenuPopup, MF_STRING, SEND_SEQ, TEXT("Send Sequenz (F10)"));
    AppendMenuW(hToolMenuPopup, MF_STRING, SEND_LOOP, TEXT("Start Sequenz Loop (F11)"));
    AppendMenuW(hToolMenuPopup, MF_STRING, STOP_LOOP, TEXT("Stop Sequenz Loop (F12)"));

    AppendMenuW(hFontMenuPopup, MF_STRING, INC_FNT, TEXT("Increase Font Size (Numpad+)"));
    AppendMenuW(hFontMenuPopup, MF_STRING, DEC_FNT, TEXT("Decrease Font Size (Numpad-)"));

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenuPopup, TEXT("File"));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hToolMenuPopup, TEXT("Tools"));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFontMenuPopup, TEXT("Font"));

    return hMenu;
}

void exitLogger() {
    PostQuitMessage(0);
    running = false;
    return;
}

void incFontSize() {
    if (fntSize > 50) return;
    hLogFont = CreateFontA(++fntSize, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Lucida Console");
    SendMessage(hLog, WM_SETFONT, (WPARAM)hLogFont, 0);
    UpdateWindow(hLog);
    RedrawWindow(hLog, 0, 0, RDW_INVALIDATE | RDW_ERASE);
}

void decFontSize() {
    if (fntSize < 7) return;
    hLogFont = CreateFontA(--fntSize, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Lucida Console");
    SendMessage(hLog, WM_SETFONT, (WPARAM)hLogFont, 0);
    UpdateWindow(hLog);
    RedrawWindow(hLog, 0, 0, RDW_INVALIDATE | RDW_ERASE);
}

void exportLog() {
    char filterFile[512];
    OpenFile(filterFile, true);
    if (filterFile != nullptr) {
        std::string line;
        std::ofstream myfile(filterFile);
        char* outLogBuffer = new char[GetWindowTextLengthA(hLog)];
        sequence.clear();
        if (myfile.is_open()) {
            GetWindowTextA(hLog, outLogBuffer, GetWindowTextLengthA(hLog));
            myfile << outLogBuffer;
            myfile.close();
        }
        delete[] outLogBuffer;
    }
}

void loadFilter() {
    char filterFile[512];
    OpenFile(filterFile, false, true);
    if (filterFile != nullptr) {
        std::string line;
        std::ifstream myfile(filterFile);
        filter.clear();
        if (myfile.is_open()) {
            while (std::getline(myfile, line)) {
                filter.push_back(line);
            }
            myfile.close();
        }
    }
}

void loadSequence() {
    char filterFile[512];
    OpenFile(filterFile, false);
    if (filterFile != nullptr) {
        std::string line;
        std::ifstream myfile(filterFile);
        sequence.clear();
        if (myfile.is_open()) {
            while (std::getline(myfile, line)) {
                sequence.push_back(line);
            }
            myfile.close();
        }
    }
}

void filterTheLog() {
    if (filter.size() < 1) {
        MessageBoxA(0, "Empty Filter!", "Filter File is empty or not correct", MB_OK);
        return;
    }
    FilterLog = IsDlgButtonChecked(hwnd, LOG_FILTER);
    if (FilterLog == BST_CHECKED) {
        CheckDlgButton(hwnd, LOG_FILTER, BST_UNCHECKED);
        filterLog = false;
    }
    else {
        CheckDlgButton(hwnd, LOG_FILTER, BST_CHECKED);
        filterLog = true;
    }
}

void sendButton() {
    GetWindowText(hCraftedPacket, craftedBuffer, 533);
    size_t len;

    //convert buffer to chars:
    wcstombs_s(&len, bufferToSend, 533, craftedBuffer, 533);

    //convert buffer to Bytes:
    size_t i;
    i = 0;
    for (size_t count = 0; count < len; ++i, count += 3) {

        if (bufferToSend[count] >= 'a') {
            bufferToSend[count] -= 0x20;
        }

        if (bufferToSend[count + 1] >= 'a') {
            bufferToSend[count + 1] -= 0x20;
        }

        if (bufferToSend[count] >= 'A') {
            bufferToSend[count] -= 'A';
            bufferToSend[count] += 10;
        }
        else {
            bufferToSend[count] -= 48;
        }

        if (bufferToSend[count + 1] >= 'A') {
            bufferToSend[count + 1] -= 'A';
            bufferToSend[count + 1] += 10;
        }
        else {
            bufferToSend[count + 1] -= 48;
        }

        bufferToSend[i] = (__int8)(((char)bufferToSend[count]) * (char)16);
        bufferToSend[i] += (__int8)bufferToSend[count + 1];
    }
    bufferToSend[i] = '\0';

    if (thisPTR != 0) {
        Send(thisPTR, bufferToSend, i);
    }
}

void logSend() {
    std::cout << "Log Button pressed!" << std::endl;
    LogSend = IsDlgButtonChecked(hwnd, LOG_SEND);
    if (LogSend) {
        CheckDlgButton(hwnd, LOG_SEND, BST_UNCHECKED);
        logSentHook = false;
    }
    else {
        CheckDlgButton(hwnd, LOG_SEND, BST_CHECKED);
        logSentHook = true;
    }
    // UpdateWindow(hwnd);
    // RedrawWindow(hLogSend, 0, 0, RDW_INVALIDATE | RDW_ERASE);
}

LRESULT CALLBACK MessageHandler(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam) {
    switch (uMessage) {
    case WM_CLOSE:
    case WM_DESTROY:
        exitLogger();
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case MYMENU_EXIT:
            exitLogger();
            break;
        case INC_FNT:
            incFontSize();
            break;
        case DEC_FNT:
            decFontSize();
            break;
        case EXPORT_LOG:
            exportLog();
            break;
        case LOAD_SEQ:
            loadSequence();
            break;
        case LOAD_FILTER:
            loadFilter();
            break;
        case LOG_FILTER:
            filterTheLog();
            break;
        case SEND_BUTTON:
            sendButton();
            break;
        case CLEAR_BUTTON:
            SetWindowTextA(hLog, "");
            break;
        case LOG_SEND:
            logSend();
            break;
        }
    }
    return DefWindowProc(hWindow, uMessage, wParam, lParam);
}

//Register our windows Class
BOOL RegisterDLLWindowClass(const wchar_t szClassName[]) {
    //HICON hIcon = static_cast<HICON>(::LoadImage(inj_hModule,MAKEINTRESOURCE(IDI_SHIELD),IMAGE_ICON,48, 48,LR_DEFAULTCOLOR));
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
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE); // CreateSolidBrush(RGB(128, 128, 128));
    if (!RegisterClassEx(&wc))
        return 0;
    return 1;
}

inline void printSendBufferToLog() {
    
#ifdef _DEBUG
    std::cout << "Sent Packet len: " << std::dec << sentLen << std::endl;
#endif

    int index = GetWindowTextLengthW(hLog);
    int limit = (int)SendMessageA(hLog, EM_GETLIMITTEXT, 0, 0);
    if (limit < index + (int)(sentLen * 3)) {
        SendMessageA(hLog, EM_SETSEL, 0, 1024);
        SendMessageA(hLog, EM_REPLACESEL, 0, (LPARAM)"");
    }

    SendMessageA(hLog, EM_SETSEL, (WPARAM)index, (LPARAM)index); // set selection - end of text
    std::string buffer = "[SEND]";
    int bufLen = buffer.size();
    
    for (DWORD i = 0; i < sentLen; ++i) {
        buffer += (hex_chars[((sentBuffer)[i] & 0xF0) >> 4]);
        buffer += (hex_chars[((sentBuffer)[i] & 0x0F) >> 0]);
        buffer += ' ';
    }
    buffer += "\r\n";
    std::cout << buffer << std::endl;

    if (filterLog) {
        for (size_t i = 0; i < filter.size(); ++i) {
            if (buffer.compare(bufLen, filter.at(i).size(), filter.at(i)) == 0) {
                return;
            }
        }
    }
    SendMessageA(hLog, EM_REPLACESEL, 0, (LPARAM)buffer.c_str());
}

void hotKeys() {
    while (running) {
        if (GetAsyncKeyState(VK_F9) & 1) {
            SetWindowTextA(hLog, "");
        }
        if (GetAsyncKeyState(VK_F5) & 1) {
            logSend();
        }
        if (GetAsyncKeyState(VK_F1) & 1) {
            filterTheLog();
        }
        if (GetAsyncKeyState(VK_ADD) & 1) {
            incFontSize();
        }
        if (GetAsyncKeyState(VK_SUBTRACT) & 1) {
            decFontSize();
        }       
    }
}

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
    hLogFont = CreateFontA(fntSize,0,0,0,0,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE | DEFAULT_PITCH,"Lucida Console");

    RegisterDLLWindowClass(L"InjectedDLLWindowClass");
    hwnd = CreateWindowEx(0, L"InjectedDLLWindowClass", L"Erarnitox's Tera Proxy | GuidedHacking.com", WS_EX_LAYERED /*| WS_EX_APPWINDOW*/, CW_USEDEFAULT, CW_USEDEFAULT, 1020, 885, NULL, hMenu, inj_hModule, NULL);
    hLog = CreateWindowEx(0, L"edit", L"", WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_NOHIDESEL, 5, 5, 1005, 700, hwnd, NULL, hModule, NULL);
    SendMessage(hLog,WM_SETFONT,(WPARAM)hLogFont,0);

    hClearButton = CreateWindowEx(0, L"button", L"Clear Log (F9)", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 710, 100, 30, hwnd, (HMENU)CLEAR_BUTTON, hModule, NULL);
    hSendButton = CreateWindowEx(0, L"button", L"Send (F8)", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 800, 100, 30, hwnd, (HMENU)SEND_BUTTON, hModule, NULL);
    hCraftedPacket = CreateWindowEx(0, L"edit", L"<Packet Data>", WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_BORDER, 110, 730, 900, 100, hwnd, NULL, hModule, NULL);
    SendMessage(hCraftedPacket, WM_SETFONT, (WPARAM)hLogFont, 0);

    hLogSend = CreateWindowEx(0, L"button", L"Log Send (F5)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 110, 705, 120, 25, hwnd, (HMENU)LOG_SEND, hModule, NULL);
    hLogRecv = CreateWindowEx(0, L"button", L"Log Recv (F6)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_DISABLED, 250, 705, 120, 25, hwnd, NULL, hModule, NULL);
    hFilterLog = CreateWindowEx(0, L"button", L"Filter Log (F1)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 850, 705, 120, 25, hwnd, (HMENU)LOG_FILTER, hModule, NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    //UpdateWindow(hwnd); // redraw window;

    std::thread hotKeyThread = std::thread(hotKeys);

    while (running && GetMessage(&messages, NULL, 0, 0)){
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    //exit:
    running = false;
    hotKeyThread.join();
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

