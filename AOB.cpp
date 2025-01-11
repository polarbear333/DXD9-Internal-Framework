#include <iostream>
#include <vector>
#include <string>

intptr_t FindPattern(std::vector<unsigned char> data, intptr_t baseAddress, const unsigned char* lpPattern, const char* pszMask, intptr_t offset, intptr_t resultUsage) {
    std::vector<std::pair<unsigned char, bool>> pattern;
    for (size_t x = 0, y = strlen(pszMask); x < y; x++) {
        pattern.push_back(std::make_pair(lpPattern[x], pszMask[x] == 'x'));
    }

    auto scanStart = data.begin();
    auto resultCnt = 0;

    while (true) {
        // Search for the pattern..
        auto ret = std::search(scanStart, data.end(), pattern.begin(), pattern.end(),
            [&](unsigned char curr, std::pair<unsigned char, bool> currPattern)
            {
                return (!currPattern.second) || curr == currPattern.first;
            });

        if (ret != data.end()) {
            if (resultCnt == resultUsage || resultUsage == 0) {
                return (std::distance(data.begin(), ret) + baseAddress) + offset;
            }
            ++resultCnt;
            scanStart = ++ret;
        }
        else {
            break;
        }
    }

    return 0;
}

bool NopGrenadeDec(HANDLE hProcess, intptr_t GrenadeAddress) {
    unsigned char nops[] = { 0x90, 0x90, 0x90, 0x90, 0x90 }; 

    SIZE_T bytesWritten;
    if (!WriteProcessMemory(hProcess, (LPVOID)GrenadeAddress, nops, sizeof(nops), &bytesWritten)) {
        std::cerr << "Failed to write NOPs to GrenadeAddress." << std::endl;
        return false;
    }

    std::cout << "Grenade decrement successfully NOPed." << std::endl;
    return true;
}

bool NopInjectCode(HANDLE hProcess, intptr_t InjectAddress) {
    // Code to write NOPs to the InjectAddress
    unsigned char nops[] = { 0x90, 0x90, 0x90, 0x90, 0x90 }; // 5 NOP instructions

    SIZE_T bytesWritten;
    if (!WriteProcessMemory(hProcess, (LPVOID)InjectAddress, nops, sizeof(nops), &bytesWritten)) {
        std::cerr << "Failed to write NOPs to InjectAddress." << std::endl;
        return false;
    }

    std::cout << "InjectCode successfully NOPed." << std::endl;
    return true;
}

int main() {
    DWORD processID = GetCurrentProcessId(); 
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);

    if (hProcess == NULL) {
        std::cerr << "Failed to open process." << std::endl;
        return 1;
    }

    // Get module base address
    MODULEINFO modInfo = { 0 };
    GetModuleInformation(hProcess, GetModuleHandle(" Replace with the executable"), &modInfo, sizeof(modInfo));
    intptr_t moduleBase = (intptr_t)modInfo.lpBaseOfDll;

    // AOB scan patterns
    const unsigned char GrenadePattern[] = { 0xFE, 0x8C, 0x38, 0x1E, 0x03, 0x00, 0x00 };
    const char GrenadeMask[] = "xxxxxxxx";

    const unsigned char InjectPattern[] = { 0xC7, 0x86, 0xE4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF6 };
    const char InjectMask[] = "xxxxxxxxxx";

    // Find the Grenade pattern
    intptr_t GrenadeAddress = FindPattern(
        std::vector<unsigned char>((char*)moduleBase, (char*)moduleBase + modInfo.SizeOfImage),
        moduleBase,
        GrenadePattern,
        GrenadeMask,
        0,
        0
    );

    if (GrenadeAddress == 0) {
        std::cerr << "Failed to find Grenade pattern." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    // Find the Inject pattern
    intptr_t InjectAddress = FindPattern(
        std::vector<unsigned char>((char*)moduleBase, (char*)moduleBase + modInfo.SizeOfImage),
        moduleBase,
        InjectPattern,
        InjectMask,
        0,
        0
    );

    if (InjectAddress == 0) {
        std::cerr << "Failed to find Inject pattern." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    // Call NopGrenadeDec to NOP the Grenade decrement
    if (NopGrenadeDec(hProcess, GrenadeAddress)) {
        // Successfully NOPed Grenade decrement
    }
    else {
        // Handle error
    }

    // Call NopInjectCode to NOP the InjectCode
    if (NopInjectCode(hProcess, InjectAddress)) {
        // Successfully NOPed InjectCode
    }
    else {
        // Handle error
    }

    // Close the process handle
    CloseHandle(hProcess);

    return 0;
}