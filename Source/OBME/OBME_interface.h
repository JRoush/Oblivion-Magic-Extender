/*
    Interface class for the Oblivion Magic Extender

    When OBME recieves the 'PostLoad' message from obse, it dispatches it's own 'Interface' message to all other plugins.
        Message.Sender  -   the string "OBME"
        Message.Type    -   The value of the constant kMessageType.  This distinguishes the interface message from other 
                            potential messages from OBME.
        Message.Data    -   A pointer to an OBME_Interface object.

    This interface lets an obse plugin do the same things (generally speaking) that a script can do using the new OBME script commands.
    There is almost a one-to-one relationship between the interface functions and the new commands, so the documentation is mostly the 
    same (see the 'OBME Features' pdf file). 

    Interface Members:
        Version                 -   The version of the interface object by OBME.  This should be greater than or equal to
                                    the constant kInterfaceVersion as defined in this file, otherwise the player has an outdated
                                    version of OBME installed.
        EffectSettingComands    -   Functions that get or change EffectSetting objects.  Where the script versions take a magic 
                                    effect code, these functions take an EffectSetting* instead.  You can use LookupMgef() to get 
                                    the EffectSetting*, or use the standard OBSE/COEF methods.
        EffectItemCommands      -   Functions that get or change EffectItems on magic item forms.  Where the script versions take a 
                                    magic item formID + effect item index, these functions take an EffectItem* instead.  You can use 
                                    GetNthEffectItem() to get the EffectItem*, or use the standard OBSE/COEF methods.
        EffectHandlerComands    -   Functions to lookup effect handler objects by code, or to register new handlers

    This interface is not intended to be a comprehensive library for magic-related tasks.  It only exposes functionality that cannot
    (or should not) be achieved using the existing OBSE/COEF source code.
    *** You will need to use the header files from OBSE or COEF, and perhaps from OBME as well, to do anything useful ***

*/
#pragma once

// argument classes         --  You can find declarations for these classes in:
class   BSStringT;          // obse/GameData.h as "String" --or-- COEF/API/BSTypes/BSStringT.h
class   TESFile;            // obse/GameData.h as "ModEntry::Data"  --or-- COEF/API/TESFiles/TESFile.h
class   TESForm;            // obse/GameForms.h  --or-- COEF/API/TESForms/TESForm.h
class   TESObjectREFR;      // obse/GameObjects.h --or-- COEF/API/TESForms/TESObjectREFR.h
class   EffectSetting;      // OBME/EffectSetting.h
class   EffectItem;         // OBME/EffectItem.h
class   OutputLog;          // COEF/Utilities/OutputLog.h

class OBME_Interface
{
public:

    // obse message type 
    static const UInt32 kMessageType = 0x00000001;      // 'message type' code used by obme when dispatching interface

    // version Info
    static const UInt32 kInterfaceVersion = 0x02000100; // version of interface declaration in this file
    UInt32 /*00*/ version;                              // version of the interface passed by OBME (should be >= version above)

    // Typedef for clarity, to show that an argument or return value may take on any 32-bit data or pointer value
    typedef UInt32 MixedType;    
    enum MixedTypeCodes
    {
        // value type enumeration, copied from OBSEArrayVarInterface::Element
	    kType_Invalid,
	    kType_Numeric,
	    kType_FormID,
	    kType_String,
	    kType_Array,
    };

