#pragma once
class Hook {
    void* tToHook;
    char* oldOpcodes;
    int tLen;
public:
    Hook(void* toHook, void* ourFunct, int len) : tToHook(toHook), oldOpcodes(nullptr), tLen(len){
        if (len < 5) {
            return;
        }

        DWORD curProtection;
        VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &curProtection);

        oldOpcodes = (char*)malloc(len);
        if (oldOpcodes != nullptr) {
            for (int i = 0; i < len; ++i) {
                oldOpcodes[i] = ((char*)toHook)[i];
            }
        }

        memset(toHook, 0x90, len);

        DWORD relativeAddress = ((DWORD)ourFunct - (DWORD)toHook) - 5;

        *(BYTE*)toHook = 0xE9;
        *(DWORD*)((DWORD)toHook + 1) = relativeAddress;

        VirtualProtect(toHook, len, curProtection, &curProtection);
    }

    ~Hook() {
        if (oldOpcodes != nullptr) {
            DWORD curProtection;
            VirtualProtect(tToHook, tLen, PAGE_EXECUTE_READWRITE, &curProtection);
            for (int i = 0; i < tLen; ++i) {
                ((char*)tToHook)[i] = oldOpcodes[i];
            }
            VirtualProtect(tToHook, tLen, curProtection, &curProtection);
            free(oldOpcodes);
        }
    }
};
