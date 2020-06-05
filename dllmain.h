#pragma once
//Dependencies:
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <commdlg.h>
#include <fstream>
#include <thread>
#include "resource.h"

//Defines:
#define MYMENU_EXIT (WM_APP + 100)
#define SEND_BUTTON (WM_APP + 101)
#define LOG_SEND (WM_APP + 102)
#define LOG_RECV (WM_APP + 103)
#define CLEAR_BUTTON (WM_APP + 104)
#define LOG_FILTER (WM_APP + 105)

#define SEND_SEQ (WM_APP + 106)
#define SEND_LOOP (WM_APP + 107)
#define STOP_LOOP (WM_APP + 108)
#define LOAD_SEQ (WM_APP + 109)
#define EXP_SEQ (WM_APP + 110)

#define INC_FNT (WM_APP + 111)
#define DEC_FNT (WM_APP + 112)
#define LOAD_FILTER (WM_APP + 113)
#define EXPORT_LOG (WM_APP + 114)


//Prototypes:
DWORD WINAPI WindowThread(HMODULE hModule);
void OpenFile(char* filename, bool save, bool filter = false);
HMENU CreateDLLWindowMenu();
BOOL RegisterDLLWindowClass(const wchar_t szClassName[]);
LRESULT CALLBACK MessageHandler(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL RegisterDLLWindowClass(const wchar_t szClassName[]);
DWORD WINAPI WindowThread(HMODULE hModule);
void hotKeys();
void exitLogger();
void incFontSize();
void decFontSize();
void exportLog();
void loadFilter();
void loadSequence();
void sendButton();
void logSend();
void filterTheLog();
void gameSendCaller(char* data, size_t size);
void sendSequence();
void stopSequenceLoop();
void startSequenceLoop();
void sequenceLoop();
void printSendBufferToLog();
void printRecvBufferToLog();

namespace game {
	bool logSentHook{ false };
	bool logRecvHook{ false };
	void* thisPTR;
	DWORD sentLen;
	char* sentBuffer;
	DWORD recvLen;
	char* recvBuffer;
	char* tmpBuffer;
	DWORD jmpBackAddrSend;
	DWORD jmpBackAddrRecv;

	//additional "resgisters" to use for tmp values:
	void* teax;
	void* tebx;
	void* tecx;
	void* tedx;
	void* tesi;
	void* tedi;
	void* tebp;
	void* tesp;

	void* reax;
	void* rebx;
	void* recx;
	void* redx;
	void* resi;
	void* redi;
	void* rebp;
	void* resp;
}