    //// 04 Subinterface for EffectSetting-related commands
    //class EffectSettingComandInterface
    //{
    //public:
    //    virtual /*00*/ bool            ResolveModMgefCode(UInt32& mgefCode, TESFile* file); // no equivalent script command 
    //    virtual /*04*/ bool            ResolveSavedMgefCode(UInt32& mgefCode); 
    //    virtual /*08*/ EffectSetting*  LookupMgef(UInt32 mgefCode); // no equivalent script command
    //    virtual /*0C*/ EffectSetting*  CreateMgef(UInt32 newCode, EffectSetting* cloneFrom);
    //    virtual /*10*/ UInt32          GetHandler(EffectSetting* mgef);                  
    //    virtual /*14*/ void            SetHandler(UInt32 newEHCode, EffectSetting* mgef); 
    //    virtual /*18*/ UInt8           GetHostility(EffectSetting* mgef);
    //    virtual /*1C*/ void            SetHostility(UInt8 newHostility, EffectSetting* mgef); 
    //    virtual /*20*/ MixedType       GetHandlerParam(const char* paramName, EffectSetting* mgef, UInt32& valueType);
    //    virtual /*24*/ bool            SetHandlerParam(const char* paramName, EffectSetting* mgef, MixedType newValue);
    //} effectSettingComands;

    //// 08 Subinterface for EffectItem-related commands
    //class EffectItemComandInterface
    //{
    //public:
    //    virtual /*00*/ EffectItem*     GetNthEffectItem(TESForm* magicObject, SInt32 efitIndex); // no equivalent script command
    //    virtual /*04*/ void            GetName(EffectItem* item, BSStringT& name);
    //    virtual /*08*/ void            SetName(EffectItem* item, const char* newName);
    //    virtual /*0C*/ void            ClearName(EffectItem* item);
    //    virtual /*10*/ const char*     GetIconPath(EffectItem* item);
    //    virtual /*14*/ void            SetIconPath(EffectItem* item, const char* newPath);
    //    virtual /*18*/ void            ClearIconPath(EffectItem* item);  
    //    virtual /*1C*/ UInt8           GetHostility(EffectItem* item);
    //    virtual /*20*/ void            SetHostility(EffectItem* item, UInt8 newHostility);
    //    virtual /*24*/ void            ClearHostility(EffectItem* item);
    //    virtual /*28*/ float           GetBaseCost(EffectItem* item);
    //    virtual /*2C*/ void            SetBaseCost(EffectItem* item, float newBaseCost);
    //    virtual /*30*/ void            ClearBaseCost(EffectItem* item);
    //    virtual /*34*/ UInt32          GetResistAV(EffectItem* item);
    //    virtual /*38*/ void            SetResistAV(EffectItem* item, UInt32 newResistAV);
    //    virtual /*3C*/ void            ClearResistAV(EffectItem* item);
    //    virtual /*40*/ UInt32          GetSchool(EffectItem* item);
    //    virtual /*44*/ void            SetSchool(EffectItem* item, UInt32 newSchool);
    //    virtual /*48*/ void            ClearSchool(EffectItem* item);
    //    virtual /*4C*/ UInt32          GetVFXCode(EffectItem* item);
    //    virtual /*50*/ void            SetVFXCode(EffectItem* item, UInt32 newVFXCode);
    //    virtual /*54*/ void            ClearVFXCode(EffectItem* item);
    //    virtual /*58*/ MixedType       GetHandlerParam(const char* paramName, EffectItem* item, UInt32& valueType);
    //    virtual /*5C*/ bool            SetHandlerParam(const char* paramName, EffectItem* item, MixedType newValue);
    //    virtual /*60*/ void            ClearHandlerParam(const char* paramName, EffectItem* item);
    //} effectItemComands;

    //// 0C Subinterface for EffectHandler-related commands
    //class EffectHandlerComandInterface
    //{
    //public:
    //    virtual /*00*/ EffectHandler*  LookupEffectHandler(UInt32 ehCode); // no equivalent script command
    //    virtual /*04*/ void            RegisterEffectHandler(EffectHandler* eh); // no equivalent script command
    //} effectHandlerComands;

    // Subinterface for special use (**for internal use by OBME only**)
    class PrivateInterface
    {
    public:
        virtual /*00*/ const char*      Description();
        virtual /*04*/ void             OBMETest(TESObjectREFR* thisObj, const char* argA, const char* argB, const char* argC);
    } privateCommands;

    // constructor (**for internal use by OBME only**)
    OBME_Interface();
};
