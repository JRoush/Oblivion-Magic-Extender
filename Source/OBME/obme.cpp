/*
    Main unit for OBME submodules.  Handles:
     -   Initialization (writing hooks & patches)
    -   global debugging log (linked to log in loader)
    -   global OBME interface
    -   global module handle
*/
#include "OBME/OBME_interface.h"

// includes for initialization
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/MagicGroup.h"
#include "OBME/EffectItem.h"
#include "OBME/EffectItemList.h"
#include "OBME/MagicItemForm.h"

/*--------------------------------------------------------------------------------------------*/
// global debugging log for the submodule
_declspec(dllimport) OutputLog _gLog;
OutputLog& gLog = _gLog;

/*--------------------------------------------------------------------------------------------*/
// global obme interface
OBME_Interface      g_obmeIntfc;
// module (or "instance") handle
HMODULE hModule = 0;

/*--------------------------------------------------------------------------------------------*/
// submodule initialization
extern "C" _declspec(dllexport) void* Initialize()
{   
    // begin initialization  
    _MESSAGE("Initializing Submodule {%p} ...", hModule); 

    OBME::MagicGroup::Initialize();
    OBME::EffectHandler::Initialize();
    OBME::EffectSetting::Initialize();
    OBME::EffectItem::InitHooks();
    OBME::EffectItemList::InitHooks();
    OBME::SpellItem::InitHooks();
    
    // initialization complete
    _DMESSAGE("Submodule initialization completed sucessfully");
    return &g_obmeIntfc;
}

/*--------------------------------------------------------------------------------------------*/
// submodule loading
// Project uses standard windows libraries, define an entry point for the DLL to handle loading/unloading
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:    // dll loaded
        hModule = (HMODULE)hDllHandle;  // store module handle
        _MESSAGE("Attaching Submodule ..."); 
        break;
    case DLL_PROCESS_DETACH:    // dll unloaded
        _MESSAGE("Detaching Submodule ...");      
        break;
    }   
    return true;
}