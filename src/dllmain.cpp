#include "dllmain.h"

#include "Game.h"
#include "Hook.h"
#include "Scan.h"

HMODULE inj_hModule;
HWND hCraftedPacket;
HWND hLog;

BOOL LogSend{ FALSE };
BOOL LogRecv{ FALSE };
BOOL FilterLog{ FALSE };

bool filterLog{ false };
bool running{ true };

HWND hwnd;
HWND hLogSend;
HWND hLogRecv;
HWND hFilterLog;
HWND hClearButton;
HWND hSendButton;
HFONT hLogFont;

char const hex_chars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

uintptr_t moduleBase;
size_t fntSize{ 14 };

std::vector<std::string> filter;
std::vector<std::string> sequence;

std::thread sendsequenceLoopThread;
bool sendSequenceLoopRunning{ false };

void OpenFile(char* filename, bool save, bool filter) {
    filename[0] = 0;
    OPENFILENAMEA saveDialog{};
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
    HMENU hMenu = CreateMenu();
   
    if (hMenu == NULL)
        return FALSE;
    HMENU hFileMenuPopup = CreatePopupMenu();
    HMENU hToolMenuPopup = CreatePopupMenu();
    HMENU hFontMenuPopup = CreatePopupMenu();

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

inline void exitLogger() {
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
        std::ofstream myfile(filterFile);
        int len{ GetWindowTextLengthA(hLog) };
        auto outLogBuffer = std::make_unique<char[]>(len);
        sequence.clear();
        if (myfile.is_open()) {
            GetWindowTextA(hLog, outLogBuffer.get(), GetWindowTextLengthA(hLog));
            myfile << outLogBuffer;
            myfile.close();
        }
    }
}

void startSequenceLoop() {
    if (sendSequenceLoopRunning) return;
    sendSequenceLoopRunning = true;
    sendsequenceLoopThread = std::thread(sequenceLoop);
}

void stopSequenceLoop() {
    if (!sendSequenceLoopRunning) return;
    sendSequenceLoopRunning = false;
    sendsequenceLoopThread.join();
}

