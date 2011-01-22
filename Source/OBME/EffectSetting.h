/* 
    OBME expansion of the EffectSetting class
*/
#pragma once

// base classes
#include "API/Magic/EffectSetting.h"

namespace OBME {

// argument classes from OBME
class   MgefHandler;      // OBME/EffectHandlers/EffectHandler.h

class EffectSetting : public ::EffectSetting
{
public:    

    enum EffectSettingCode
    { 
        kMgefCode_None              = 0x0,
        kMgefCode_Invalid           = 0xFFFFFFFF,
        kMgefCode_DynamicFlag       = 0x80000000,
    };

    enum EffectSettingOBMEFlags
    {                     
        kMgefObmeFlag__ParamFlagA           = /*02*/ 0x00000004, // [deprecated, must be 0]
        kMgefObmeFlag_Beneficial            = /*03*/ 0x00000008, // part of the new three-state hostilities        
        kMgefObmeFlag__ParamFlagB           = /*10*/ 0x00010000, // [deprecated, must be 0]
        kMgefObmeFlag_NoProjRange           = /*11*/ 0x00020000, //
        kMgefObmeFlag_AtomicResistance      = /*12*/ 0x00040000, //
        kMgefObmeFlag__ParamFlagC           = /*13*/ 0x00080000, // [deprecated, must be 0]
        kMgefObmeFlag__ParamFlagD           = /*14*/ 0x00100000, // [deprecated, must be 0]
        kMgefObmeFlag_HideInMenus           = /*1E*/ 0x40000000, //
        kMgefObmeFlag_InfProjSpeed          = /*1F*/ 0x80000000, //
    };

    enum EffectSettingFlagShifts
    {      
        // vanilla flags
        kMgefFlagShift_Hostile              = 0x00,
        kMgefFlagShift_Recovers             = 0x01,
        kMgefFlagShift_Detrimental          = 0x02, // used only with ModAV handler
        kMgefFlagShift_MagnitudeIsPercent   = 0x03, // used for display name, if no name override given
        kMgefFlagShift_OnSelf               = 0x04,
        kMgefFlagShift_OnTouch              = 0x05,
        kMgefFlagShift_OnTarget             = 0x06,
        kMgefFlagShift_NoDuration           = 0x07,
        kMgefFlagShift_NoMagnitude          = 0x08,
        kMgefFlagShift_NoArea               = 0x09,
        kMgefFlagShift_FXPersists           = 0x0A, 
        kMgefFlagShift_Spells               = 0x0B,
        kMgefFlagShift_Enchantments         = 0x0C,
        kMgefFlagShift_NoAlchemy            = 0x0D,
        kMgefFlagShift_UnknownF             = 0x0E,
        kMgefFlagShift_NoRecast             = 0x0F,
        kMgefFlagShift_UseWeapon            = 0x10, // used only with Summon handlers, denotes use of mgefParam
        kMgefFlagShift_UseArmor             = 0x11, // used only with Summon handlers, denotes use of mgefParam
        kMgefFlagShift_UseActor             = 0x12, // used only with Summon handlers, denotes use of mgefParam
        kMgefFlagShift_UseSkill             = 0x13, // used only with ModAV handler, denotes use of mgefParam
        kMgefFlagShift_UseAttribute         = 0x14, // used only with ModAV handler, denotes use of mgefParam
        kMgefFlagShift_PCHasEffect          = 0x15,
        kMgefFlagShift_Disabled             = 0x16,
        kMgefFlagShift_UnknownO             = 0x17, // meta effects? no range?
        kMgefFlagShift_UseActorValue        = 0x18, // used only with ModAV handler, denotes use of mgefParam
        kMgefFlagShift_ProjectileTypeLow    = 0x19, 
        kMgefFlagShift_ProjectileTypeHigh   = 0x1A, 
        kMgefFlagShift_NoHitVisualFX        = 0x1B,
        kMgefFlagShift_PersistOnDeath       = 0x1C, 
        kMgefFlagShift_ExplodesWithForce    = 0x1D, 
        kMgefFlagShift_MagnitudeIsLevel     = 0x1E, // used for display name, if no name override given
        kMgefFlagShift_MagnitudeIsFeet      = 0x1F, // used for display name, if no name override given
        // OBME flags
        kMgefFlagShift__ParamFlagA          = 0x22, // [deprecated, must be 0]
        kMgefFlagShift_Beneficial           = 0x23, 
        kMgefFlagShift__ParamFlagB          = 0x30, // [deprecated, must be 0]
        kMgefFlagShift_NoProjRange          = 0x31,
        kMgefFlagShift_AtomicResistance     = 0x32,        
        kMgefFlagShift__ParamFlagC          = 0x33, // [deprecated, must be 0]
        kMgefFlagShift__ParamFlagD          = 0x34, // [deprecated, must be 0]
        kMgefFlagShift_HideInMenus          = 0x3E,
    };

