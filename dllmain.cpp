#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <psapi.h>
#include <cstdint>
#include <cstring>

#include "EntityEx.h"
#include "CameraExtended.h"
#include "D3D9Helper.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "psapi.lib")

#include "detours.h"
#pragma comment(lib, "detours.lib")

// Handle for the injected DLL.
HINSTANCE DllInstance;

typedef HRESULT(__stdcall* EndSceneFunction)(IDirect3DDevice9* pDevice);
EndSceneFunction OriginalEndScene;


D3D9Helper Direct3DHelper;
LPD3DXFONT RenderFont;

// Menu parameters.
float menuXCoordinate = 35, menuWidth = 220, menuYCoordinate = 100, menuHeight = 70;
float elementPadding = 10;
float textLineHeight = 20;
std::vector<std::string> menuTextLines = { "Press Numpad0 for ESP", "Press Numpad1 to Exit" };


bool isESPActive = false;
std::vector<EntityEx> gameEntities;
CameraExtended viewCamera = CameraExtended();

// Retrieves information about a specific module.
MODULEINFO GetModuleInformation(const wchar_t* moduleName) {
    MODULEINFO modInfo = { 0 };
    HMODULE hModule = GetModuleHandleW(moduleName);
    if (hModule != NULL) {
        GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo));
    }
    return modInfo;
}

// Finds the base address of the object table in memory.
uintptr_t FindObjectTableBaseAddress() {
    HMODULE keystoneModule = GetModuleHandleW(L"keystone.dll");
    if (!keystoneModule) {
        std::cerr << "Error: Could not find keystone.dll" << std::endl;
        return 0;
    }
    std::cout << "Found keystone.dll at: " << std::hex << (uintptr_t)keystoneModule << std::endl;
    return reinterpret_cast<uintptr_t>(keystoneModule);
}

// Loads entity information from memory.
std::vector<EntityEx> LoadGameEntities() {
    std::vector<EntityEx> entities;

    // Get the base address of keystone.dll.
    uintptr_t objectTableBase = FindObjectTableBaseAddress();
    if (!objectTableBase) {
        return entities;
    }

    // Read the initial pointer from keystone.dll (this is an absolute address).
    uintptr_t currentPtr = *(uintptr_t*)(objectTableBase + 0x00126054);

    // Get information about the game executable.
    MODULEINFO gameExecutableInfo = GetModuleInformation(L"GameExecutable.exe");
    if (gameExecutableInfo.lpBaseOfDll == NULL) {
        return entities;
    }
    uintptr_t gameExecutableBase = (uintptr_t)gameExecutableInfo.lpBaseOfDll;
    uintptr_t gameExecutableEnd = gameExecutableBase + gameExecutableInfo.SizeOfImage;

    uintptr_t addressOffsets[] = { 0x64, 0x58, 0x8A8, 0x694, 0x8, 0x0 };
    for (uintptr_t i = 0; i < sizeof(addressOffsets) / sizeof(addressOffsets[0]); ++i) {
        uintptr_t currentOffset = addressOffsets[i];

        // Add the offset to the current address.
        currentPtr += currentOffset;

        // Check for invalid memory read.
        if (IsBadReadPtr((void*)currentPtr, sizeof(uintptr_t))) {
            return entities;
        }

        // Dereference the current address to get the next pointer in the chain.
        if (i < sizeof(addressOffsets) / sizeof(addressOffsets[0]) - 1) {
            uintptr_t possiblePtr = *(uintptr_t*)currentPtr;

            // Check if the value points within the game executable and adjust if necessary.
            if ((possiblePtr >= gameExecutableBase && possiblePtr < gameExecutableEnd) || possiblePtr > 0x10000) {
                currentPtr = possiblePtr;
            }
            else {
                return entities;
            }
        }
    }

    // The final currentPtr is the entity table pointer.
    uintptr_t entityTableBase = (uintptr_t)currentPtr;

    // Check for invalid memory read.
    if (!entityTableBase || IsBadReadPtr((void*)entityTableBase, sizeof(uintptr_t))) {
        return entities;
    }

    // Maximum number of entities to process.
    const int entityLimit = 1024;
    for (int i = 0; i < entityLimit; ++i) {
        uintptr_t entityAddress = *(uintptr_t*)(entityTableBase + i * sizeof(uintptr_t));
        // Check for invalid memory read or null pointer.
        if (!entityAddress || IsBadReadPtr((void*)entityAddress, sizeof(Entity))) {
            continue;
        }
        // Add the entity to the list.
        entities.push_back(*reinterpret_cast<EntityEx*>(entityAddress));
    }

    return entities;
}

