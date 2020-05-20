#pragma once
//Tanks to rake for this code here and Nomade for making it more stable

char* ScanBasic(const char* pattern, const char* mask, char* begin, size_t size){
    size_t patternLen = strlen(mask);

    for (size_t i = 0; i < size; i++){
        bool found = true;
        for (size_t j = 0; j < patternLen; j++){
            if (mask[j] != '?' && pattern[j] != *(char*)((intptr_t)begin + i + j)){
                found = false;
                break;
            }
        }
        if (found){
            return (begin + i);
        }
    }
    return nullptr;
}


char* ScanInternal(const char* pattern, const char* mask, char* begin, size_t size){
    char* match{ nullptr };
    MEMORY_BASIC_INFORMATION mbi{};

    for (char* curr = begin; curr < begin + size; curr += mbi.RegionSize){
        if (!VirtualQuery(curr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) continue;
        match = ScanBasic(pattern, mask, curr, mbi.RegionSize);

        if (match != nullptr && match != pattern){
            break;
        }
    }
    return match;
}