    // cached vanilla effects  
    static EffectSetting*   defaultVFXEffect;
    static EffectSetting*   diseaseVFXEffect;
    static EffectSetting*   poisonVFXEffect;

    // additional members - this changes the size of EffectSetting!
    MEMBER /*++/++*/ MgefHandler*           effectHandler;      
    MEMBER /*++/++*/ UInt32                 mgefObmeFlags; // seperate field for overriden flags

    // virtual method overrides - TESFormIDListView
    _LOCAL /*010/034*/ virtual              ~EffectSetting();
    _LOCAL /*01C/040*/ virtual bool         LoadForm(TESFile& file); 
    _LOCAL /*024/048*/ virtual void         SaveFormChunks();
    _LOCAL /*06C/070*/ virtual void         LinkForm();
    _LOCAL /*074/078*/ virtual void         GetDebugDescription(BSStringT& output);
    _LOCAL /*0B4/0B8*/ virtual void         CopyFrom(TESForm& form); 
    _LOCAL /*0B8/0BC*/ virtual bool         CompareTo(TESForm& compareTo);
    #ifndef OBLIVION
    _LOCAL /*---/10C*/ virtual bool         DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result); 
    _LOCAL /*---/114*/ virtual void         SetInDialog(HWND dialog);
    _LOCAL /*---/118*/ virtual void         GetFromDialog(HWND dialog);
    _LOCAL /*---/11C*/ virtual void         CleanupDialog(HWND dialog);
    _LOCAL /*---/124*/ virtual void         SetupFormListColumns(HWND listView);
    _LOCAL /*---/130*/ virtual void         GetFormListDispInfo(void* displayInfo);
    _LOCAL /*---/134*/ virtual int          CompareInFormList(const TESForm& compareTo, int compareBy);
    #endif

    // methods: debugging
    _LOCAL BSStringT        GetDebugDescEx();

    // methods: conversion
    _LOCAL static UInt8     GetDefaultHostility(UInt32 mgefCode);  // returns vanilla hostility for vanilla effect codes, neutral for new effects
    _LOCAL static UInt32    GetDefaultMgefFlags(UInt32 mgefCode);  // returns (uneditable) vanilla mgefFlags    
    _LOCAL static UInt32    GetDefaultHandlerCode(UInt32 mgefCode); // returns vanilla handler for vanilla effect codes, ACTV for new effects
    _LOCAL bool             RequiresObmeMgefChunks(); // returns true if obme-specific chunks are required to serialize this effect

    // methods: fields
    _LOCAL bool             GetFlag(UInt32 flagShift) const; // use flag shift, not actual flag bit pattern, to accomodate > 32 flags
    _LOCAL void             SetFlag(UInt32 flagShift, bool newval);  // use flag shift, not actual flag bit pattern
    _LOCAL UInt8            GetHostility() const; // 3-state hostility (see OBME/Magic.h)
    _LOCAL void             SetHostility(UInt8 newval); // 3-state hostility
    _LOCAL UInt32           GetResistAV(); // standardizes 'invalid' value
    _LOCAL void             SetResistAV(UInt32 newResistAV); // standardizes 'invalid' value
    _LOCAL void             SetProjectileType(UInt32 newType); // not present in game

    // methods: effect codes
    _LOCAL static bool      IsMgefCodeVanilla(UInt32 mgefCode); // returns true if code is defined in vanilla oblivion
    _LOCAL static bool      IsMgefCodeValid(UInt32 mgefCode); // returns true if code is valid  
    _LOCAL static bool      IsMgefCodeDynamic(UInt32 mgefCode); // returns true if code is dynamically assigned
    _LOCAL static UInt32    GetUnusedDynamicCode(); // checks table & returns an unused dynamic mgef code
    _LOCAL static UInt32    GetUnusedStaticCode(); // checks table & returns an unused static mgef code
    _LOCAL static bool      ResolveModMgefCode(UInt32& mgefCode, TESFile& file); // does form-id like resolution on code from mod file          
    _LOCAL static bool      ResolveSavedMgefCode(UInt32& mgefCode); //  does form-id like resolution on code from savegame

    // methods: effect handler
    _LOCAL MgefHandler&             GetHandler();
    _LOCAL void                     SetHandler(MgefHandler* handler);

    // methods: CS dialogs
    #ifndef OBLIVION
    _LOCAL void                     InitializeDialog(HWND dialog);
    #endif

    // methods: hooks
    _LOCAL static void              Initialize();

    // constructor
    _LOCAL EffectSetting();

};

}   // end of OBME namespace