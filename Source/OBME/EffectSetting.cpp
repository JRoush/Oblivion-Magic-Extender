#include "OBME/EffectSetting.h"
#include "OBME/EffectSetting.rc.h"
#include "OBME/OBME_version.h"
#include "OBME/Magic.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/MagicItemForm.h" // TODO - replace with OBME/MagicItemForm.h
#include "API/Magic/MagicItemObject.h" // TODO - replace with OBME/MagicItemForm.h
#include "OBME/EffectHandlers/ValueModifierEffect.h"

#include "API/BSTypes/BSStringT.h"
#include "API/NiTypes/NiTMap.h"
#include "API/TES/TESDataHandler.h"
#include "API/TESForms/TESObject.h"
#include "API/TESFiles/TESFile.h"
#include "API/Actors/ActorValues.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogExtraData.h"
#include "API/CSDialogs/DialogConstants.h"
#include "API/ExtraData/ExtraDataList.h"
#include "API/Settings/Settings.h"
#include "Components/TESFileFormats.h"
#include "Utilities/Memaddr.h"

#include <set>
#include <typeinfo>

#pragma warning (disable:4800) // forced typecast of int to bool

// local declaration of module handle.
extern HMODULE hModule;

namespace OBME {

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}

// structs representing major MGEF chunk types
struct EffectSettingObmeChunk   // OBME chunk
{// size 30
    MEMBER /*00*/ UInt32        obmeRecordVersion;  // version
    MEMBER /*04*/ UInt8         _mgefParamResType;  // [deprecated] Resolution Type, for mgefParam, redundant to mgefFlags
    MEMBER /*05*/ UInt8         _mgefParamBResType; // [deprecated, must be 0] Resolution Type
    MEMBER /*06*/ UInt16        reserved06;
    MEMBER /*08*/ UInt32        effectHandler;      // effect handler code
    MEMBER /*0C*/ UInt32        mgefObmeFlags;      // bitmask
    MEMBER /*10*/ UInt32        _mgefParamB;        // [deprecated, must be 0]
    MEMBER /*14*/ UInt32        reserved14[7];
};
struct EffectSettingDataChunk   // DATA chunk
{// size 40
    MEMBER /*00*/ UInt32        mgefFlags;      // bitmask
    MEMBER /*04*/ float         baseCost;
    MEMBER /*08*/ UInt32        mgefParam;      // formid, extended enum, or neither, depending on vanilla flags
    MEMBER /*0C*/ UInt32        school;         // extended enum
    MEMBER /*10*/ UInt32        resistAV;       // extended enum
    MEMBER /*14*/ UInt16        numCounters;
    MEMBER /*16*/ UInt16        pad16;
    MEMBER /*18*/ UInt32        light;          // formid
    MEMBER /*1C*/ float         projSpeed;
    MEMBER /*20*/ UInt32        effectShader;   // formid
    MEMBER /*24*/ UInt32        enchantShader;  // formid
    MEMBER /*28*/ UInt32        castingSound;   // formid
    MEMBER /*2C*/ UInt32        boltSound;      // formid
    MEMBER /*30*/ UInt32        hitSound;       // formid
    MEMBER /*34*/ UInt32        areaSound;      // formid
    MEMBER /*38*/ float         enchantFactor;
    MEMBER /*3C*/ float         barterFactor;
};
struct EffectSettingDatxChunk   // DATX chunk, deprecated as of v1.beta4 but kept for compatibility
{// size 20
    MEMBER /*00*/ UInt32        effectHandler;      // effect handler code
    MEMBER /*04*/ UInt32        mgefFlagOverrides;  // bitmask
    MEMBER /*08*/ UInt32        mgefParamB;         // 
    MEMBER /*0C*/ UInt32        reserved0C[5];
};

// memory patch & hook addresses
#ifdef OBLIVION // bitmask of virtual methods that need to be manually copied from OBME vtbl to vanilla vtbl
    const UInt32 TESForm_PatchMethods[2] = { 0x28000290, 0x00006000 };
#else
    const UInt32 TESForm_PatchMethods[3] = { 0x50052000, 0x0000C000, 0x000032E8 };
#endif
memaddr TESForm_Vtbl                            (0x00A32B14,0x0095C914);    // EffectSetting::TESFormIDListView::{vtbl}
memaddr TESModel_Vtbl                           (0x00A32AF0,0x0095C8C4);    // EffectSetting::TESModel::{vtbl}
memaddr TESFullName_Vtbl                        (0x00A32AC4,0x0095C850);    // EffectSetting::TESFullName::{vtbl}
memaddr TESDescription_Vtbl                     (0x00A32AD8,0x0095C888);    // EffectSetting::TESDecription::{vtbl}
memaddr TESIcon_Vtbl                            (0x00A32AA8,0x0095C804);    // EffectSetting::TESIcon::{vtbl}
memaddr EffectSetting_Create                    (0x004165E0,0x0);
memaddr EffectSettingCollection_Add_Hook        (0x00417220,0x0056AC40);
memaddr EffectSettingCollection_AddLast_Hook    (0x00418DAA,0x0056C85A);
memaddr TESForm_CreateMgef_Hook                 (0x00448679,0x00479AB4);
memaddr TESForm_CreateMgef_Retn                 (0x004486A1,0x00479ADC);
memaddr TESDataHandler_LoadMgef_Hook            (0x0044E367,0x00483E06);
memaddr TESDataHandler_LoadMgef_Retn            (0x0044E3AC,0x00483E47);
memaddr TESCS_CreateMgefDialog_Patch            (0x0       ,0x0041A5DB);
memaddr TESDataHandler_AddForm_LookupPatch      (0x0       ,0x00481D1C); // entry for MGEF in form type switch table
memaddr TESDataHandler_AddForm_JumpTable        (0x0       ,0x00481CB0); // form type switch jump table
memaddr TESDataHandler_AddForm_JumpPatch        (0x0       ,0x00481D58); // entry for MGEF in form type switch jump table