// Hooked EndScene function.
HRESULT __stdcall HookedEndScene(IDirect3DDevice9* pDevice) {
    Direct3DHelper.pDevice = pDevice;

    // Draw the menu background.
    Direct3DHelper.drawFilledRectangle(menuXCoordinate, menuYCoordinate, menuWidth, menuHeight, D3DCOLOR_ARGB(120, 54, 162, 255));
    // Draw each menu line.
    for (size_t i = 0; i < menuTextLines.size(); ++i) {
        Direct3DHelper.drawText(menuTextLines.at(i), menuXCoordinate + elementPadding, menuYCoordinate + elementPadding + i * textLineHeight, D3DCOLOR_ARGB(255, 153, 255, 153));
    }

    // ESP rendering.
    if (isESPActive) {
        gameEntities = LoadGameEntities();
        for (EntityEx entityEx : gameEntities) {
            if (entityEx.entity) {
                // Convert world coordinates to screen coordinates.
                Vector3 screenCoords = viewCamera.WorldToScreen(entityEx.entity->feet);
                if (screenCoords.z < 100) {
                    // Calculate entity height on screen.
                    Vector3 torsoScreenCoords = viewCamera.WorldToScreen(entityEx.entity->torso);
                    float onScreenHeight = abs(screenCoords.y - torsoScreenCoords.y) * 2;

                    // Display entity information.
                    std::string displayText = "health: " + std::to_string((int)(entityEx.entity->health * 100));
                    Direct3DHelper.drawText(displayText, torsoScreenCoords.x - 20, torsoScreenCoords.y - onScreenHeight / 1.2, D3DCOLOR_ARGB(255, 153, 255, 153));

                    // Determine box color based on entity type.
                    D3DCOLOR boxColor = D3DCOLOR_ARGB(255, 255, 0, 0); // Default to red.
                    if (entityEx.typeID == 3680)
                        boxColor = D3DCOLOR_ARGB(255, 0, 255, 0); // Green for type 3680.

                    // Calculate entity width on screen and draw the bounding box.
                    float onScreenWidth = onScreenHeight / 2;
                    Direct3DHelper.drawRectangle(screenCoords.x - onScreenWidth / 2, torsoScreenCoords.y - onScreenHeight / 2, onScreenWidth, onScreenHeight, boxColor);
                }
            }
        }
    }

    // Call the original EndScene function.
    return OriginalEndScene(pDevice);
}

// Sets up the function hook.
void SetupHook() {
    OriginalEndScene = (EndSceneFunction)DetourFunction((PBYTE)Direct3DHelper.vTable[42], (PBYTE)HookedEndScene);
}

// Removes the function hook.
void RemoveHook() {
    DetourRemove((PBYTE)OriginalEndScene, (PBYTE)HookedEndScene);
}

// Thread procedure for DLL cleanup.
DWORD __stdcall CleanupThread(LPVOID lpParameter) {
    Sleep(100);
    FreeLibraryAndExitThread(DllInstance, 0);
    return 0;
}

// Main menu logic.
DWORD WINAPI MainMenu(HINSTANCE hModule) {
    // Allocate a console for debugging output.
    AllocConsole();
    FILE* consoleStream;
    freopen_s(&consoleStream, "CONOUT$", "w", stdout);

    // Initialize the D3D9 helper.
    if (!Direct3DHelper.initVTable()) {
        std::cout << "Could not initialize D3D9 helper. Exiting." << std::endl;
        Sleep(1000);
        fclose(consoleStream);
        FreeConsole();
        CreateThread(0, 0, CleanupThread, 0, 0, 0);
        return 0;
    }

    // Set up the function hook.
    SetupHook();

    // Main loop.
    while (true) {
        Sleep(50);
        // Toggle ESP.
        if (GetAsyncKeyState(VK_NUMPAD0)) {
            isESPActive = !isESPActive;
            Sleep(1000);
        }
        // Exit.
        if (GetAsyncKeyState(VK_NUMPAD1)) {
            RemoveHook();
            Direct3DHelper.cleanup();
            break;
        }
    }

    // Exit message.
    std::cout << "Exiting." << std::endl;
    Sleep(1000);
    fclose(consoleStream);
    FreeConsole();
    CreateThread(0, 0, CleanupThread, 0, 0, 0);
    return 0;
}

// DLL entry point.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DllInstance = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainMenu, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}