void sequenceLoop() {
    while (sendSequenceLoopRunning) {
        sendSequence();
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

void sendSequence() {
    std::unique_ptr<char[]> buffer;
    size_t len;
    for (std::string data : sequence) { 
        len = data.size();
        buffer = std::make_unique<char[]>(len +1);
        std::memcpy(buffer.get(), data.c_str(), len);
        buffer[len] = '\0';
        gameSendCaller(buffer.get(), ++len);
        Sleep(50);
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
    size_t len = GetWindowTextLengthW(hCraftedPacket);
    ++len;
    auto craftedBuffer = std::make_unique<wchar_t[]>(len);
    auto bufferToSend = std::make_unique<char[]>(len);
    GetWindowTextW(hCraftedPacket, craftedBuffer.get(), len);
   
    //convert buffer to chars:
    wcstombs_s(&len, bufferToSend.get(), len, craftedBuffer.get(), len);

    gameSendCaller(bufferToSend.get(), len);
}

void gameSendCaller(char* data, size_t size) {
    //convert buffer to Bytes:
    size_t i;
    i = 0;
    for (size_t count = 0; count < size; ++i, count += 3) {

        if (data[count] > 'f' || data[count + 1] > 'f') return;

        if (data[count] >= 'a') {
            data[count] -= 0x20;
        }

        if (data[count + 1] >= 'a') {
            data[count + 1] -= 0x20;
        }

        if (data[count] >= 'A') {
            data[count] -= 'A';
            data[count] += 10;
        }
        else {
            data[count] -= 48;
        }

        if (data[count + 1] >= 'A') {
            data[count + 1] -= 'A';
            data[count + 1] += 10;
        }
        else {
            data[count + 1] -= 48;
        }

        data[i] = (__int8)(((char)data[count]) * (char)16);
        data[i] += (__int8)data[count + 1];
    }
    data[i] = '\0';

    if (game::thisPTR != 0) {
        game::Send(game::thisPTR, data, i);
    }
}

void logSend() {
#if USE_SEND_HOOK > 0
    LogSend = IsDlgButtonChecked(hwnd, LOG_SEND);
    if (LogSend) {
        CheckDlgButton(hwnd, LOG_SEND, BST_UNCHECKED);
        game::logSentHook = false;
    }
    else {
        CheckDlgButton(hwnd, LOG_SEND, BST_CHECKED);
        game::logSentHook = true;
    }
#endif
}

void logRecv() {
#if USE_RECV_HOOK > 0
    LogRecv = IsDlgButtonChecked(hwnd, LOG_RECV);
    if (LogRecv) {
        CheckDlgButton(hwnd, LOG_RECV, BST_UNCHECKED);
        game::logRecvHook = false;
    }
    else {
        CheckDlgButton(hwnd, LOG_RECV, BST_CHECKED);
        game::logRecvHook = true;
    }
#endif
}

inline void resize(LPARAM lParam) {
    WORD nWidth = (LOWORD(lParam))>300? LOWORD(lParam) : 300;
    WORD nHeight = (HIWORD(lParam)) > 400 ? HIWORD(lParam) : 400;
    MoveWindow(hLog, 10, 10, (nWidth - 20), (nHeight - 145), false);

    MoveWindow(hClearButton, 5, nHeight-130, 100, 30, false);
    MoveWindow(hSendButton, 5, nHeight-45, 100, 30, false);
    MoveWindow(hCraftedPacket, 110, nHeight-110, (nWidth - 120), 100, false);
    MoveWindow(hLogSend, 110, nHeight-135, 120, 25, false);
    MoveWindow(hLogRecv, 250, nHeight - 135, 120, 25, false);
    MoveWindow(hFilterLog, nWidth-170, nHeight - 135, 120, 25, false);
}

LRESULT CALLBACK MessageHandler(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam) {
    switch (uMessage) {
    case WM_CLOSE:
    case WM_DESTROY:
        exitLogger();
        break;
    case WM_SIZE:
        resize(lParam);
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
        case SEND_SEQ:
            sendSequence();
            break;
        case CLEAR_BUTTON:
            SetWindowTextA(hLog, "");
            break;
        case LOG_SEND:
            logSend();
            break;
        case LOG_RECV:
            logRecv();
            break;
        case SEND_LOOP:
            startSequenceLoop();
            break;
        case STOP_LOOP:
            stopSequenceLoop();
            break;
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
    wc.hIcon = LoadIconW(NULL, IDI_SHIELD);
    wc.hIconSm = LoadIconW(NULL, IDI_SHIELD);
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
    std::cout << "Sent Packet len: " << std::dec << game::sentLen << std::endl;
#endif

    int index{ GetWindowTextLengthW(hLog) };
    int limit{ static_cast<int>(SendMessageA(hLog, EM_GETLIMITTEXT, 0, 0)) };
    if (limit < index + static_cast<int>((game::sentLen * 3))) {
        SendMessageA(hLog, EM_SETSEL, 0, 1024);
        SendMessageA(hLog, EM_REPLACESEL, 0, (LPARAM)"");
    }

    SendMessageA(hLog, EM_SETSEL, static_cast<WPARAM>(index), static_cast<LPARAM>(index)); // set selection - end of text
    std::string buffer{"[SEND]"};
    size_t bufLen{ buffer.size() };
    
    for (DWORD i{ 0 }; i < game::sentLen; ++i) {
        buffer += (hex_chars[((game::sentBuffer)[i] & 0xF0) >> 4]);
        buffer += (hex_chars[((game::sentBuffer)[i] & 0x0F) >> 0]);
        buffer += ' ';
    }
    buffer += "\r\n";
  
    if (filterLog) {
        for (size_t i{ 0 }; i < filter.size(); ++i) {
            if (buffer.compare(bufLen, filter.at(i).size(), filter.at(i)) == 0) {
                return;
            }
        }
    }
    SendMessageA(hLog, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(buffer.c_str()));
}

inline void printRecvBufferToLog() {

#ifdef _DEBUG
    std::cout << "Sent Packet len: " << std::dec << game::recvLen << std::endl;
#endif

    int index{ GetWindowTextLengthW(hLog) };
    int limit{ static_cast<int>(SendMessageA(hLog, EM_GETLIMITTEXT, 0, 0)) };
    if (limit < index + static_cast<int>((game::recvLen * 3))) {
        SendMessageA(hLog, EM_SETSEL, 0, 1024);
        SendMessageA(hLog, EM_REPLACESEL, 0, (LPARAM)"");
    }

    SendMessageA(hLog, EM_SETSEL, static_cast<WPARAM>(index), static_cast<LPARAM>(index)); // set selection - end of text
    std::string buffer{ "[Recv]" };
    size_t bufLen{ buffer.size() };

    for (DWORD i{ 0 }; i < game::recvLen; ++i) {
        buffer += (hex_chars[((game::recvBuffer)[i] & 0xF0) >> 4]);
        buffer += (hex_chars[((game::recvBuffer)[i] & 0x0F) >> 0]);
        buffer += ' ';
    }
    buffer += "\r\n";

    if (filterLog) {
        for (size_t i{ 0 }; i < filter.size(); ++i) {
            if (buffer.compare(bufLen, filter.at(i).size(), filter.at(i)) == 0) {
                return;
            }
        }
    }
    SendMessageA(hLog, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(buffer.c_str()));
}

void hotKeys() {
    while (running) {
        if (GetAsyncKeyState(VK_F9) & 1) {
            SetWindowTextA(hLog, "");
        }
        if (GetAsyncKeyState(VK_F5) & 1) {
            logSend();
        }
        if (GetAsyncKeyState(VK_F8) & 1) {
            sendButton();
        }
        if (GetAsyncKeyState(VK_F6) & 1) {
            logRecv();
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

    moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandle(game::moduleName));
#if USE_SEND_HOOK > 0 | USE_SEND_BUTTON > 0
    game::Send = reinterpret_cast<game::InternalSend>(Pattern::ScanInternal(game::internalSendPattern, game::internalSendMask, reinterpret_cast<char*>(moduleBase), 0xF000000));
#endif
#if USE_RECV_HOOK > 0
#if USE_RECV_PATTERN > 0
    game::toHookRecv += reinterpret_cast<size_t>(Pattern::ScanInternal(game::internalRecvPattern, game::internalRecvMask, reinterpret_cast<char*>(moduleBase), 0xF000000));
#endif
#endif

#ifdef _DEBUG
    std::cout << "send function location:" << std::hex << (int)game::Send << std::endl;
#endif // _DEBUG

#if USE_SEND_HOOK > 0
#if USE_SEND_PATTERN > 0
    game::toHookSend += reinterpret_cast<size_t>(game::Send);
#endif
    game::jmpBackAddrSend = game::toHookSend + game::sendHookLen;
#endif
#if USE_RECV_HOOK > 0
    game::jmpBackAddrRecv = game::toHookRecv + game::recvHookLen;
#endif

    {//begin Hook block
#if USE_SEND_HOOK > 0
        Hook sendHook{ reinterpret_cast<void*>(game::toHookSend), reinterpret_cast<void*>(game::sendHookFunc), game::sendHookLen };
#endif
#if USE_RECV_HOOK > 0
        Hook recvHook{ reinterpret_cast<void*>(game::toHookRecv), reinterpret_cast<void*>(game::recvHookFunc), game::recvHookLen };
#endif

        HMENU hMenu{ CreateDLLWindowMenu() };
        hLogFont = CreateFontA(fntSize, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Lucida Console");

        RegisterDLLWindowClass(L"InjectedDLLWindowClass");
        hwnd = CreateWindowEx(0, L"InjectedDLLWindowClass", game::title, WS_EX_LAYERED | WS_EX_APPWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 885, NULL, hMenu, inj_hModule, NULL);
        hLog = CreateWindowEx(0, L"edit", L"", WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_NOHIDESEL, 5, 5, 805, 700, hwnd, NULL, hModule, NULL);
        SendMessage(hLog, WM_SETFONT, (WPARAM)hLogFont, 0);

        hClearButton = CreateWindowEx(0, L"button", L"Clear Log (F9)", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 710, 100, 30, hwnd, (HMENU)CLEAR_BUTTON, hModule, NULL);
#if USE_SEND_BUTTON > 0
        hSendButton = CreateWindowEx(0, L"button", L"Send (F8)", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 5, 800, 100, 30, hwnd, (HMENU)SEND_BUTTON, hModule, NULL);
#else
        HWND hSendButton{ CreateWindowEx(0, L"button", L"Send (F8)", WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_BORDER | BS_DEFPUSHBUTTON, 5, 800, 100, 30, hwnd, (HMENU)NULL, hModule, NULL) };
#endif
        hCraftedPacket = CreateWindowEx(0, L"edit", L"<Packet Data>", WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_BORDER, 110, 730, 700, 100, hwnd, NULL, hModule, NULL);
        SendMessage(hCraftedPacket, WM_SETFONT, (WPARAM)hLogFont, 0);

#if USE_SEND_HOOK > 0
        hLogSend = CreateWindowEx(0, L"button", L"Log Send (F5)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 110, 705, 120, 25, hwnd, (HMENU)LOG_SEND, hModule, NULL);
#else
        hLogSend = CreateWindowEx(0, L"button", L"Log Send (F5)", WS_CHILD | WS_VISIBLE | WS_DISABLED| BS_CHECKBOX, 110, 705, 120, 25, hwnd, (HMENU)NULL, hModule, NULL);
#endif
#if USE_RECV_HOOK > 0
        hLogRecv = CreateWindowEx(0, L"button", L"Log Recv (F6)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 250, 705, 120, 25, hwnd, (HMENU)LOG_RECV, hModule, NULL);
#else 
        hLogRecv = CreateWindowEx(0, L"button", L"Log Recv (F6)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_DISABLED, 250, 705, 120, 25, hwnd, NULL, hModule, NULL);
#endif
        hFilterLog = CreateWindowEx(0, L"button", L"Filter Log (F1)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 650, 705, 120, 25, hwnd, (HMENU)LOG_FILTER, hModule, NULL);

#if USE_CUSTOM_ICON > 0
        //Set custom icon:
        HICON hicon = (HICON)LoadImageW(inj_hModule,MAKEINTRESOURCEW(IDI_ICON1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
#endif

        ShowWindow(hwnd, SW_SHOWNORMAL);
        std::thread hotKeyThread{ hotKeys };

        MSG messages;
        while (running && GetMessage(&messages, NULL, 0, 0)) {
            TranslateMessage(&messages);
            DispatchMessage(&messages);
        }

        //exit:
        if (sendSequenceLoopRunning) {
            stopSequenceLoop();
        }
        running = false;
        hotKeyThread.join();
    }//end Hook block
    

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

//Entrypoint:
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