// virtual methods - TESFormIDListView
bool EffectSetting::LoadForm(TESFile& file)
{
    /*
        LoadForm:
        -   Load chunks from record, parse
        -   Resolve all parameters to the current load order
        -   Update records from vanilla & older obme versions
    */
    if (file.GetRecordType() != TESForm::kFormType_EffectSetting) return false; // bad file or incorrect form type
    _VMESSAGE("Loading effect {%08X} /%08X", mgefCode, file.currentRecord.recordID);
    gLog.Indent();

    // initialize form from record
    file.InitializeFormFromRecord(*this);

    // flags for indicating what chunks have been loaded
    enum EffectSettingChunks
    {
        kMgefChunk_Edid     = 0x00000001,
        kMgefChunk_Eddx     = 0x00000002,
        kMgefChunk_Full     = 0x00000004,
        kMgefChunk_Desc     = 0x00000008,
        kMgefChunk_Icon     = 0x00000010,
        kMgefChunk_Modl     = 0x00000020,
        kMgefChunk_Data     = 0x00000040,
        kMgefChunk_Datx     = 0x00000080, // deprecated since v1 beta4
        kMgefChunk_Esce     = 0x00000100,
        kMgefChunk_Obme     = 0x00000200,
        kMgefChunk_Ehnd     = 0x00000400,
    }; 
    UInt32 loadFlags = 0;    // no chunks loaded
    UInt32 loadVersion = VERSION_VANILLA_OBLIVION;

    // reserve storage for chunk data that needs version-specific processing
    EffectSettingObmeChunk  obme;
    EffectSettingDataChunk  data;
    EffectSettingDatxChunk  datx;
    struct {UInt32* counters; UInt32  count;} esce = {0,0};        
    char edid[0x200];
    MgefHandler* ehnd = 0;

    // loop over chunks until end of record (or invalid chunk)
    for(UInt32 chunktype = file.GetChunkType(); chunktype; chunktype = file.GetNextChunk() ? file.GetChunkType() : 0)
    {
        // process chunk
        switch (Swap32(chunktype))
        {

        // 'fake' editor id chunk, actually stores mgefCode            
        case 'EDID':
            // already processed by loading routine
            loadFlags |= kMgefChunk_Edid;
            _VMESSAGE("EDID chunk: '%4.4s' {%08X}",&mgefCode,mgefCode);
            break;

        // obme data, added in v1.beta4 (so version is guaranteed >= VERSION_OBME_FIRSTVERSIONED)
        case 'OBME': 
            if (~loadFlags & kMgefChunk_Edid) _WARNING("Unexpected chunk 'OBME' - must immediately follow chunk 'EDID'");
            memset(&obme,0,sizeof(obme));
            file.GetChunkData(&obme,sizeof(obme));
            loadFlags |= kMgefChunk_Obme;
            _VMESSAGE("OBME chunk: version {%08X}, handler {%08X}",obme.obmeRecordVersion,obme.effectHandler);
            break;

        // obme auxiliary data pre-v1.beta4
        case 'DATX':
            if (~loadFlags & kMgefChunk_Data) _WARNING("Unexpected chunk 'DATX' - must come after chunk 'DATA'");
            memset(&datx,0,sizeof(datx));
            file.GetChunkData(&datx,sizeof(datx));
            loadFlags |= kMgefChunk_Datx;
            _VMESSAGE("DATX chunk, handler {%08X}",datx.effectHandler);
            break;

        // actual editor id
        case 'EDDX':               
            memset(edid,0,sizeof(edid));
            file.GetChunkData(edid,sizeof(edid));
            loadFlags |= kMgefChunk_Eddx;
            _VMESSAGE("EDDX chunk: '%s'",edid);
            break;

        // name
        case 'FULL':            
            TESFullName::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Full;
            _VMESSAGE("FULL chunk: '%s'",name.c_str());
            break;

        // description
        case 'DESC':            
            TESDescription::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Desc;
            _VMESSAGE("DESC chunk: '%s'",GetDescription(this,Swap32('DESC')));
            break;

        // icon
        case 'ICON':            
            TESIcon::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Icon;
            _VMESSAGE("ICON chunk: path '%s'",texturePath.c_str());
            break;

        // model
        case 'MODL':
        case 'MODB':
        case 'MODT':            
            TESModel::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Modl;
            _VMESSAGE("%4.4s chunk: model path '%s'",&chunktype,ModelPath());
            break;

        // main effect data
        case 'DATA':
            if (file.currentChunk.chunkLength == sizeof(data))
            {
                // normal DATA chunk
                memset(&data,0,sizeof(data));
                file.GetChunkData(&data,sizeof(data));
                loadFlags |= kMgefChunk_Data;
                _VMESSAGE("DATA chunk: flags {%08X}",data.mgefFlags);
            }
            else if (file.currentChunk.chunkLength == sizeof(data) + sizeof(datx))
            {
                // extended DATA chunk from v1.beta1
                UInt8* buffer = new UInt8[file.currentChunk.chunkLength];
                file.GetChunkData(buffer,0);
                memcpy(&data,buffer,sizeof(data));
                memcpy(&datx,buffer+sizeof(data),sizeof(datx));
                delete buffer;
                loadFlags |= kMgefChunk_Data | kMgefChunk_Datx;
                _VMESSAGE("DATA chunk (extended): flags {%08X}, handler {%08X}",data.mgefFlags,datx.effectHandler);
            }
            break;

        // counter effects by code
        case 'ESCE':               
            if (~loadFlags & kMgefChunk_Data) _WARNING("Unexpected chunk 'ESCE' - must come after chunk 'DATA'");
            esce.count = file.currentChunk.chunkLength / 4;
            if (esce.counters) MemoryHeap::FormHeapFree(esce.counters); // clear any data from previously loaded ESCE chunk
            esce.counters = (UInt32*)MemoryHeap::FormHeapAlloc(esce.count * 4);
            file.GetChunkData(esce.counters,esce.count * 4);
            // done
            loadFlags |= kMgefChunk_Esce;
            _DMESSAGE("ESCE chunk: %i counters",esce.count);
            break;

        // effect handler, added in v2 (so version is guaranteed > VERSION_OBME_LASTV1)
        case 'EHND':
            if (loadFlags & kMgefChunk_Obme)
            {
                if (ehnd) delete ehnd;   // clear any data from previously loaded EHND chunk
                ehnd = MgefHandler::Create(obme.effectHandler,*this);   // create handler from code
                if (ehnd) ehnd->LoadHandlerChunk(file,obme.obmeRecordVersion); // load handler data
                else _ERROR("Unrecognized EffectHandler code '%4.4s' (%08X)", &obme.effectHandler, obme.effectHandler);              
                _VMESSAGE("EHND chunk"); 
            }
            else _WARNING("Unexpected chunk 'EHND' - must come after chunk 'OBME'");
            // done
            loadFlags |= kMgefChunk_Ehnd;
            break;

        // unrecognized chunk type
        default:
            _WARNING("Unexpected chunk '%4.4s' {%08X} w/ size %08X", &chunktype, chunktype, file.currentChunk.chunkLength);
            break;

        }
        // continue to next chunk
    } 

    // process record version
    if (loadFlags & kMgefChunk_Obme) loadVersion = obme.obmeRecordVersion;
    else if (loadFlags & kMgefChunk_Datx) loadVersion = VERSION_OBME_LASTUNVERSIONED;  // assume last "unversioned" version
    else loadVersion = VERSION_VANILLA_OBLIVION;
    _DMESSAGE("Final Record Version %08X",loadVersion);

    // process main data & auxiliary chunks
    if (loadFlags & kMgefChunk_Data)
    {
        // resolve simple references
        TESFileFormats::ResolveModValue(data.light,file,TESFileFormats::kResolutionType_FormID); // Light
        TESFileFormats::ResolveModValue(data.effectShader,file,TESFileFormats::kResolutionType_FormID); // EffectShader
        TESFileFormats::ResolveModValue(data.enchantShader,file,TESFileFormats::kResolutionType_FormID); // EnchantmentShader
        TESFileFormats::ResolveModValue(data.castingSound,file,TESFileFormats::kResolutionType_FormID); // Cast Sound
        TESFileFormats::ResolveModValue(data.boltSound,file,TESFileFormats::kResolutionType_FormID); // Bolt Sound
        TESFileFormats::ResolveModValue(data.hitSound,file,TESFileFormats::kResolutionType_FormID); // Hit Sound
        TESFileFormats::ResolveModValue(data.areaSound,file,TESFileFormats::kResolutionType_FormID); // Area Sound
        TESFileFormats::ResolveModValue(data.resistAV,file,TESFileFormats::kResolutionType_ActorValue); // Resistance AV (for AddActorValues)
        TESFileFormats::ResolveModValue(data.school,file,TESFileFormats::kResolutionType_FormID); // School (for extended school types)
        // backup original DATA values that require special processing
        UInt32 _mgefFlags = mgefFlags;
        // copy DATA onto mgef
        memcpy(&mgefFlags,&data,sizeof(data));
        SetResistAV(data.resistAV); // standardize resistance value
        // flags (mgef + obme)
        if (loadVersion == VERSION_VANILLA_OBLIVION) 
        {            
            mgefFlags = (data.mgefFlags & 0x0FC03C00) | (_mgefFlags & ~0x0FE03C00);  // vanilla mgef records can't override all flags
            mgefObmeFlags = 0;
            SetHostility(GetDefaultHostility(mgefCode));
        }
        else if (loadVersion > VERSION_VANILLA_OBLIVION && loadVersion < MAKE_VERSION(2,0,0,0))
        {
            mgefFlags = data.mgefFlags & ~EffectSetting::kMgefFlag_PCHasEffect;
            _DMESSAGE("MgefFlags trimmed to %08X",mgefFlags);
            mgefObmeFlags = datx.mgefFlagOverrides & ~0x00190004; // v1 'Param' flags not stored in obmeFlags field 
        }
        else
        {
            mgefFlags = data.mgefFlags & ~EffectSetting::kMgefFlag_PCHasEffect;
            mgefObmeFlags = obme.mgefObmeFlags;
        }
        // counter effects
        if (numCounters != esce.count) _WARNING("DATA chunk expects %i counter effects, but ESCE chunk contains %i", numCounters,esce.count);
        if (counterArray) MemoryHeap::FormHeapFree(counterArray);
        counterArray = esce.counters;
        numCounters = esce.count;
        for (int c = 0; c < numCounters; c++) TESFileFormats::ResolveModValue(counterArray[c],file,TESFileFormats::kResolutionType_MgefCode); // resolve codes
        // handler & handler-specific parameters
        if (loadVersion > VERSION_VANILLA_OBLIVION && loadVersion < MAKE_VERSION(2,0,0,0))
        {
            // get handler parameters from DATX or OBME chunk, depending on version
            UInt32 ehCode = (loadVersion < VERSION_OBME_FIRSTVERSIONED) ? datx.effectHandler : obme.effectHandler;
            UInt32 hParamA = mgefParam;
            UInt32 hParamB = (loadVersion < VERSION_OBME_FIRSTVERSIONED) ? datx.mgefParamB : obme._mgefParamB;
            UInt32 hFlags = (loadVersion < VERSION_OBME_FIRSTVERSIONED) ? datx.mgefFlagOverrides : obme.mgefObmeFlags;
            // reset handler-specific param & flags on mgef proper
            mgefParam = 0;
            mgefFlags &= ~0x011F0000;   // leave Detrimental flag alone
            // set handler
            SetHandler(MgefHandler::Create(ehCode,*this));
            if (ValueModifierMgefHandler* mdav = dynamic_cast<ValueModifierMgefHandler*>(&GetHandler()))
            {
                // AV source & AV
                TESFileFormats::ResolveModValue(hParamA,file,TESFileFormats::kResolutionType_ActorValue);
                TESFileFormats::ResolveModValue(hParamB,file,TESFileFormats::kResolutionType_ActorValue);
                if (hParamA == ActorValues::kActorVal_Strength && hParamB == ActorValues::kActorVal_Luck) SetFlag(kMgefFlagShift_UseAttribute,true);
                else if (hParamA == ActorValues::kActorVal_Armorer && hParamB == ActorValues::kActorVal_Speechcraft) SetFlag(kMgefFlagShift_UseSkill,true);
                else 
                {
                    mgefParam = hParamA;
                    SetFlag(kMgefFlagShift_UseActorValue,true);
                }
                // avPart
                mdav->avPart = (hFlags >> (kMgefFlagShift__ParamFlagC-0x20)) & 0x3;
                if (hFlags & kMgefObmeFlag__ParamFlagB) mdav->avPart = ValueModifierMgefHandler::kAVPart_MaxSpecial;
            }
            else // TODO - support other 3 handlers
            {
            } 
        }
        else
        {
            SetHandler((loadVersion == VERSION_VANILLA_OBLIVION) ? 0 : ehnd);
            if (mgefFlags & (kMgefFlag_UseWeapon | kMgefFlag_UseArmor | kMgefFlag_UseActor))
                TESFileFormats::ResolveModValue(mgefParam,file,TESFileFormats::kResolutionType_FormID); // param is a summoned formID
            else if (mgefFlags & kMgefFlag_UseActorValue) 
                TESFileFormats::ResolveModValue(mgefParam,file,TESFileFormats::kResolutionType_ActorValue); // param is a modified avCode
        }
    }

    // process editorID
    if (loadVersion > VERSION_VANILLA_OBLIVION && (loadFlags & kMgefChunk_Eddx)) // use EDDX chunk for non-vanilla records
    {
        SetEditorID(edid);
    }
    else if (loadVersion < MAKE_VERSION(2,0,0,0)) // use stringified mgefCode for vanilla records + v1 records w/o EDDX chunk
    {
        edid[4] = 0;
        *(UInt32*)edid = mgefCode;
        SetEditorID(edid);
        _DMESSAGE("Autoset EditorID: '%s'",edid);
    }

    // done
    gLog.Outdent();
    return true;
}
void EffectSetting::SaveFormChunks()
{
    int requiresOBME = RequiresObmeMgefChunks();
    _VMESSAGE("Saving %s (requires OBME chunks = %i)", GetDebugDescEx().c_str(), requiresOBME); 
    gLog.Indent();

    // initialize form record & save editor id    
    #ifndef OBLIVION    
        BSStringT edid = GetEditorID();
        editorID = "";  // temporarily clear editorID so it's not saved by InitializeFormRecord
        InitializeFormRecord();
        editorID = edid;
    #else
        InitializeFormRecord();
    #endif

    // mgefCode (saved in EDID chunk with a trailing zero)
    char codestr[5] = {{0}};
    *(UInt32*)codestr = mgefCode;
    PutFormRecordChunkData(Swap32('EDID'),codestr,sizeof(codestr));
    _DMESSAGE("EDID chunk: '%s'",codestr);

    // save OBME data
    if (requiresOBME)
    {        
        EffectSettingObmeChunk obme;
        memset(&obme,0,sizeof(obme));   
        obme.obmeRecordVersion = OBME_VERSION(OBME_MGEF_VERSION);
        obme.effectHandler = GetHandler().HandlerCode();
        obme.mgefObmeFlags = mgefObmeFlags;
        // set resolution info (deprecated, kept only for convenience with TES4Edit)
        if (mgefFlags & (kMgefFlag_UseWeapon | kMgefFlag_UseArmor | kMgefFlag_UseActor))
        {
            obme._mgefParamResType = TESFileFormats::kResolutionType_FormID; // param is a summoned formID
        }
        else if (mgefFlags & kMgefFlag_UseActorValue)
        {
            obme._mgefParamResType = TESFileFormats::kResolutionType_ActorValue; // param is a modified avCode
        }
        // save
        PutFormRecordChunkData(Swap32('OBME'),&obme,sizeof(obme));
        _DMESSAGE("OBME chunk: version {%08X}",obme.obmeRecordVersion);
    }

    // editorID
    if (requiresOBME && (GetEditorID() == 0 || strcmp(GetEditorID(),codestr) != 0))
    {
        // editorID is not identical to mgefCode, save as an EDDX chunk
        PutFormRecordChunkData(Swap32('EDDX'),(void*)GetEditorID(),strlen(GetEditorID())+1);
        _DMESSAGE("EDDX chunk: '%s'",GetEditorID());
    }    

    // base form components
    TESFullName::SaveComponent();
    TESDescription::SaveComponent();
    TESIcon::SaveComponent(Swap32('ICON'));
    TESModel::SaveComponent(Swap32('MODL'),Swap32('MODB'),Swap32('MODT'));

     // main data chunk
    if (true)
    {
        EffectSettingDataChunk data;
        memcpy(&data,&mgefFlags,sizeof(data));
        if (light) data.light = ((TESForm*)light)->formID;
        if (effectShader) data.effectShader = ((TESForm*)effectShader)->formID;
        if (enchantShader) data.enchantShader = ((TESForm*)enchantShader)->formID;
        if (castingSound) data.castingSound = ((TESForm*)castingSound)->formID;
        if (boltSound) data.boltSound = ((TESForm*)boltSound)->formID;
        if (hitSound) data.hitSound = ((TESForm*)hitSound)->formID;
        if (areaSound) data.areaSound = ((TESForm*)areaSound)->formID;
        PutFormRecordChunkData(Swap32('DATA'),&data,sizeof(data));
        _DMESSAGE("DATA chunk: flags={%08X}, resistAV=%08X",mgefFlags,resistAV);
    }

    // counter-effect list
    if ((numCounters > 0) && counterArray)
    {
        // save array of effect codes
        PutFormRecordChunkData(Swap32('ESCE'),counterArray,numCounters*sizeof(UInt32));
        _DMESSAGE("ESCE chunk: %i entries",numCounters);
    }
    
    // EffectHandler
    if (requiresOBME) GetHandler().SaveHandlerChunk();

    // finalize form record
    FinalizeFormRecord();
    gLog.Outdent();
}
void EffectSetting::LinkForm()
{
    
    if (formFlags & kFormFlags_Linked) return;  // form already linked
    _VMESSAGE("Linking %s", GetDebugDescEx().c_str()); 

    // resolve form light, sound, shaders formids into pointers
    // NOTE: because TESSound, TESEffectShader, and TESObjectLIGH are not yet defined in COEF, the
    // normal dynamic cast check on the form pointer is omitted.
    // If any of these formIDs refer to forms of the wrong type, all hell will break loose.
    castingSound = (TESSound*)TESForm::LookupByFormID((UInt32)castingSound);
    boltSound = (TESSound*)TESForm::LookupByFormID((UInt32)boltSound);
    hitSound = (TESSound*)TESForm::LookupByFormID((UInt32)hitSound);
    areaSound = (TESSound*)TESForm::LookupByFormID((UInt32)areaSound);
    effectShader = (TESEffectShader*)TESForm::LookupByFormID((UInt32)effectShader);
    enchantShader = (TESEffectShader*)TESForm::LookupByFormID((UInt32)enchantShader);
    light = (TESObjectLIGH*)TESForm::LookupByFormID((UInt32)light);
    #ifndef OBLIVION
    if (castingSound) ((TESForm*)castingSound)->AddReference(this);
    if (boltSound) ((TESForm*)boltSound)->AddReference(this);
    if (hitSound) ((TESForm*)hitSound)->AddReference(this);
    if (effectShader) ((TESForm*)effectShader)->AddReference(this);
    if (enchantShader) ((TESForm*)enchantShader)->AddReference(this);
    if (light) ((TESForm*)light)->AddReference(this);
    // TODO - school, resistanceAV
    #endif
    
    GetHandler().LinkHandler(); // link handler object

    // set linked flag
    formFlags |= kFormFlags_Linked;
}
void EffectSetting::GetDebugDescription(BSStringT& output)
{
    output.Format("MGEF form '%s'/%08X {%4.4s:%08X} '%s'",GetEditorID(),formID,&mgefCode,mgefCode,name.c_str());
}
void EffectSetting::CopyFrom(TESForm& copyFrom)
{      
    _VMESSAGE("Copying %s",GetDebugDescEx().c_str());

    // convert form to mgef
    EffectSetting* mgef = (EffectSetting*)dynamic_cast<::EffectSetting*>(&copyFrom);
    if (!mgef)
    {
        BSStringT desc;
        copyFrom.GetDebugDescription(desc);        
        _WARNING("Attempted copy from %s",desc.c_str());
        return;
    }    

    // copy effect code
    if (mgefCode != mgef->mgefCode)
    {
        if (formFlags & TESForm::kFormFlags_Temporary) mgefCode = mgef->mgefCode;   // this form is temporary, simply copy effect code
        else if (mgef->formFlags & TESForm::kFormFlags_Temporary)
        {
            // other form is temporary, but this one is not
            bool proceed = true;
            #ifndef OBLIVION
            // prompt user to confirm new code
            char format[0x200], prompt[0x200], title[0x200];      
            LoadString(hModule,IDS_MGEF_MGEFCODEWARNINGPROMPT,format,sizeof(format));   // load prompt string from resource table
            sprintf_s(prompt,sizeof(prompt),format,GetEditorID(),formID,mgefCode,mgef->mgefCode);             
            LoadString(hModule,IDS_MGEF_MGEFCODEWARNINGTITLE,title,sizeof(title));   // load title string from resource table
            proceed = (IDYES == MessageBox(dialogHandle,prompt,title,MB_YESNO|MB_DEFBUTTON2|MB_ICONINFORMATION|MB_APPLMODAL));
            #endif
            if (proceed)
            {
                // user confirms
                EffectSettingCollection::collection.RemoveAt(mgefCode); // unregister current code
                mgefCode = mgef->mgefCode; // copy code
                if (IsMgefCodeValid(mgefCode)) EffectSettingCollection::collection.SetAt(mgefCode,this); // register under new code
                #ifndef OBLIVION
                // TODO - notify all EffectItemLists and EffectSettings that use this code that it has changed
                #endif
            }
        }
    }

    // call vanilla mgef copy
    UInt32 curCode = mgefCode; // cache mgefCode
    ::EffectSetting::CopyFrom(copyFrom); // (copies effect code)
    mgefCode = curCode; // restore cached code

    // copy resistAV (again) to standardize 'invalid' av code
    SetResistAV(mgef->GetResistAV());

    // copy flags
    mgefObmeFlags = mgef->mgefObmeFlags;

    // copy handler
    MgefHandler* newHandler = MgefHandler::Create(mgef->GetHandler().HandlerCode(),*this);
    newHandler->CopyFrom(mgef->GetHandler());
    SetHandler(newHandler);

    // TODO - references in CS
}
bool EffectSetting::CompareTo(TESForm& compareTo)
{        
    _DMESSAGE("Comparing %s",GetDebugDescEx().c_str());

    // define failure codes - all of which must evaluate to boolean true
    enum
    {
        kCompareSuccess         = 0,
        kCompareFail_General    = 1,
        kCompareFail_Polymorphic,
        kCompareFail_ResistAV,
        kCompareFail_Counters,
        kCompareFail_BaseMgef,
        kCompareFail_ObmeFlags,
        kCompareFail_Handler,
    };

    // convert form to mgef
    EffectSetting* mgef = (EffectSetting*)dynamic_cast<::EffectSetting*>(&compareTo);
    if (!mgef)
    {
        BSStringT desc;
        compareTo.GetDebugDescription(desc);    
        _WARNING("Attempted compare to %s",desc.c_str());
        return BoolEx(kCompareFail_Polymorphic);
    }    

    // compare resistances
    if (GetResistAV() != mgef->GetResistAV()) return BoolEx(kCompareFail_ResistAV); // resistances don't match

    // compare counter effects
    if (numCounters != mgef->numCounters) return BoolEx(kCompareFail_Counters); // counter effect count doesn't match
    if (numCounters)
    {
        bool* counterFound = new bool[numCounters];
        for (int j = 0; j < numCounters; j++) counterFound[j] = false;
        for (int i = 0; i < numCounters; i++)
        {
            bool found = false;
            for (int j = 0; j < numCounters; j++)
            {
                if (counterArray[i] == mgef->counterArray[j] && !counterFound[j]) // check if effect matches an unmatched counter
                {
                    found = true; // note that a counter was found
                    counterFound[j] = true; // mark that this counter effect has been matched
                    break; // move to next effect
                }
            }
            if (!found)  return BoolEx(kCompareFail_Counters); // unmatched counter effect
        }
    }
   
    // cache resistance -  sidestep issues with different 'invalid' codes
    UInt32 _resistAV = resistAV; 
    resistAV = mgef->resistAV;
    // cache counter effects - sidestep differences in order
    UInt16 _numCounters = numCounters;
    UInt32* _counterArray = counterArray;
    numCounters = mgef->numCounters;
    counterArray = mgef->counterArray;
    // call base compare method
    bool noMatch = ::EffectSetting::CompareTo(compareTo);
    // restore cached fields
    resistAV = _resistAV;
    numCounters = _numCounters; 
    counterArray = _counterArray;
    if (noMatch) return BoolEx(kCompareFail_BaseMgef); // other base fields did not match    

    // compare flags
    if (mgefObmeFlags != mgef->mgefObmeFlags) return BoolEx(kCompareFail_ObmeFlags);

    // compare handler
    if (GetHandler().CompareTo(mgef->GetHandler())) return BoolEx(kCompareFail_Handler);

    return BoolEx(kCompareSuccess);
}
#ifndef OBLIVION
static const UInt32 kTabCount = 4;
static const UInt32 kTabTemplateID[kTabCount] = {IDD_MGEF_GENERAL,IDD_MGEF_DYNAMICS,IDD_MGEF_FX,IDD_MGEF_HANDLER};
static const char* kTabLabels[kTabCount] = {"General","Dynamics","FX","Handler"};
static const char* kNoneEntry = " NONE ";
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; // send in lieu of WM_COMMAND to avoid problems with TESFormIDListVIew::DlgProc 
bool ExtraDataList_RemoveData(BaseExtraList* list, BSExtraData* data, bool destroy = true)
{
    // helper functions - find & remove specified data from extra list
    // unlike the methods defined by the API, this will only remove the specified item, not all items of the given type
    if (!list || !data) return false; // bad arguments
    bool found = false;
    int typeCount = 0;
    BSExtraData** prev = &list->extraList;
    for (BSExtraData* extra = list->extraList; extra; extra = extra->extraNext)
    {        
        if (extra->extraType == data->extraType) typeCount++;    // found extra with same type
        if (extra == data) 
        {            
            *prev = extra->extraNext;   // item found, remove from LL
            typeCount--; //decrement type count
            found = true;
        }
        else prev = &(**prev).extraNext;       
    }
    if (typeCount == 0) list->extraTypes[data->extraType/8] &= ~(1<<(data->extraType%8)); // clear extra list flags
    if (destroy) delete data;  // destroy extra data object
    return found;
}
TESDialog::Subwindow* InsertTabSubwindow(HWND dialog, HWND tabs, INT index)
{
    _DMESSAGE("Inserting tab %i '%s' w/ template ID %08X in tabctrl %08X of dialog %08X",index,kTabLabels[index],kTabTemplateID[index],tabs,dialog);
    // insert tab into tabs control
    TESTabControl::InsertItem(tabs,index,kTabLabels[index],(void*)index);
    // create a subwindow object for the tab
    TESDialog::Subwindow* subwindow = new TESDialog::Subwindow;
    // set subwindow position relative to parent dialog
    subwindow->hDialog = dialog;
    subwindow->hInstance = (HINSTANCE)hModule;
    subwindow->hContainer = tabs;
    RECT rect;
    GetWindowRect(tabs,&rect);
    TabCtrl_AdjustRect(tabs,false,&rect);
    subwindow->position.x = rect.left;
    subwindow->position.y = rect.top;
    ScreenToClient(dialog,&subwindow->position); 
    // build subwindow control list (copy controls from tab dialog template into parent dialog)
    TESDialog::BuildSubwindow(kTabTemplateID[index],subwindow);
    // add subwindow extra data to parent
    TESDialog::AddDialogExtraSubwindow(dialog,kTabTemplateID[index],subwindow);
    // hide tab controls
    for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
    {
        ShowWindow(node->data, false);
    }
    return subwindow;
}
void ShowTabSubwindow(HWND dialog, INT index, bool show)
{
    _DMESSAGE("Showing (%i) Tab %i '%s'",show,index,kTabLabels[index]);
    // fetch subwindow for tab    
    TESDialog::Subwindow* subwindow = 0;
    if (subwindow = TESDialog::GetSubwindow(dialog,kTabTemplateID[index]))
    {
        _DMESSAGE("Showing subwindow <%p>",subwindow);
        // show controls
        if (subwindow->hSubwindow) 
        {
            // subwindow is instantiated as an actual subwindow
            ShowWindow(subwindow->hSubwindow, show);
        }  
        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            // iteratre through controls & show
            ShowWindow(node->data, show);
        }
    }
    // handler subwindow
    DialogExtraSubwindow* extraSubwindow = (DialogExtraSubwindow*)GetWindowLong(GetDlgItem(dialog,IDC_MGEF_HANDLER),GWL_USERDATA);
    if (kTabTemplateID[index] == IDD_MGEF_HANDLER && extraSubwindow && extraSubwindow->subwindow)
    {
        _DMESSAGE("Showing handler subwindow <%p>",extraSubwindow->subwindow);
        // this is the effect handler tab, show/hide handler subwindow as well
        if (extraSubwindow->subwindow->hSubwindow) 
        {
            // subwindow is instantiated as an actual subwindow
            ShowWindow(extraSubwindow->subwindow->hSubwindow, show);
        }  
        for (BSSimpleList<HWND>::Node* node = &extraSubwindow->subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            // iteratre through controls & show
            ShowWindow(node->data, show);
        }
    }
}
void EffectSetting::InitializeDialog(HWND dialog)
{
    _DMESSAGE("Initializing MGEF dialog ...");
    HWND ctl;

    // build tabs
    ctl = GetDlgItem(dialog,IDC_MGEF_TABS);
    for (UInt32 i = 0; i < kTabCount; i++) InsertTabSubwindow(dialog,ctl,i);
    // select first tab
    TabCtrl_SetCurSel(ctl,0);
    ShowTabSubwindow(dialog,0,true);

    // GENERAL
    {
        // school
        ctl = GetDlgItem(dialog,IDC_MGEF_SCHOOL);
        TESComboBox::Clear(ctl);
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)Magic::kSchool__MAX);
        for (int i = 0; i < Magic::kSchool__MAX; i++)
        {
            TESComboBox::AddItem(ctl,Magic::GetSchoolName(i),(void*)i);
        }
        // effect item description callback
        ctl = GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK);
        TESComboBox::PopulateWithScripts(ctl,true,false,false);
        TESComboBox::AddItem(ctl,"< Magnitude is % >",(void*)EffectSetting::kMgefFlagShift_MagnitudeIsPercent);
        TESComboBox::AddItem(ctl,"< Magnitude is Level >",(void*)EffectSetting::kMgefFlagShift_MagnitudeIsLevel);
        TESComboBox::AddItem(ctl,"< Magnitude is Feet >",(void*)EffectSetting::kMgefFlagShift_MagnitudeIsFeet);
        // counter list columns
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        TESListView::ClearColumns(ctl);
        TESListView::ClearItems(ctl);
        SetupFormListColumns(ctl);
        SetWindowLong(ctl,GWL_USERDATA,1); // sorted by editorid
        // counter effect add list
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectSetting,true,false);
    }

    // DYNAMICS
        // Cost Callbacks
        ctl = GetDlgItem(dialog,IDC_MGEF_GETCOSTCALLBACK);
        TESComboBox::PopulateWithScripts(ctl,true,false,false);
        ctl = GetDlgItem(dialog,IDC_MGEF_SETCOSTCALLBACK);
        TESComboBox::PopulateWithScripts(ctl,true,false,false);
        // Shielding
        ctl = GetDlgItem(dialog,IDC_MGEF_PROVIDESSHIELDING);
        TESComboBox::Clear(ctl);
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)0);
        ctl = GetDlgItem(dialog,IDC_MGEF_DAMAGESSHIELDING);
        TESComboBox::Clear(ctl);
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)0);
        // Resistance AV
        ctl = GetDlgItem(dialog,IDC_MGEF_RESISTVALUE);
        TESComboBox::PopulateWithActorValues(ctl,true,false);
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)ActorValues::kActorVal__UBOUND); // game uses 0xFFFFFFFF as an invalid resistance code
        // Hostility
        ctl = GetDlgItem(dialog,IDC_MGEF_HOSTILITY);
        TESComboBox::Clear(ctl);
        TESComboBox::AddItem(ctl,"Neutral",(void*)Magic::kHostility_Neutral);
        TESComboBox::AddItem(ctl,"Hostile",(void*)Magic::kHostility_Hostile);
        TESComboBox::AddItem(ctl,"Beneficial",(void*)Magic::kHostility_Beneficial);
        // Projectile Type
        ctl = GetDlgItem(dialog,IDC_MGEF_PROJECTILETYPE);
        TESComboBox::Clear(ctl);
        for (int i = 0; i < Magic::kProjType__MAX; i++)
        {
            TESComboBox::AddItem(ctl,Magic::projectileTypeNames[i]->value.s,(void*)i);
        }

    // FX
        // Light, Hit + Enchant shaders
        ctl = GetDlgItem(dialog,IDC_MGEF_LIGHT);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Light,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_EFFECTSHADER);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectShader,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_ENCHANTSHADER);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectShader,true,true);
        // Sounds
        ctl = GetDlgItem(dialog,IDC_MGEF_CASTINGSOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_BOLTSOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_HITSOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_AREASOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);    

    // HANDLER
        ctl = GetDlgItem(dialog,IDC_MGEF_HANDLER);
        TESComboBox::Clear(ctl);
        UInt32 ehCode = 0;
        const char* ehName = 0;
        while (EffectHandler::GetNextHandler(ehCode,ehName))
        {
            TESComboBox::AddItem(ctl,ehName,(void*)ehCode);
        }
        SetWindowLong(ctl,GWL_USERDATA,0); // store DialogExtraSubwindow* for handler in UserData
}
bool EffectSetting::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    //_DMESSAGE("MSG {%p} WP {%p} LP {%p}",uMsg,wParam,lParam);
    HWND ctl;
    char buffer[0x50];
    void* curData;

    // parse message
    switch (uMsg)
    {
    case WM_USERCOMMAND:
    case WM_COMMAND:
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam))  // switch on control id
        {
        case IDC_MGEF_DYNAMICMGEFCODE:  // Dynamic mgefCode checkbox
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Dynamic code check clicked");  
                ctl = GetDlgItem(dialog,IDC_MGEF_MGEFCODE);
                if (IsDlgButtonChecked(dialog,IDC_MGEF_DYNAMICMGEFCODE))
                {
                    // dynamic mgefCode                
                    sprintf_s(buffer,sizeof(buffer),"%08X", IsMgefCodeDynamic(mgefCode) ? mgefCode : GetUnusedDynamicCode());
                    EnableWindow(ctl,false);
                }
                else
                {
                    // static mgefCode                
                    sprintf_s(buffer,sizeof(buffer),"%08X", IsMgefCodeDynamic(mgefCode) ? GetUnusedStaticCode() : mgefCode);
                    EnableWindow(ctl,true);
                }
                SetWindowText(ctl,buffer);
                result = 0; return true;   // retval = false signals command handled    
            }
            break;
        case IDC_MGEF_ADDCOUNTER:     // Add Counter Effect button
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Add counter effect");  
                curData = TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES));
                if (curData) 
                {              
                    ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
                    TESListView::InsertItem(ctl,curData); // insert new item
                    ListView_SortItems(ctl,TESFormIDListView::FormListComparator,GetWindowLong(ctl,GWL_USERDATA)); // (re)sort items
                    TESListView::ForceSelectItem(ctl,TESListView::LookupByData(ctl,curData)); // force select new entry
                }
                result = 0; return true;    // retval = false signals command handled
            }
            break;
        case IDC_MGEF_REMOVECOUNTER:    // Remove Counter effect button
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Remove counter effect");  
                ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
                for (int i = ListView_GetNextItem(ctl,-1,LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(ctl,i-1,LVNI_SELECTED))
                {
                    ListView_DeleteItem(ctl,i);
                }
                result = 0; return true;    // retval = false signals command handled   
            }
            break;
        case IDC_MGEF_HANDLER:  // Handler selection combo box
        {
            if (commandCode == CBN_SELCHANGE)
            {
                _VMESSAGE("Handler changed");
                ctl = GetDlgItem(dialog,IDC_MGEF_HANDLER);
                // create a new handler if necessary
                UInt32 ehCode = (UInt32)TESComboBox::GetCurSelData(ctl);
                if (ehCode != GetHandler().HandlerCode()) SetHandler(MgefHandler::Create(ehCode,*this));
                // change dialog templates if necessary
                DialogExtraSubwindow* extraSubwindow = (DialogExtraSubwindow*)GetWindowLong(ctl,GWL_USERDATA);
                if (extraSubwindow && GetHandler().DialogTemplateID() != extraSubwindow->dialogTemplateID)
                {
                     ExtraDataList_RemoveData(TESDialog::GetDialogExtraList(dialog),extraSubwindow,true);
                     extraSubwindow = 0;
                }
                // create a new subwindow if necessary
                if (GetHandler().DialogTemplateID() && !extraSubwindow)
                {                    
                    TESDialog::Subwindow* subwindow = new TESDialog::Subwindow;               
                    // set subwindow position relative to parent dialog
                    subwindow->hDialog = dialog;
                    subwindow->hInstance = (HINSTANCE)hModule;
                    subwindow->hContainer = GetDlgItem(dialog,IDC_MGEF_HANDLERCONTAINER);
                    RECT rect;
                    GetWindowRect(subwindow->hContainer,&rect);
                    subwindow->position.x = rect.left;
                    subwindow->position.y = rect.top;
                    ScreenToClient(dialog,&subwindow->position); 
                    // build subwindow control list (copy controls from tab dialog template into parent dialog)
                    TESDialog::BuildSubwindow(GetHandler().DialogTemplateID(),subwindow);
                    // add subwindow extra data to parent
                    extraSubwindow = TESDialog::AddDialogExtraSubwindow(dialog,GetHandler().DialogTemplateID(),subwindow);
                    // initialize
                    GetHandler().InitializeDialog(dialog);
                    // hide controls if handler tab is nto selected
                    int cursel = TabCtrl_GetCurSel(GetDlgItem(dialog,IDC_MGEF_TABS));
                    if (cursel < 0 || kTabTemplateID[cursel] != IDD_MGEF_HANDLER)
                    {                    
                        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
                        {
                            ShowWindow(node->data, false);
                        }
                    }
                }
                // store current dialog template in handler selection window UserData
                SetWindowLong(ctl,GWL_USERDATA,(UInt32)extraSubwindow);
                // set handler in dialog
                GetHandler().SetInDialog(dialog);
                result = 0; return true;  // retval = false signals command handled
            }
            break;
        }
        }
        break;
    }
    case WM_NOTIFY:  
    {
        NMHDR* nmhdr = (NMHDR*)lParam;  // extract notify message info struct
        switch (nmhdr->idFrom)  // switch on control id
        {
        case IDC_MGEF_TABS: // main tab control
            if (nmhdr->code == TCN_SELCHANGING)
            {
                // current tab is about to be unselected
                _VMESSAGE("Tab hidden");
                ShowTabSubwindow(dialog,TabCtrl_GetCurSel(nmhdr->hwndFrom),false);
                result = 0; return true;   // retval = false to allow change
            }
            else if (nmhdr->code == TCN_SELCHANGE)
            {
                // current tab was just selected
                _VMESSAGE("Tab Shown");
                ShowTabSubwindow(dialog,TabCtrl_GetCurSel(nmhdr->hwndFrom),true);  
                return true;  // no retval
            }
            break;
        }
        break;
    }
    }
    // invoke handler msg callback
    if (GetHandler().DialogMessageCallback(dialog,uMsg,wParam,lParam,result)) return true;
    // invoke base class msg callback
    return ::EffectSetting::DialogMessageCallback(dialog,uMsg,wParam,lParam,result);
}
void EffectSetting::SetInDialog(HWND dialog)
{
    _VMESSAGE("Setting %s",GetDebugDescEx().c_str());
    if (!dialogHandle)
    {
        // first time initialization
        InitializeDialog(dialog);
        dialogHandle = dialog;
    }

    HWND ctl;
    char buffer[0x50];
    
    // GENERAL
    {
        // effect code box
        bool codeEditable = !IsMgefCodeVanilla(mgefCode) && // can't change vanilla effect codes 
                            (formFlags & TESForm::kFormFlags_FromActiveFile) > 0 && // code only editable if effect is in active file
                            (fileList.Empty() || // no override files (newly created effect) ...
                                (fileList.Count() == 1 && // .. or only a single override file, the active file
                                fileList.firstNode.data == TESDataHandler::dataHandler->fileManager.activeFile)
                             );
        ctl = GetDlgItem(dialog,IDC_MGEF_MGEFCODE);            
        sprintf_s(buffer,sizeof(buffer),"%08X", mgefCode);
        SetWindowText(ctl,buffer);
        EnableWindow(ctl,!IsMgefCodeDynamic(mgefCode) && codeEditable);
        // dynamic effect code checkbox
        CheckDlgButton(dialog,IDC_MGEF_DYNAMICMGEFCODE,IsMgefCodeDynamic(mgefCode));
        EnableWindow(GetDlgItem(dialog,IDC_MGEF_DYNAMICMGEFCODE),codeEditable);    
        // school
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_SCHOOL),(void*)school);
        // effect item description callback
        void* nameFormat = 0;  // TODO - get mgef member
        if (GetFlag(kMgefFlagShift_MagnitudeIsLevel)) nameFormat = (void*)kMgefFlagShift_MagnitudeIsLevel;
        if (GetFlag(kMgefFlagShift_MagnitudeIsFeet)) nameFormat = (void*)kMgefFlagShift_MagnitudeIsFeet;
        if (GetFlag(kMgefFlagShift_MagnitudeIsPercent)) nameFormat = (void*)kMgefFlagShift_MagnitudeIsPercent;
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK),nameFormat);
        // counter effects
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        TESListView::ClearItems(ctl);
        for (int i = 0; i < numCounters; i++)
        {
            void* countereff = EffectSetting::LookupByCode(counterArray[i]);
            if (countereff) TESListView::InsertItem(ctl,countereff);
            else _WARNING("Unrecognized counter effect '%4.4s' {%08X}",&counterArray[i],counterArray[i]);
        }
        ListView_SortItems(ctl,TESFormIDListView::FormListComparator,GetWindowLong(ctl,GWL_USERDATA)); // (re)sort items
        // counter effect add list
        TESComboBox::SetCurSel(GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES),0);
    }

    // DYNAMICS
    {
        // Cost
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_BASECOST,baseCost);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_DISPELFACTOR,0/*TODO*/);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_GETCOSTCALLBACK),0/*TODO*/);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_SETCOSTCALLBACK),0/*TODO*/);
        // Shielding        
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_PROVIDESSHIELDING),0/*TODO*/);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_DAMAGESSHIELDING),0/*TODO*/);
        // Resistance
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_RESISTVALUE),(void*)GetResistAV());
        // Hostility
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HOSTILITY),(void*)GetHostility());
        // Mag, Dur, Area, Range
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_MAGNITUDEEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_AREAEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_DURATIONEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_BARTERFACTOR,barterFactor);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_ENCHANTMENTFACTOR,enchantFactor);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_TARGETFACTOR,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_TOUCHFACTOR,0/*TODO*/);
        // Projectile
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEMULT,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEBASE,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJECTILESPEED,projSpeed);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_PROJECTILETYPE),(void*)GetProjectileType());
    }

    // FX
    {
        // Light, Hit + Enchant shaders
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_LIGHT),light);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_EFFECTSHADER),effectShader);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_ENCHANTSHADER),enchantShader);
        // Sounds
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_CASTINGSOUND),castingSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_BOLTSOUND),boltSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HITSOUND),hitSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_AREASOUND),areaSound);   
    }

    // HANDLER
    {
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HANDLER),(void*)GetHandler().HandlerCode()); 
        SendNotifyMessage(dialog, WM_USERCOMMAND,(CBN_SELCHANGE << 0x10) | IDC_MGEF_HANDLER,  // trigger CBN_SELCHANGED event
                                    (LPARAM)GetDlgItem(dialog,IDC_MGEF_HANDLER));
        GetHandler().SetInDialog(dialog);
    }

    // FLAGS
    for (int i = 0; i < 0x40; i++) CheckDlgButton(dialog,IDC_MGEF_FLAGEXBASE + i,GetFlag(i));

    // BASES
    ::TESFormIDListView::SetInDialog(dialog);
}
void EffectSetting::GetFromDialog(HWND dialog)
{
    _VMESSAGE("Getting %s",GetDebugDescEx().c_str());

    // call base method - retrieves most vanilla effectsetting fields, and calls GetFromDialog for components
    // will fail to retrieve AssocItem and Flags, as the control IDs for these fields have been changed
    UInt32 resistance = resistAV;
    ::EffectSetting::GetFromDialog(dialog);

    HWND ctl;
    char buffer[0x50];

    // GENERAL
    {
        // effect code
        GetWindowText(GetDlgItem(dialog,IDC_MGEF_MGEFCODE),buffer,sizeof(buffer));
        mgefCode = strtoul(buffer,0,16);  // TODO - validation?
        // effect item description callback
        UInt32 nameFormat = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK));
        SetFlag(kMgefFlagShift_MagnitudeIsPercent, nameFormat == kMgefFlagShift_MagnitudeIsPercent);
        SetFlag(kMgefFlagShift_MagnitudeIsLevel, nameFormat == kMgefFlagShift_MagnitudeIsLevel);
        SetFlag(kMgefFlagShift_MagnitudeIsFeet, nameFormat == kMgefFlagShift_MagnitudeIsFeet);
        // ... TODO - mgef member = (nameFormat > 0xFF) ? (DataT*)nameFormat : 0;
        // counter effects
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);        
        if (counterArray) {MemoryHeap::FormHeapFree(counterArray); counterArray = 0;} // clear current counter array
        if (numCounters = ListView_GetItemCount(ctl))
        {      
            // build array of effect codes
            counterArray = (UInt32*)MemoryHeap::FormHeapAlloc(numCounters * 4);
            gLog.Indent();
            for (int i = 0; i < numCounters; i++)
            {
                EffectSetting* mgef = (EffectSetting*)TESListView::GetItemData(ctl,i);
                counterArray[i] = mgef->mgefCode;
            }
            gLog.Outdent();
        }
    }

    // DYNAMICS
    {
        // Cost
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_DISPELFACTOR);
        /*TODO = */ TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_GETCOSTCALLBACK));
        /*TODO = */ TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_SETCOSTCALLBACK));
        // Shielding        
        /*TODO = */ TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_PROVIDESSHIELDING));
        /*TODO = */ TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_DAMAGESSHIELDING));
        // Hostility
        SetHostility((UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_HOSTILITY)));
        // Mag, Dur, Area, Range
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_MAGNITUDEEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_AREAEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_DURATIONEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_TARGETFACTOR);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_TOUCHFACTOR);
        // Projectile
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEMULT);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEBASE);      
    }

    // HANDLER
    {
        UInt32 ehCode = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_HANDLER));
        if (ehCode != GetHandler().HandlerCode()) SetHandler(MgefHandler::Create(ehCode,*this));          
        GetHandler().GetFromDialog(dialog);
    }

    // FLAGS
    for (int i = 0; i < 0x40; i++) {if (GetDlgItem(dialog,IDC_MGEF_FLAGEXBASE + i)) { SetFlag(i,IsDlgButtonChecked(dialog,IDC_MGEF_FLAGEXBASE + i)); }}

}
void EffectSetting::CleanupDialog(HWND dialog)
{
    _DMESSAGE("Cleanup MGEF Dialog");
    ::EffectSetting::CleanupDialog(dialog);
}
void EffectSetting::SetupFormListColumns(HWND listView)
{
    _DMESSAGE("Initializing columns for LV %p",listView);
    // call base function - clears columns & adds EditorID, FormID, & Name column
    ::EffectSetting::SetupFormListColumns(listView);
    // add EffectCode column
    TESListView::InsertColumn(listView,3,"Effect Code",80);
    // resize columns to squeeze 4th col in
    ListView_SetColumnWidth(listView,0,ListView_GetColumnWidth(listView,0)-30);
    ListView_SetColumnWidth(listView,2,ListView_GetColumnWidth(listView,2)-50);
}
void EffectSetting::GetFormListDispInfo(void* displayInfo)
{
    NMLVDISPINFO* info = (NMLVDISPINFO*)displayInfo;
    if (!info) return;
    EffectSetting* mgef = (EffectSetting*)TESListView::GetItemData(info->hdr.hwndFrom,info->item.iItem);
    if (!mgef) return;
    switch (info->item.iSubItem)
    {
    case 3:
        // mgefCode column - print mgefCode label
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%08X",mgef->mgefCode);
        return;
    default:
        // call base method - labels for editorID, formID, & name
        ::EffectSetting::GetFormListDispInfo(displayInfo);
    }
}
int EffectSetting::CompareInFormList(const TESForm& compareTo, int compareBy)
{
    EffectSetting* mgef = (EffectSetting*)&compareTo;
    switch (compareBy)
    {
    case 4:  
    case -4:  
        // mgefCode column - compare mgef Codes
        if (mgefCode == mgef->mgefCode) return 0;
        if (mgefCode < mgef->mgefCode) return -1;
        if (mgefCode > mgef->mgefCode) return 1;
    default:         
        // call base method - compare editorID, formID, & name
        return ::EffectSetting::CompareInFormList(compareTo,compareBy);
    }
}
#endif
// methods: debugging
BSStringT EffectSetting::GetDebugDescEx()
{
    // potentially inefficient, but worth it for the ease of use
    BSStringT desc;
    GetDebugDescription(desc);
    return desc;
}
// conversion
UInt8 EffectSetting::GetDefaultHostility(UInt32 mgefCode)
{
    if (!IsMgefCodeVanilla(mgefCode)) return Magic::kHostility_Neutral; // default hostility

    mgefCode = Swap32(mgefCode);

    // compare first character
    switch (mgefCode >> 0x18)
    {
    case 'A': // absorb
        return Magic::kHostility_Hostile;
    case 'B': // bound weapon, bound armor, burden
        if (mgefCode == 'BRDN') return Magic::kHostility_Hostile;
        else 
    case 'R': // rally, restore, resist, reflect & reanimate
        if (mgefCode == 'RALY') return Magic::kHostility_Neutral;
        else if (mgefCode == 'REAN') return Magic::kHostility_Neutral;
        else return Magic::kHostility_Beneficial;
    case 'I': // invisibility
    case 'M': // mythic dawn summons
    case 'N': // night eye
    case 'Z': // summon actor
        return Magic::kHostility_Beneficial;
    }

    // compare first two characters
    switch (mgefCode >> 0x10)
    {
    case 'CU': // cure
    case 'FO': // fortify
    case 'WA': // water breathing, water walking
        return Magic::kHostility_Beneficial;
    case 'DG': // damage
    case 'DR': // drain
    case 'WK': // weakness
        return Magic::kHostility_Hostile;
    }

    // compare all characters
    switch (mgefCode)
    {
    case 'BRDN': // burden        
    case 'DIAR': // disintegrate armor
    case 'DIWE': // disintegrate weapon
    case 'DUMY': // mehrunes degon effect
    case 'FIDG': // fire damage
    case 'FRDG': // frost damage
    case 'PARA': // paralysis
    case 'SHDG': // shock damage
    case 'SLNC': // silence
    case 'STRP': // soul trap
    case 'TURN': // turn undead
        return Magic::kHostility_Hostile;
    case 'CHML': // chameleon
    case 'DTCT': // detect life        
    case 'FISH': // fire shield        
    case 'FRSH': // frost shield
    case 'FTHR': // feather
    case 'LISH': // shock shield        
    case 'SABS': // spell absorption        
    case 'SHLD': // shield
        return Magic::kHostility_Beneficial;
    }
    
    return Magic::kHostility_Neutral; // all remaining effects should be neutral
}
bool EffectSetting::RequiresObmeMgefChunks()
{
    // define return codes - must evaluate to boolean
    enum
    {
        kOBMENotRequired        = 0,
        kReqOBME_General        = 1,
        kReqOBME_MgefCode,
        kReqOBME_School,
        kReqOBME_ResistAV,
        kReqOBME_EditorID,
        kReqOBME_CounterEffects,
        kReqOBME_ObmeFlags, // except 'Beneficial'
        kReqOBME_BeneficialFlag,
        kReqOBME_Handler,
    };

    // mgefCode - must be vanilla
    if (!IsMgefCodeVanilla(mgefCode)) return BoolEx(kReqOBME_MgefCode);

    // school - must be a vanilla magic school
    if (school > Magic::kSchool__MAX) return BoolEx(kReqOBME_School);

    // resistanceAV - must be a vanilla AV
    if (GetResistAV() > ActorValues::kActorVal__MAX && GetResistAV() != ActorValues::kActorVal__UBOUND) return BoolEx(kReqOBME_ResistAV);
    
    // editorID - must be the mgefCode reinterpreted as a string
    char codestr[5] = {{0}};
    *(UInt32*)codestr = mgefCode;  
    if (GetEditorID() == 0 || strcmp(GetEditorID(),codestr) != 0) return BoolEx(kReqOBME_EditorID);

    // counter array - can contain no non-vanilla effects
    for (int i = 0; counterArray && i < numCounters; i++)
    {
        if (!IsMgefCodeVanilla(counterArray[i])) return BoolEx(kReqOBME_CounterEffects);
    }
    // OBME flags (all except beneficial must be zero)
    if (mgefObmeFlags & ~kMgefObmeFlag_Beneficial) return BoolEx(kReqOBME_ObmeFlags);

    // beneficial flag - must be zero unless default hostility is beneficial
    if ((mgefObmeFlags & kMgefObmeFlag_Beneficial) && GetDefaultHostility(mgefCode) != Magic::kHostility_Beneficial) return BoolEx(kReqOBME_BeneficialFlag);

    // Effect Handler - must differ from default handler object
    UInt32 defHandlerCode = EffectHandler::GetDefaultHandlerCode(mgefCode);
    if (defHandlerCode != GetHandler().HandlerCode()) return BoolEx(kReqOBME_Handler);
    MgefHandler* defHandler = MgefHandler::Create(defHandlerCode,*this);
    bool handlerNoMatch = defHandler->CompareTo(GetHandler());
    delete defHandler;
    if (handlerNoMatch) return BoolEx(kReqOBME_Handler);

    return BoolEx(kOBMENotRequired);
}
// fields
inline bool EffectSetting::GetFlag(UInt32 flagShift) const
{
    if (flagShift < 0x20) return (mgefFlags & (1<<flagShift));
    flagShift -= 0x20;
    if (flagShift <= 0x20) return (mgefObmeFlags & (1<<flagShift));
    return false; // invalid flag shift
}
inline void EffectSetting::SetFlag(UInt32 flagShift, bool newval)
{
    if (flagShift < 0x20) 
    {
        if (newval) mgefFlags |= (1<<flagShift);
        else mgefFlags &= ~(1<<flagShift);
        return;
    }
    flagShift -= 0x20;
    if (flagShift <= 0x20) 
    {
        if (newval) mgefObmeFlags |= (1<<flagShift);
        else mgefObmeFlags &= ~(1<<flagShift);
        return;
    }
    return; // invalid flag shift
}
inline UInt8 EffectSetting::GetHostility() const
{
    if (GetFlag(kMgefFlagShift_Hostile)) return Magic::kHostility_Hostile;
    else if (GetFlag(kMgefFlagShift_Beneficial)) return Magic::kHostility_Beneficial;
    else return Magic::kHostility_Neutral;
}
inline void EffectSetting::SetHostility(UInt8 newval)
{
    switch(newval)
    {
    case Magic::kHostility_Hostile:
        SetFlag(kMgefFlagShift_Hostile,true);
        SetFlag(kMgefFlagShift_Beneficial,false);
        break;
    case Magic::kHostility_Beneficial:
        SetFlag(kMgefFlagShift_Hostile,false);
        SetFlag(kMgefFlagShift_Beneficial,true);
        break;
    default:
        SetFlag(kMgefFlagShift_Hostile,false);
        SetFlag(kMgefFlagShift_Beneficial,false);
        break;
    }
}
inline UInt32 EffectSetting::GetResistAV()
{
    if (resistAV == ActorValues::kActorVal__NONE || resistAV == ActorValues::kActorVal__MAX) return ActorValues::kActorVal__UBOUND;
    else return resistAV;
}
inline void EffectSetting::SetResistAV(UInt32 newResistAV)
{
    if (newResistAV == ActorValues::kActorVal__NONE || newResistAV == ActorValues::kActorVal__MAX) resistAV = ActorValues::kActorVal__UBOUND;
    else resistAV = newResistAV;
}
void EffectSetting::SetProjectileType(UInt32 newType)
{
    mgefFlags &= ~(kMgefFlag_ProjectileTypeLow | kMgefFlag_ProjectileTypeHigh); // clear current projectile type
    switch (newType)
    {
    case Magic::kProjType_Fog:
        mgefFlags |= kMgefFlag_ProjectileTypeHigh;
    case Magic::kProjType_Spray:
        mgefFlags |= kMgefFlag_ProjectileTypeLow;
        break;
    case Magic::kProjType_Bolt:
        mgefFlags |= kMgefFlag_ProjectileTypeHigh;
    default:
        break;
    }    
}
// cached vanilla effects
EffectSetting* EffectSetting::defaultVFXEffect     = 0;
EffectSetting* EffectSetting::diseaseVFXEffect     = 0;
EffectSetting* EffectSetting::poisonVFXEffect      = 0;
// effect codes
class VanillaEffectsSet : public std::set<UInt32>
{
public:
    VanillaEffectsSet()
    {
        // don't put output statements here, as the output log may not yet be initialized
        insert(Swap32('ABAT'));
        insert(Swap32('ABFA'));
        insert(Swap32('ABHE'));
        insert(Swap32('ABSK'));
        insert(Swap32('ABSP'));
        insert(Swap32('BA01'));
        insert(Swap32('BA02'));
        insert(Swap32('BA03'));
        insert(Swap32('BA04'));
        insert(Swap32('BA05'));
        insert(Swap32('BA06'));
        insert(Swap32('BA07'));
        insert(Swap32('BA08'));
        insert(Swap32('BA09'));
        insert(Swap32('BA10'));
        insert(Swap32('BABO'));
        insert(Swap32('BACU'));
        insert(Swap32('BAGA'));
        insert(Swap32('BAGR'));
        insert(Swap32('BAHE'));
        insert(Swap32('BASH'));
        insert(Swap32('BRDN'));
        insert(Swap32('BW01'));
        insert(Swap32('BW02'));
        insert(Swap32('BW03'));
        insert(Swap32('BW04'));
        insert(Swap32('BW05'));
        insert(Swap32('BW06'));
        insert(Swap32('BW07'));
        insert(Swap32('BW08'));
        insert(Swap32('BW09'));
        insert(Swap32('BW10'));
        insert(Swap32('BWAX'));
        insert(Swap32('BWBO'));
        insert(Swap32('BWDA'));
        insert(Swap32('BWMA'));
        insert(Swap32('BWSW'));
        insert(Swap32('CALM'));
        insert(Swap32('CHML'));
        insert(Swap32('CHRM'));
        insert(Swap32('COCR'));
        insert(Swap32('COHU'));
        insert(Swap32('CUDI'));
        insert(Swap32('CUPA'));
        insert(Swap32('CUPO'));
        insert(Swap32('DARK'));
        insert(Swap32('DEMO'));
        insert(Swap32('DGAT'));
        insert(Swap32('DGFA'));
        insert(Swap32('DGHE'));
        insert(Swap32('DGSP'));
        insert(Swap32('DIAR'));
        insert(Swap32('DISE'));
        insert(Swap32('DIWE'));
        insert(Swap32('DRAT'));
        insert(Swap32('DRFA'));
        insert(Swap32('DRHE'));
        insert(Swap32('DRSK'));
        insert(Swap32('DRSP'));
        insert(Swap32('DSPL'));
        insert(Swap32('DTCT'));
        insert(Swap32('DUMY'));
        insert(Swap32('FIDG'));
        insert(Swap32('FISH'));
        insert(Swap32('FOAT'));
        insert(Swap32('FOFA'));
        insert(Swap32('FOHE'));
        insert(Swap32('FOMM'));
        insert(Swap32('FOSK'));
        insert(Swap32('FOSP'));
        insert(Swap32('FRDG'));
        insert(Swap32('FRNZ'));
        insert(Swap32('FRSH'));
        insert(Swap32('FTHR'));
        insert(Swap32('INVI'));
        insert(Swap32('LGHT'));
        insert(Swap32('LISH'));
        insert(Swap32('LOCK'));
        insert(Swap32('MYHL'));
        insert(Swap32('MYTH'));
        insert(Swap32('NEYE'));
        insert(Swap32('OPEN'));
        insert(Swap32('PARA'));
        insert(Swap32('POSN'));
        insert(Swap32('RALY'));
        insert(Swap32('REAN'));
        insert(Swap32('REAT'));
        insert(Swap32('REDG'));
        insert(Swap32('REFA'));
        insert(Swap32('REHE'));
        insert(Swap32('RESP'));
        insert(Swap32('RFLC'));
        insert(Swap32('RSDI'));
        insert(Swap32('RSFI'));
        insert(Swap32('RSFR'));
        insert(Swap32('RSMA'));
        insert(Swap32('RSNW'));
        insert(Swap32('RSPA'));
        insert(Swap32('RSPO'));
        insert(Swap32('RSSH'));
        insert(Swap32('RSWD'));
        insert(Swap32('SABS'));
        insert(Swap32('SEFF'));
        insert(Swap32('SHDG'));
        insert(Swap32('SHLD'));
        insert(Swap32('SLNC'));
        insert(Swap32('STMA'));
        insert(Swap32('STRP'));
        insert(Swap32('SUDG'));
        insert(Swap32('TELE'));
        insert(Swap32('TURN'));
        insert(Swap32('VAMP'));
        insert(Swap32('WABR'));
        insert(Swap32('WAWA'));
        insert(Swap32('WKDI'));
        insert(Swap32('WKFI'));
        insert(Swap32('WKFR'));
        insert(Swap32('WKMA'));
        insert(Swap32('WKNW'));
        insert(Swap32('WKPO'));
        insert(Swap32('WKSH'));
        insert(Swap32('Z001'));
        insert(Swap32('Z002'));
        insert(Swap32('Z003'));
        insert(Swap32('Z004'));
        insert(Swap32('Z005'));
        insert(Swap32('Z006'));
        insert(Swap32('Z007'));
        insert(Swap32('Z008'));
        insert(Swap32('Z009'));
        insert(Swap32('Z010'));
        insert(Swap32('Z011'));
        insert(Swap32('Z012'));
        insert(Swap32('Z013'));
        insert(Swap32('Z014'));
        insert(Swap32('Z015'));
        insert(Swap32('Z016'));
        insert(Swap32('Z017'));
        insert(Swap32('Z018'));
        insert(Swap32('Z019'));
        insert(Swap32('Z020'));
        insert(Swap32('ZCLA'));
        insert(Swap32('ZDAE'));
        insert(Swap32('ZDRE'));
        insert(Swap32('ZDRL'));
        insert(Swap32('ZFIA'));
        insert(Swap32('ZFRA'));
        insert(Swap32('ZGHO'));
        insert(Swap32('ZHDZ'));
        insert(Swap32('ZLIC'));
        insert(Swap32('ZSCA'));
        insert(Swap32('ZSKA'));
        insert(Swap32('ZSKC'));
        insert(Swap32('ZSKE'));
        insert(Swap32('ZSKH'));
        insert(Swap32('ZSPD'));
        insert(Swap32('ZSTA'));
        insert(Swap32('ZWRA'));
        insert(Swap32('ZWRL'));
        insert(Swap32('ZXIV'));
        insert(Swap32('ZZOM'));
    }
} vanillaEffectsSet;
bool EffectSetting::IsMgefCodeVanilla(UInt32 mgefCode)
{
    VanillaEffectsSet::iterator it = vanillaEffectsSet.find(mgefCode);
    if (vanillaEffectsSet.find(mgefCode) != vanillaEffectsSet.end()) return true;
    return false;
}
bool EffectSetting::IsMgefCodeValid(UInt32 mgefCode)
{
    return (mgefCode != kMgefCode_None && mgefCode != kMgefCode_Invalid);
}
bool EffectSetting::IsMgefCodeDynamic(UInt32 mgefCode)
{
    return (mgefCode & kMgefCode_DynamicFlag);
}
UInt32 EffectSetting::GetUnusedDynamicCode()
{
    // step up through dynamic code space & return first unused code
    static UInt32 rawcode = 0;
    // mask is current active file index in lowest word | dynamic bit
    UInt32 mask = kMgefCode_DynamicFlag | (TESDataHandler::dataHandler ? (TESDataHandler::dataHandler->fileManager.nextFormID >> 0x18) : 0xFF);
    // outer loop for wrapping back to beginning of range (once only)
    for (int cycle = 0; cycle < 2; cycle++)
    {
        while(rawcode + 1 < kMgefCode_DynamicFlag) 
        {
            // check if code is registered
            if (!LookupByCode(mask | rawcode)) return (mask | rawcode);
            // next code value
            rawcode += 0x00000100;
        }
        // wrap back to start of range
        rawcode = 0;
    }
    return kMgefCode_Invalid;
}
UInt32 EffectSetting::GetUnusedStaticCode()
{
    // step up through dynamic code space & return first unused code
    static UInt32 rawcode = 1;
    // outer loop for wrapping back to beginning of range (once only)
    for (int cycle = 0; cycle < 2; cycle++)
    {
        while(rawcode < kMgefCode_DynamicFlag) 
        {
            // check if code is registered
            if (!LookupByCode(rawcode)) return rawcode;
            // next code value
            rawcode++;
        }
        // wrap back to start of range
        rawcode = 1;
    }
    return kMgefCode_None;
}
// effect handler
MgefHandler& EffectSetting::GetHandler()
{    
    if (!effectHandler)
    {
        // this effect has no handler, create a default handler
        effectHandler = MgefHandler::Create(EffectHandler::GetDefaultHandlerCode(mgefCode),*this);
        if (!effectHandler) effectHandler = MgefHandler::Create('VTCA',*this);  // default handler not yet implemented - should never happen in release version
    }
    return *effectHandler;
}
void EffectSetting::SetHandler(MgefHandler* handler)
{
    if (effectHandler) delete effectHandler;    // clear current handler
    effectHandler = handler;
}
// constructor, destructor
EffectSetting::EffectSetting()
{    
    _VMESSAGE("Constructing Mgef <%p>",this);    

    if (static bool runonce = true)
    {
        // patch up Game/CS EffectSetting vtbl with methods overwritten by OBME  
        memaddr thisvtbl = (UInt32)memaddr::GetObjectVtbl(this);
        _MESSAGE("Patching Game/CS TESFormIDListView vtbl from <%p>",thisvtbl);
        gLog.Indent();
        for (int i = 0; i < sizeof(TESForm_PatchMethods)*0x8; i++)
        {
            if ((TESForm_PatchMethods[i/0x20] >> (i%0x20)) & 1)
            {
                TESForm_Vtbl.SetVtblEntry(i*4,thisvtbl.GetVtblEntry(i*4));
                _VMESSAGE("Patched Offset 0x%04X",i*4);
            }
        }
        gLog.Outdent();
        runonce  =  false;
    }

    // replace compiler-generated vtbls with existing game/CS vtbls
    TESForm_Vtbl.SetObjectVtbl(this);    
    TESModel_Vtbl.SetObjectVtbl(dynamic_cast<TESModel*>(this));
    TESFullName_Vtbl.SetObjectVtbl(dynamic_cast<TESFullName*>(this));
    TESDescription_Vtbl.SetObjectVtbl(dynamic_cast<TESDescription*>(this));
    TESIcon_Vtbl.SetObjectVtbl(dynamic_cast<TESIcon*>(this));

    // game constructor uses some odd initialization values
    // NOTE: be *very* careful about changing default initializations
    // the game occassionally uses a default-constructed EffectSetting, and the default values are
    // hardcoded all over the place.  In particular,  a default-created effect is used as a comparison 
    // template for filtering effects before spellmaking, enchanting, etc.  Do NOT change the mgefFlags 
    // initialization from zero, or the default school from 6, as this will cause some effects to be
    // incorrectly filtered out.
    resistAV = ActorValues::kActorVal__UBOUND; // standardize the 'No Resistance' value to 0xFFFFFFFF, which is actually used in Resistance system
    mgefCode = kMgefCode_None;
    // initialize new fields
    effectHandler = 0;
    mgefObmeFlags = 0x0;
}
EffectSetting::~EffectSetting()
{
    _VMESSAGE("Destructing Mgef <%p>",this);
    if (effectHandler) delete effectHandler;
}

// hooks
EffectSetting* EffectSetting_Create_Hndl()
{
    _VMESSAGE("Creating Mgef ...");
    return new EffectSetting;
}
EffectSetting* EffectSettingCollection_Add_Hndl(UInt32 mgefCode, const char* name, UInt32 school, float baseCost, UInt32 param, 
                                                UInt32 mgefFlags, UInt32 resistAV, UInt16 numCounters, UInt32 firstCounter)       
{   
    /* 
        This function doesn't perform integrity checks on the mgefFlags like the vanilla code
        It also assumes - requires - that the provided effect code is not currently in use.
    */
    // create new effect
    EffectSetting* mgef = new EffectSetting; 
    mgef->mgefCode = mgefCode;
    EffectSettingCollection::collection.SetAt(mgefCode,mgef);  // add new effect to table
    // set editor name
    char namebuffer[5];
    *(UInt32*)namebuffer = mgefCode;
    namebuffer[4] = 0;
    mgef->SetEditorID(namebuffer);
    // initialize with provided parameters
    mgef->name = name;
    mgef->school = school;
    mgef->baseCost = baseCost;
    mgef->mgefParam = param;
    mgef->mgefFlags = mgefFlags;    
    mgef->SetResistAV(resistAV); // standardize 'invalid' avCodes
    mgef->SetCounterEffects(numCounters,&firstCounter);
    mgef->numCounters = numCounters; // SetCounterEffects is bugged, doesn't set numCounters properly
    // get default hostility
    mgef->SetHostility(EffectSetting::GetDefaultHostility(mgefCode));
    // done
    _DMESSAGE("Added to Collection: %s w/ host %i, handler '%s'", mgef->GetDebugDescEx().c_str(), mgef->GetHostility(),mgef->GetHandler().HandlerName());
    return mgef;
}
void _declspec(naked) TESForm_CreateMgef_Hndl(void)
{    
    __asm
    {   
        // hook prolog
        push    ecx
        // create new effect
        call    EffectSetting_Create_Hndl
        // overwritten code
        pop     ecx
        jmp     [TESForm_CreateMgef_Retn._addr]
    }
}
void _declspec(naked) TESDataHandler_AddForm_JumpHndl(void)
{
    EffectSetting* mgef;
    __asm
    {
        // prolog
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     mgef, esi
    }
    if (!EffectSetting::IsMgefCodeValid(mgef->mgefCode))
    {
        // mgef needs to be given a valid code
        // happens when a new mgef is created & added to the handler without being given a valid code first
        mgef->mgefCode = EffectSetting::GetUnusedDynamicCode();
    }
    _DMESSAGE("Adding new MGEF {%08X}",mgef->mgefCode);
    EffectSettingCollection::collection.SetAt(mgef->mgefCode,mgef);
    __asm
    {
        // epilog
        mov     esp, ebp
        popad
        // overwritten code
        pop     ebx
        pop     edi
        mov     al, 1
        pop     esi
        retn    4
    }
}
void _declspec(naked) TESDataHandler_LoadMgef_Hndl(void)
{
    TESForm*        form;
    TESFile*        file;
    UInt32          result;
    __asm
    {   
        // prolog
        push    eax
        pushad
        push    ebp
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        // grab params
        mov     form, esi
        mov     file, edi  
    }
    result = false;
    if (file && file->GetChunkType() == Swap32('EDID'))
    {
        // load effect code from EDID chunk
        char codestr[10];
        file->GetChunkData(codestr,sizeof(codestr));
        UInt32 mgefCode = *(UInt32*)codestr; // reinterpret as integer
        // create/load effect record
        if (EffectSetting::IsMgefCodeValid(mgefCode))
        {
            // resolve code
            TESFileFormats::ResolveModValue(mgefCode,*file,TESFileFormats::kResolutionType_MgefCode);
            // search for already created effect with this code
            EffectSetting* mgef = dynamic_cast<EffectSetting*>(form);
            if (mgef && mgef->mgefCode == mgefCode)
            {
                // effect w/ same formid & mgefcode was found by datahandler
            }
            else 
            {
                if (form)
                {
                    // formid belongs to an existing object that is not MGEF or has a different effect code
                    _ERROR("Cannot load MGEF record into existing %s form '%s' (%08X)",
                        TESForm::GetFormTypeName(form->GetFormType()),form->GetEditorID(),form->formID);
                }
                // lookup in table
                mgef = (EffectSetting*)EffectSettingCollection::LookupByCode(mgefCode);
                if (!mgef)
                {
                    // effect does not yet exist, create
                    mgef = EffectSetting_Create_Hndl();
                    mgef->mgefCode = mgefCode;
                    EffectSettingCollection::collection.SetAt(mgefCode,mgef);  // add new effect to table
                }
            }
             // load effect data   
            result = TESDataHandler::LoadForm(*mgef,*file);
        }
        else
        {
            // bad raw effect code
            _ERROR("Invalid effect code {%08X}",mgefCode);
        }
    } 
    else if (file) 
    {
        // bad leading chunk type
        UInt32 chunktype = file->GetChunkType();
        _ERROR("Unexpected chunk '%4.4s' {%08X} w/ size %08X.  MGEF record must begin with an EDID chunk.",&chunktype,chunktype,file->currentChunk.chunkLength);
    }
    __asm
    {
        mov     eax, result
        mov     [ebp + 0x24], eax
        // hook epilog
        mov     esp, ebp
        pop     ebp
        popad
        // overwritten code
        pop     eax
        jmp     [TESDataHandler_LoadMgef_Retn._addr]
    }
}
void UpdateEffectItems(TESForm* form)
{
    // iterate through effect items & update EffectSetting* to match mgefCode
    if (!form) return;
    EffectItemList* efflist = dynamic_cast<EffectItemList*>(form);
    if (!efflist) return;
    for (EffectItemList::Node* effnode = &efflist->firstNode; effnode; effnode = effnode->next)
    {
        if (!effnode->data) continue;
        effnode->data->effect = EffectSetting::LookupByCode(effnode->data->mgefCode);
    }
}
void CacheVanillaEffects()
{
    _VMESSAGE("Caching Meta Effects ...");
    // cache pointers to meta effects
    EffectSetting::defaultVFXEffect     = (EffectSetting*)EffectSettingCollection::LookupByCode(Swap32('SEFF'));
    EffectSetting::diseaseVFXEffect     = (EffectSetting*)EffectSettingCollection::LookupByCode(Swap32('DISE'));
    EffectSetting::poisonVFXEffect      = (EffectSetting*)EffectSettingCollection::LookupByCode(Swap32('POSN'));
}
void EffectSetting::Initialize()
{
    _MESSAGE("Initializing ...");

    // In the game, there are 4 functions that allocate new EffectSettings:
    //  EffectSetting::Create
    //  EffectSettingCollection::Add
    //  TESForm::Create
    //  TESDataHandler::LoadObject
    // We must patch these functions to allocate OBME::EffectSetting instead. In the editor, there are 
    // only 3 - EffectSetting::Create is missing.  Also, in the editor EffectItemList has an EffectSetting 
    // member for filtering, which should be left as an ordinary ::EffectSetting
    EffectSetting_Create.WriteRelJump(&EffectSetting_Create_Hndl);
    EffectSettingCollection_Add_Hook.WriteRelJump(&EffectSettingCollection_Add_Hndl);
    TESForm_CreateMgef_Hook.WriteRelJump(&TESForm_CreateMgef_Hndl);
    TESDataHandler_LoadMgef_Hook.WriteRelJump(&TESDataHandler_LoadMgef_Hndl);

    // patch switch table in TESDataHandler::AddForm to add effectsettings to the global mgef map
    TESDataHandler_AddForm_LookupPatch.WriteData8((TESDataHandler_AddForm_JumpPatch - TESDataHandler_AddForm_JumpTable)/4);
    TESDataHandler_AddForm_JumpPatch.WriteData32((UInt32)&TESDataHandler_AddForm_JumpHndl);

    // hook creation of vanilla magic effects, so we can cache them
    EffectSettingCollection_AddLast_Hook.WriteRelJump(&CacheVanillaEffects);

    // TODO - hook loading of player's known effect list to resolve effect codes

    // patch creation of MGEF dialog in CS - replace CS module instance with local module instance
    TESCS_CreateMgefDialog_Patch.WriteData32((UInt32)&hModule);

    // check if effects were created before OBSE patched in; they need to be destroyed & remade as OBME::EffectSettings 
    if (EffectSettingCollection::collection.GetCount() > 0)
    {
        _VMESSAGE("Found magic effects table already filled - resetting ...");
        // prevent TESForm::TESForm from assigning formIDs by temporarily clearing global data handler
        TESDataHandler* dataHandler = TESDataHandler::dataHandler;
        TESDataHandler::dataHandler = 0;
        // reset effect setting collection
        EffectSettingCollection::Reset();
        // restore the global data handler
        TESDataHandler::dataHandler = dataHandler;
        // update the EffectSetting* on any magic items already created        
        if (dataHandler)
        {
            for(BSSimpleList<SpellItem*>::Node* node = &dataHandler->spellItems.firstNode; node; node = node->next)
            {
                if (!node->data) continue;
                UpdateEffectItems(node->data);
            }
            for(BSSimpleList<EnchantmentItem*>::Node* node = &dataHandler->enchantmentItems.firstNode; node; node = node->next)
            {
                if (!node->data) continue;
                UpdateEffectItems(node->data);
            }
            if (dataHandler->objects)
            {
                for(TESObject* object = dataHandler->objects->first; object; object = object->next)
                {
                    UpdateEffectItems(object);
                }
            }
        } 
    }
}

};  // end namespace OBME