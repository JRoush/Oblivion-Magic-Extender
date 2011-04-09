#include "OBME/EffectSetting.h"
#include "OBME/EffectSetting.rc.h"  // mgefCode change prompt
#include "OBME/OBME_version.h"
#include "OBME/Magic.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/EffectItem.h" // TODO - replace with OBME/EffectItem.h
#include "OBME/EffectHandlers/ValueModifierEffect.h"   
#include "OBME/EffectHandlers/SummonCreatureEffect.h" 
#include "OBME/EffectHandlers/BoundItemEffect.h"
#include "OBME/EffectHandlers/ScriptEffect.h"
#include "OBME/EffectHandlers/DispelEffect.h"

#include "API/TES/TESDataHandler.h"
#include "API/TESFiles/TESFile.h"
#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"
#include "Components/FormRefCounter.h"

#include <map>

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
    MEMBER /*14*/ UInt32        costCallback;       // cost callback script formID
    MEMBER /*18*/ float         dispelFactor; 
    MEMBER /*1C*/ UInt32        reserved[5];
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

// virtual methods - TESFormIDListView
void ConvertV1HandlerData(EffectSetting& mgef, TESFile& file, UInt32 version, UInt32 hParamA, UInt32 hParamB, UInt32 hParamFlags, UInt32 hUsesFlags)
{   
    /* Converts handler information for the four altered handlers MDAV, SMAC/SMBO, SEFF, DSPL . Needs to manage 
        -   mgefParam (for MDAV/SMAC/SMBO)
        -   Uses* mgefFlags (for MDAV/SMBO/SMAC)
        -   Detrimental flag (for MDAV)
        -   MagicGroups (for SMAC/SMBO)
        -   any relevant handler members
    */
    if (version == VERSION_VANILLA_OBLIVION || version >= MAKE_VERSION(2,0,0,0)) return; // this function is for v1 record only

    // MDAV
    if (ValueModifierMgefHandler* mdav = dynamic_cast<ValueModifierMgefHandler*>(&mgef.GetHandler()))
    {
        if (version < MAKE_VERSION(1,0,4,0)) // v1.beta1 - v1.beta3
        {            
            // Actor Value
            TESFileFormats::ResolveModValue(hParamA,file,TESFileFormats::kResType_ActorValue);            
            // UseSkill|UseAttribute|UseAV flags
            if (hParamA && hParamA != ActorValues::kActorVal_Armorer && hParamA != ActorValues::kActorVal__NONE) // AV is not Strength, Armorer, or Invalid
            {
                mgef.mgefParam = hParamA;    // a particular AV was specified, use it
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseActorValue,true);
            }
            else if (hUsesFlags & EffectSetting::kMgefFlag_UseAttribute) 
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseAttribute,true);
            else if (hUsesFlags & EffectSetting::kMgefFlag_UseSkill) 
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseSkill,true);
            else 
            {
                mgef.mgefParam = hParamA;   // no good guess as to the original intent, default to specific AV
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseActorValue,true); 
            }
            // detrimental
            mgef.SetFlag(EffectSetting::kMgefFlagShift_Detrimental, hParamFlags & EffectSetting::kMgefFlag_Detrimental);
            // AV Part - use default AV parts, as v1.b1 had no override for this and v1.beta2-v1.beta3 use conflicting formats
            if (mgef.GetFlag(EffectSetting::kMgefFlagShift_Recovers)) mdav->avPart = ValueModifierMgefHandler::kAVPart_MaxSpecial;
            else mdav->avPart = ValueModifierMgefHandler::kAVPart_Damage;
            // issue ambiguity warning
            if (version >= MAKE_VERSION(1,0,2,0))
            {
                _WARNING("%s was saved with OBME v1.beta2 OR v1.beta3.  Because of the ambiguity, OBME is unabled to "
                "properly load the following data specific to the ValueModifier handler: AV Part, secondary AV selections", mgef.GetDebugDescEx());
            }
        }
        else // v1.beta4 - v1.0
        {
            TESFileFormats::ResolveModValue(hParamA,file,TESFileFormats::kResType_ActorValue); // 'low'/default AV
            TESFileFormats::ResolveModValue(hParamB,file,TESFileFormats::kResType_ActorValue); // 'high' AV
            // Uses Flags, AV code
            if (hParamA == ActorValues::kActorVal_Strength && hParamB == ActorValues::kActorVal_Luck) 
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseAttribute,true);
            else if (hParamA == ActorValues::kActorVal_Armorer && hParamB == ActorValues::kActorVal_Speechcraft) 
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseSkill,true);
            else 
            {
                mgef.mgefParam = hParamA;
                mgef.SetFlag(EffectSetting::kMgefFlagShift_UseActorValue,true);
                // issue warning for discarding 'upper' AV
                if (hParamB != ActorValues::kActorVal__MAX) _WARNING("%s was saved using the v1.0 format.  OBME will discard the following "
                        "data specific to the ValueModifier handler: secondary AV selection", mgef.GetDebugDescEx());
            }
            // avPart - encoded in Param flags C & D
            mdav->avPart = (hParamFlags >> (EffectSetting::kMgefFlagShift__ParamFlagC-0x20)) & 0x3;
            if (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagB) 
            {
                if (mdav->avPart == ValueModifierMgefHandler::kAVPart_Max)  mdav->avPart = ValueModifierMgefHandler::kAVPart_MaxSpecial; // uses max special
                else _WARNING("%s was saved using the v1.0 format.  OBME will discard the following "
                        "data specific to the ValueModifier handler: 'Ability Special' flag for non-Max modifier", mgef.GetDebugDescEx());
            }
            // detrimental
            if (hParamFlags & EffectSetting::kMgefFlag_Detrimental) mgef.SetFlag(EffectSetting::kMgefFlagShift_Detrimental,true);
        } 
    }

    // SMAC/SMBO
    else if (dynamic_cast<AssociatedItemMgefHandler*>(&mgef.GetHandler()))
    {
        // summoned formID
        TESFileFormats::ResolveModValue(hParamA,file,TESFileFormats::kResType_FormID);
        mgef.mgefParam = hParamA;
        // uses vanilla limit
        bool useVanillaLim = (version < MAKE_VERSION(1,0,2,0)) ? (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagA) // v1.beta1
                                                                : (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagC); // v1.beta2-v1.0        
        // SMAC
        if (dynamic_cast<SummonCreatureMgefHandler*>(&mgef.GetHandler()))
        {            
            mgef.SetFlag(EffectSetting::kMgefFlagShift_UseActor,true); // set 'Use Actor' flag
            // limit
            if (useVanillaLim) mgef.MagicGroupList::AddMagicGroup((MagicGroup*)MagicGroup::g_SummonCreatureLimit->formID,1); // add vanilla limit
            else
            {
                mgef.MagicGroupList::RemoveMagicGroup((MagicGroup*)MagicGroup::g_SummonCreatureLimit->formID); // remove vanilla limit
                TESFileFormats::ResolveModValue(hParamB,file,TESFileFormats::kResType_FormID);  // resolve limit global formid
                mgef.MagicGroupList::AddMagicGroup((MagicGroup*)hParamB,1); // add custom limit           
                //  MagicGroup::LinkComponent() will resolve into a ptr to global & create a new group for the limit (or use an existing group that matches)
            }
        }   
        // SMBO
        else if (dynamic_cast<BoundItemMgefHandler*>(&mgef.GetHandler()))
        {
            UInt32 vanillaLim; SInt32 vanillaLimWeight;
            EffectSetting::GetDefaultMagicGroup(mgef.mgefCode,vanillaLim,vanillaLimWeight);
            if (useVanillaLim && vanillaLim && !hParamA)
            {
                // vanilla effect w/ empty summon field - use hardcoded vanilla limit to determine proper flags
                mgef.MagicGroupList::AddMagicGroup((MagicGroup*)vanillaLim,vanillaLimWeight); // add vanilla limit
                if (vanillaLim == MagicGroup::kFormID_BoundWeaponLimit) mgef.SetFlag(EffectSetting::kMgefFlagShift_UseWeapon, true);
                else mgef.SetFlag(EffectSetting::kMgefFlagShift_UseArmor, true);
            }
            else if (useVanillaLim)
            {
                // use vanilla limit - need to determine proper group based on summoned form, if any
                if (vanillaLim) mgef.MagicGroupList::RemoveMagicGroup((MagicGroup*)vanillaLim); // remove hardcoded vanilla limit
                mgef.MagicGroupList::AddMagicGroup((MagicGroup*)MagicGroup::kFormID_BoundWeaponLimit,1); // add bound weap lim as placeholder group
                // BoundItemMgefHandler::LinkHandler() will change vanilla limit group and/or set the UseWeapon/UseArmor based on the
                // associated item & default settings.
            }
            else 
            {
                if (vanillaLim) mgef.MagicGroupList::RemoveMagicGroup((MagicGroup*)vanillaLim); // remove hardcoded vanilla limit
                TESFileFormats::ResolveModValue(hParamB,file,TESFileFormats::kResType_FormID);  // resolve limit global formid
                mgef.MagicGroupList::AddMagicGroup((MagicGroup*)hParamB,1); // add custom limit           
                //  MagicGroup::LinkComponent() will resolve into a ptr to global & create a new group for the limit (or use an existing group that matches)
            }
        }
        // warning for 'UseMagnitudeScaling' flag
        if (hParamFlags & (version < MAKE_VERSION(1,0,2,0) ? EffectSetting::kMgefObmeFlag__ParamFlagB : EffectSetting::kMgefObmeFlag__ParamFlagA))
        {
            _WARNING("%s uses the 'Summoned Form is Scaled with Magnitude' flag, which will be discarded because is no longer supported."
                , mgef.GetDebugDescEx());
        }
    }

    // SEFF
    else if (ScriptMgefHandler* seff = dynamic_cast<ScriptMgefHandler*>(&mgef.GetHandler()))
    {
        // script formID
        TESFileFormats::ResolveModValue(hParamA,file,TESFileFormats::kResType_FormID);
        seff->scriptFormID = hParamA;
          // uses 'SEFF Always Applies' behavior
        seff->useSEFFAlwaysApplies = (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagA); 
        // determine EffectItem::ActorValue field resolution for compatibility
        if (version >= MAKE_VERSION(1,0,2,0))
        {
            if (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagC) seff->efitAVResType = TESFileFormats::kResType_FormID;
            else if (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagD) seff->efitAVResType = TESFileFormats::kResType_MgefCode;
            else seff->efitAVResType = TESFileFormats::kResType_None;
        }
    }  

    // DSPL
    else if (DispelMgefHandler* dspl = dynamic_cast<DispelMgefHandler*>(&mgef.GetHandler()))
    {
        if (version >= MAKE_VERSION(1,0,2,0))   // v1.beta2+
        {
            // mgefCode / ehCode
            if (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagB) dspl->ehCode = hParamA; // ehCode
            else
            {
                TESFileFormats::ResolveModValue(hParamA,file,TESFileFormats::kResType_MgefCode);
                dspl->mgefCode = hParamA; // mgefCode
            }
            // magic type & cast type
            dspl->magicTypes = (hParamB & 0x1F) | ((hParamB >> 2) & 0x180); // extract magic type from ParamB bitmask
            dspl->castTypes = (hParamB >> 5) & 0xF;
            // behavior flags
            dspl->atomicDispel = (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagA);
            dspl->distributeMag = (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagC);
            dspl->atomicItems = (hParamFlags & EffectSetting::kMgefObmeFlag__ParamFlagD);
        }
    }
}
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

    // unlink form - revert form references back to formIDs, mgefCodes, etc.
    UnlinkForm();

    // initialize form from record
    file.InitializeFormFromRecord(*this);

    // flags for indicating what chunks have been loaded
    enum EffectSettingChunks
    {
        kMgefChunk_Edid     = 0x00000001,
        kMgefChunk_Eddx     = 0x00000002, // v1.beta2+
        kMgefChunk_Full     = 0x00000004,
        kMgefChunk_Desc     = 0x00000008,
        kMgefChunk_Icon     = 0x00000010,
        kMgefChunk_Modl     = 0x00000020,
        kMgefChunk_Data     = 0x00000040,
        kMgefChunk_Datx     = 0x00000080, // v1.beta2 - v1.beta3
        kMgefChunk_Esce     = 0x00000100,
        kMgefChunk_Obme     = 0x00000200, // v1.beta4+
        kMgefChunk_Ehnd     = 0x00000400, // v2.beta1+
        kMgefChunk_Mgls     = 0x00000800, // v2.beta1+
        kMgefChunk_DataEx   = 0x00001000, // v1.beta1
    }; 
    UInt32 loadFlags = 0;    // no chunks loaded
    UInt32 loadVersion = VERSION_VANILLA_OBLIVION;

    // reserve storage for chunk data that needs version-specific processing
    EffectSettingObmeChunk  obme;
    EffectSettingDataChunk  data;
    EffectSettingDatxChunk  datx;
    MagicGroupList mgls;
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
                loadFlags |= kMgefChunk_Data | kMgefChunk_Datx | kMgefChunk_DataEx;
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

        case 'MGLS':
            if (~loadFlags & kMgefChunk_Data) _WARNING("Unexpected chunk 'MGLS' - must come after chunk 'DATA'");
            MagicGroupList::LoadComponent(mgls,file);
            // done
            _VMESSAGE("MGLS chunk");  
            loadFlags |= kMgefChunk_Mgls;
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
    else if (loadFlags & kMgefChunk_DataEx) loadVersion = MAKE_VERSION(1,0,1,0); // v1.beta1
    else if (loadFlags & kMgefChunk_Datx) loadVersion = VERSION_OBME_LASTUNVERSIONED;  // assume last "unversioned" version
    else loadVersion = VERSION_VANILLA_OBLIVION;
    _DMESSAGE("Final Record Version %08X",loadVersion);

    // editorID
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

    // process main data & auxiliary chunks
    if (loadFlags & kMgefChunk_Data)
    {
        // resolve simple references
        TESFileFormats::ResolveModValue(data.light,file,TESFileFormats::kResType_FormID); // Light
        TESFileFormats::ResolveModValue(data.effectShader,file,TESFileFormats::kResType_FormID); // EffectShader
        TESFileFormats::ResolveModValue(data.enchantShader,file,TESFileFormats::kResType_FormID); // EnchantmentShader
        TESFileFormats::ResolveModValue(data.castingSound,file,TESFileFormats::kResType_FormID); // Cast Sound
        TESFileFormats::ResolveModValue(data.boltSound,file,TESFileFormats::kResType_FormID); // Bolt Sound
        TESFileFormats::ResolveModValue(data.hitSound,file,TESFileFormats::kResType_FormID); // Hit Sound
        TESFileFormats::ResolveModValue(data.areaSound,file,TESFileFormats::kResType_FormID); // Area Sound
        TESFileFormats::ResolveModValue(data.resistAV,file,TESFileFormats::kResType_ActorValue); // Resistance AV (for AddActorValues)
        TESFileFormats::ResolveModValue(data.school,file,TESFileFormats::kResType_FormID); // School (for extended school types)
        // copy DATA onto mgef
        memcpy(&mgefFlags,&data,sizeof(data));
        SetResistAV(data.resistAV); // standardize resistance value
        // flags (mgef + obme)
        if (loadVersion == VERSION_VANILLA_OBLIVION) 
        {            
            mgefFlags = (data.mgefFlags & kMgefFlag__Overridable) | GetDefaultMgefFlags(mgefCode);  // vanilla mgef records can't override all flags
            mgefObmeFlags = 0;
            SetHostility(GetDefaultHostility(mgefCode));
        }
        else if (loadVersion > VERSION_VANILLA_OBLIVION && loadVersion < MAKE_VERSION(2,0,0,0))
        {
            mgefFlags = data.mgefFlags & ~EffectSetting::kMgefFlag_PCHasEffect;
            _DMESSAGE("MgefFlags trimmed to %08X",mgefFlags);
            mgefObmeFlags = (loadVersion < MAKE_VERSION(1,0,4,0)) ? datx.mgefFlagOverrides : obme.mgefObmeFlags;
            mgefObmeFlags &= ~0x00190004; // v1 'Param' flags not stored in obmeFlags field 
        }
        else
        {
            mgefFlags = data.mgefFlags & ~EffectSetting::kMgefFlag_PCHasEffect;
            mgefObmeFlags = obme.mgefObmeFlags;
        }
        // magic groups
        MagicGroupList::ClearMagicGroups();
        if (loadVersion < MAKE_VERSION(2,0,0,0)) // vanilla - v1.0
        {
            UInt32 defGroup;    SInt32 defWeight;
            GetDefaultMagicGroup(mgefCode,defGroup,defWeight);
            MagicGroupList::AddMagicGroup((MagicGroup*)defGroup,defWeight); // add default group, if any   
            MagicGroupList::AddMagicGroup((MagicGroup*)MagicGroup::kFormID_ActiveEffectDisplay,1); // add default display group
        }
        else // v2+
        {
            groupList = mgls.groupList; // transfer group list from temporary object to this mgef
            mgls.groupList = 0; // clear temporary object group list so that it doesn't get destroyed when object leaves scope
        }
        // counter effects
        if (numCounters != esce.count) _WARNING("DATA chunk expects %i counter effects, but ESCE chunk contains %i", numCounters,esce.count);
        if (counterArray) MemoryHeap::FormHeapFree(counterArray);
        counterArray = esce.counters;   esce.counters = 0;
        numCounters = esce.count;   esce.count = 0;
        for (int c = 0; c < numCounters; c++) TESFileFormats::ResolveModValue(counterArray[c],file,TESFileFormats::kResType_MgefCode); // resolve codes
        // handler & handler-specific parameters
        if (loadVersion > VERSION_VANILLA_OBLIVION && loadVersion < MAKE_VERSION(2,0,0,0))
        {
            // get handler parameters from DATX or OBME chunk, depending on version
            UInt32 ehCode = (loadVersion < VERSION_OBME_FIRSTVERSIONED) ? datx.effectHandler : obme.effectHandler;
            UInt32 hParamA = mgefParam;
            UInt32 hParamB = (loadVersion < VERSION_OBME_FIRSTVERSIONED) ? datx.mgefParamB : obme._mgefParamB;
            UInt32 hParamFlags = (loadVersion < VERSION_OBME_FIRSTVERSIONED) ? datx.mgefFlagOverrides : obme.mgefObmeFlags;
            hParamFlags = (hParamFlags & 0x00190000) | (mgefFlags & kMgefFlag_Detrimental);
            UInt32 hUsesFlags = mgefFlags & 0x011F0000;
            // reset handler-specific param & flags on mgef proper
            mgefParam = 0;
            mgefFlags &= ~0x011F0004;
            // set handler
            SetHandler(MgefHandler::Create(ehCode,*this));
            // convert handler parameters & flags
            ConvertV1HandlerData(*this,file,loadVersion,hParamA,hParamB,hParamFlags,hUsesFlags);
        }
        else
        {
            SetHandler((loadVersion == VERSION_VANILLA_OBLIVION) ? 0 : ehnd);
            if (mgefFlags & (kMgefFlag_UseWeapon | kMgefFlag_UseArmor | kMgefFlag_UseActor))
                TESFileFormats::ResolveModValue(mgefParam,file,TESFileFormats::kResType_FormID); // param is a summoned formID
            else if (mgefFlags & kMgefFlag_UseActorValue) 
                TESFileFormats::ResolveModValue(mgefParam,file,TESFileFormats::kResType_ActorValue); // param is a modified avCode
        }
        ehnd = 0;
        // cost callback, dispel factor
        if (loadVersion < MAKE_VERSION(2,0,0,0))
        {
            costCallback = 0;
            dispelFactor = 1.0;
        }
        else
        {
            TESFileFormats::ResolveModValue(obme.costCallback,file,TESFileFormats::kResType_FormID);
            costCallback = (Script*)obme.costCallback;
            dispelFactor = obme.dispelFactor;
        }
    }  

    // clean up temporart dynamic objects
    if (esce.counters) MemoryHeap::FormHeapFree(esce.counters);
    if (ehnd) delete ehnd;

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
        obme.costCallback = costCallback ? ((TESForm*)costCallback)->formID : 0;
        obme.dispelFactor = dispelFactor;
        // set resolution info (deprecated, kept only for convenience with TES4Edit)
        if (mgefFlags & (kMgefFlag_UseWeapon | kMgefFlag_UseArmor | kMgefFlag_UseActor))
        {
            obme._mgefParamResType = TESFileFormats::kResType_FormID; // param is a summoned formID
        }
        else if (mgefFlags & kMgefFlag_UseActorValue)
        {
            obme._mgefParamResType = TESFileFormats::kResType_ActorValue; // param is a modified avCode
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

    // MagicGroupList
    if (requiresOBME) MagicGroupList::SaveComponent();

    // finalize form record
    FinalizeFormRecord();
    gLog.Outdent();
}
void EffectSetting::LinkForm()
{
    
    if (formFlags & kFormFlags_Linked) return;  // form already linked
    _VMESSAGE("Linking %s", GetDebugDescEx().c_str());

    // resistAV 
    if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue))
    {
        FormRefCounter::AddReference(this,form);  
    }
    // TODO - school

    // VFX, AFX
    // NOTE: because TESSound, TESEffectShader, and TESObjectLIGH are not yet defined in COEF, the
    // normal dynamic cast check on the form pointer is omitted.
    // If any of these formIDs refer to forms of the wrong type, all hell will break loose.
    castingSound    = (TESSound*)TESForm::LookupByFormID((UInt32)castingSound);
    boltSound       = (TESSound*)TESForm::LookupByFormID((UInt32)boltSound);
    hitSound        = (TESSound*)TESForm::LookupByFormID((UInt32)hitSound);
    areaSound       = (TESSound*)TESForm::LookupByFormID((UInt32)areaSound);
    effectShader    = (TESEffectShader*)TESForm::LookupByFormID((UInt32)effectShader);
    enchantShader   = (TESEffectShader*)TESForm::LookupByFormID((UInt32)enchantShader);
    light           = (TESObjectLIGH*)TESForm::LookupByFormID((UInt32)light);
    if (castingSound)   FormRefCounter::AddReference(this,(TESForm*)castingSound);
    if (boltSound)      FormRefCounter::AddReference(this,(TESForm*)boltSound);
    if (hitSound)       FormRefCounter::AddReference(this,(TESForm*)hitSound);
    if (effectShader)   FormRefCounter::AddReference(this,(TESForm*)effectShader);
    if (enchantShader)  FormRefCounter::AddReference(this,(TESForm*)enchantShader);
    if (light)          FormRefCounter::AddReference(this,(TESForm*)light);    

    // magic groups
    MagicGroupList::LinkComponent();

    // counter effects - unrecognized counters are discarded, to prevent 'dirty' edits in mgef dialog
    UInt32 j = 0;
    for (int i = 0; i < numCounters; i++)
    {
        UInt32 code = counterArray[i];
        counterArray[i] = 0;
        if (::EffectSetting* counter = EffectSetting::LookupByCode(code)) 
        {
            counterArray[j] = code; j++;
            FormRefCounter::AddReference(this,counter);
        }
        else _WARNING("%s has unrecognized counter effect code '%4.4s' {%08X}",GetDebugDescEx().c_str(),&code,code);
    }
    numCounters = j;
    
    // handler (includes mgefParam)
    GetHandler().LinkHandler(); // link handler object

    // cost callback
    // TODO: because Script is not yet defined in COEF, the normal dynamic cast check on the form pointer is omitted.
    costCallback = (Script*)TESForm::LookupByFormID((UInt32)costCallback);
    if (costCallback) FormRefCounter::AddReference(this,(TESForm*)costCallback);

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
        BSStringT desc; copyFrom.GetDebugDescription(desc);        
        _WARNING("Attempted copy from %s",desc.c_str());
        return;
    }   

    // copy BaseFormComponents
    TESForm::CopyAllComponentsFrom(copyFrom);
    
    // floating point members
    baseCost = mgef->baseCost;
    enchantFactor = mgef->enchantFactor;
    barterFactor = mgef->barterFactor;
    projSpeed = mgef->projSpeed;

    // resistAV 
    if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) FormRefCounter::RemoveReference(this,form); 
    if (TESForm* form = TESFileFormats::GetFormFromCode(mgef->GetResistAV(),TESFileFormats::kResType_ActorValue)) FormRefCounter::AddReference(this,form); 
    SetResistAV(mgef->GetResistAV()); // standardize 'no resistance' code
    
    // school - TODO: update ref counts
    school = mgef->school;

    // VFX, AFX
    if (light)              FormRefCounter::RemoveReference(this,(TESForm*)light);
    if (mgef->light)        FormRefCounter::AddReference(this,(TESForm*)mgef->light);
    light = mgef->light;
    if (effectShader)       FormRefCounter::RemoveReference(this,(TESForm*)effectShader);
    if (mgef->effectShader) FormRefCounter::AddReference(this,(TESForm*)mgef->effectShader);
    effectShader = mgef->effectShader;
    if (enchantShader)      FormRefCounter::RemoveReference(this,(TESForm*)enchantShader);
    if (mgef->enchantShader)FormRefCounter::AddReference(this,(TESForm*)mgef->enchantShader);
    enchantShader = mgef->enchantShader;
    if (castingSound)       FormRefCounter::RemoveReference(this,(TESForm*)castingSound);
    if (mgef->castingSound) FormRefCounter::AddReference(this,(TESForm*)mgef->castingSound);
    castingSound = mgef->castingSound;
    if (boltSound)          FormRefCounter::RemoveReference(this,(TESForm*)boltSound);
    if (mgef->boltSound)    FormRefCounter::AddReference(this,(TESForm*)mgef->boltSound);
    boltSound = mgef->boltSound;
    if (hitSound)           FormRefCounter::RemoveReference(this,(TESForm*)hitSound);
    if (mgef->hitSound)     FormRefCounter::AddReference(this,(TESForm*)mgef->hitSound);
    hitSound = mgef->hitSound;
    if (areaSound)          FormRefCounter::RemoveReference(this,(TESForm*)areaSound);
    if (mgef->areaSound)    FormRefCounter::AddReference(this,(TESForm*)mgef->areaSound);    
    areaSound = mgef->areaSound;

    // flags
    mgefFlags = mgef->mgefFlags;
    mgefObmeFlags = mgef->mgefObmeFlags;

    // groups
    MagicGroupList::CopyComponentFrom(*(MagicGroupList*)mgef);

    // counter array
    if (counterArray)
    {
        // clear refs to counter effects
        for (int i = 0; i < numCounters; i++)
        {
            if (::EffectSetting* counter = EffectSetting::LookupByCode(counterArray[i])) { FormRefCounter::RemoveReference(this,counter); }
        }
        // clear current counter array
        MemoryHeap::FormHeapFree(counterArray);
        counterArray = 0;
        numCounters = 0;
    }
    if (mgef->counterArray && (numCounters = mgef->numCounters))
    {
        counterArray = (UInt32*)MemoryHeap::FormHeapAlloc(numCounters * 4);
        // clear refs to counter effects
        for (int i = 0; i < numCounters; i++)
        {
            counterArray[i] = mgef->counterArray[i];
            if (::EffectSetting* counter = EffectSetting::LookupByCode(counterArray[i])) { FormRefCounter::AddReference(this,counter); }
        }
    }

    // handler & mgefParam
    if (GetHandler().HandlerCode() != mgef->GetHandler().HandlerCode())
    {
        GetHandler().UnlinkHandler();   // clear any references made by the handler
        SetHandler(MgefHandler::Create(mgef->GetHandler().HandlerCode(),*this));
    }
    GetHandler().CopyFrom(mgef->GetHandler());   // manages CrossRefs for mgefParam
    mgefParam = mgef->mgefParam;    // handler only copies if it uses the param

    // cost callback & dispel factor
    if (costCallback) FormRefCounter::RemoveReference(this,(TESForm*)costCallback);
    if (mgef->costCallback) FormRefCounter::AddReference(this,(TESForm*)mgef->costCallback);
    costCallback = mgef->costCallback;
    dispelFactor = mgef->dispelFactor;
    
    // assign a formID for non-temporary objects that don't already have one
    if ((~formFlags & kFormFlags_Temporary) && formID == 0 && TESDataHandler::dataHandler)
    {
        SetFormID(TESDataHandler::dataHandler->ReserveNextUnusedFormID());
    }
    
    // effect code
    if (mgefCode != mgef->mgefCode)
    {
        if (formFlags & kFormFlags_Temporary) mgefCode = mgef->mgefCode;   // this form is temporary, simply copy effect code
        else if (mgef->formFlags & kFormFlags_Temporary)
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
                _DMESSAGE("Changing mgefCode for %s to '%4.4s' {%08X} ...",GetDebugDescEx().c_str(),&mgef->mgefCode,mgef->mgefCode);
                // user confirms
                #ifndef OBLIVION                
                if (FormReferenceList* crossrefs = TESForm::GetCrossReferenceList(false))
                {
                    for (FormReferenceList::Node* node = &crossrefs->firstNode; node && node->data; node = node->next)
                    {
                        if (EffectSetting* ref = (EffectSetting*)dynamic_cast<::EffectSetting*>(node->data))  // MGEF
                        {
                            ref->ReplaceMgefCodeRef(mgefCode,mgef->mgefCode);
                        }
                        else if (EffectItemList* ref = dynamic_cast<EffectItemList*>(node->data)) // SPEL,ENCH,ALCH,INGR,SGST
                        {
                            // TODO - notify all EffectItemLists that use this code that it has changed  
                        }                        
                    }
                }
                #endif
                EffectSettingCollection::collection.RemoveAt(mgefCode); // unregister current code
                mgefCode = mgef->mgefCode; // copy code
                if (IsMgefCodeValid(mgefCode)) EffectSettingCollection::collection.SetAt(mgefCode,this); // register under new code
            }
        }
    }
}
bool EffectSetting::CompareTo(TESForm& compareTo)
{        
    _DMESSAGE("Comparing %s",GetDebugDescEx().c_str());

    // convert form to mgef
    EffectSetting* mgef = (EffectSetting*)dynamic_cast<::EffectSetting*>(&compareTo);
    if (!mgef)
    {
        BSStringT desc;  compareTo.GetDebugDescription(desc);    
        _WARNING("Attempted compare to %s",desc.c_str());
        return BoolEx(02);
    }    

    // compare BaseFormComponents
    if (TESForm::CompareAllComponentsTo(compareTo)) return BoolEx(10);

    // compare mgefCodes
    if (mgefCode != mgef->mgefCode) return BoolEx(20);

    // floating point members
    if (baseCost != mgef->baseCost) return BoolEx(30);
    if (enchantFactor != mgef->enchantFactor) return BoolEx(31);
    if (barterFactor != mgef->barterFactor) return BoolEx(32);
    if (projSpeed != mgef->projSpeed) return BoolEx(33);
    // resistances
    if (GetResistAV() != mgef->GetResistAV()) return BoolEx(40);
    // school
    if (school != mgef->school) return BoolEx(42);
    // VFX & AFX
    if (light != mgef->light) return BoolEx(50);
    if (effectShader != mgef->effectShader) return BoolEx(51);
    if (enchantShader != mgef->enchantShader) return BoolEx(52);
    if (castingSound != mgef->castingSound) return BoolEx(53);
    if (boltSound != mgef->boltSound) return BoolEx(54);
    if (hitSound != mgef->hitSound) return BoolEx(55);
    if (areaSound != mgef->areaSound) return BoolEx(56);
    // flags
    if (mgefFlags != mgef->mgefFlags) return BoolEx(60);
    if (mgefObmeFlags != mgef->mgefObmeFlags) return BoolEx(62);

    // groups
    if (MagicGroupList::CompareComponentTo(*(MagicGroupList*)mgef)) return BoolEx(70);

    // counter effects
    if (numCounters != mgef->numCounters) return BoolEx(80); // counter effect count doesn't match
    for (int i = 0; i < numCounters; i++)
    {
        bool found = false;
        for (int j = 0; j < numCounters; j++)
        {
            if (counterArray[i] == mgef->counterArray[j]) { found = true; break; }
        }
        if (!found)  return BoolEx(80); // unmatched counter effect
    }
   
    // handler & mgefParam
    if (GetHandler().CompareTo(mgef->GetHandler())) return BoolEx(90);
    if (mgefParam != mgef->mgefParam) return BoolEx(90);

    // cost callback & dispel factor
    if (costCallback != mgef->costCallback) return BoolEx(100);
    if (dispelFactor != mgef->dispelFactor) return BoolEx(101);

    return false;
}
#ifndef OBLIVION
// Reference management
void EffectSetting::RemoveFormReference(TESForm& form)
{
    _DMESSAGE("Removing form %08X from %s",form.formID,GetDebugDescEx().c_str());

    // call base method to handle vanilla references (TODO - possible rewrite?)
    UInt32 _mgefParam = mgefParam;
    mgefParam = 0;
    ::EffectSetting::RemoveFormReference(form);
    mgefParam = _mgefParam;

    // school, resistAV
    if (&form == TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) SetResistAV(ActorValues::kActorVal__UBOUND); // resistAV
    // TODO - school

    // groups
    MagicGroupList::RemoveComponentFormRef(form);

    // counter effects
    if (::EffectSetting* ref = dynamic_cast<::EffectSetting*>(&form))
    {
        // remove refs by mgefcode & compact list
        int insPos = 0;
        for (int i = 0; i < numCounters; i++) 
        { 
            UInt32 code = counterArray[i];
            counterArray[i] = 0;
            if (code != ref->mgefCode) { counterArray[insPos] = code; insPos++; }        
        }
        numCounters = insPos;
        // deallocate list only if it is now empty - a list that is too big does no harm
        if (numCounters == 0 && counterArray) { MemoryHeap::FormHeapFree(counterArray); counterArray = 0;}
    }

    // handler + mgefParam
    GetHandler().RemoveFormReference(form);

    // cost callback
     if (&form == (TESForm*)costCallback) costCallback = 0;
}
bool EffectSetting::FormRefRevisionsMatch(BSSimpleList<TESForm*>* checkinList)
{
    // TODO - rewrite to replace handling of mgefParam & new members
    // This method is only used for revision control, though, which few in any modders use
    return ::EffectSetting::FormRefRevisionsMatch(checkinList);
}
void EffectSetting::GetRevisionUnmatchedFormRefs(BSSimpleList<TESForm*>* checkinList, BSStringT& output)
{
    // TODO - rewrite to replace handling of mgefParam & new members
    // This method is only used for revision control, though, which few in any modders use
    ::EffectSetting::GetRevisionUnmatchedFormRefs(checkinList,output);
}
#endif
// methods: debugging
BSStringT EffectSetting::GetDebugDescEx() const
{
    // potentially inefficient, but worth it for the ease of use
    BSStringT desc;
    (const_cast<EffectSetting*>(this))->GetDebugDescription(desc);
    return desc;
}
// methods: serialization
void EffectSetting::UnlinkForm()
{
     if (~formFlags & kFormFlags_Linked) return;  // form not linked
    _VMESSAGE("Unlinking %s", GetDebugDescEx().c_str());

    // resistAV   
    if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue))
    {
        FormRefCounter::RemoveReference(this,form);
    }
    // TODO - school

    // VFX, AFX
    if (castingSound)   FormRefCounter::RemoveReference(this,(TESForm*)castingSound);
    if (boltSound)      FormRefCounter::RemoveReference(this,(TESForm*)boltSound);
    if (hitSound)       FormRefCounter::RemoveReference(this,(TESForm*)hitSound);
    if (effectShader)   FormRefCounter::RemoveReference(this,(TESForm*)effectShader);
    if (enchantShader)  FormRefCounter::RemoveReference(this,(TESForm*)enchantShader);
    if (light)          FormRefCounter::RemoveReference(this,(TESForm*)light);    
    if (castingSound)   castingSound = (TESSound*)((TESForm*)castingSound)->formID;
    if (boltSound)      boltSound = (TESSound*)((TESForm*)boltSound)->formID;
    if (hitSound)       hitSound = (TESSound*)((TESForm*)hitSound)->formID;
    if (effectShader)   effectShader = (TESEffectShader*)((TESForm*)effectShader)->formID;
    if (enchantShader)  enchantShader = (TESEffectShader*)((TESForm*)enchantShader)->formID;
    if (light)          light = (TESObjectLIGH*)((TESForm*)light)->formID;

    // magic groups
    MagicGroupList::UnlinkComponent();

    // counter effects
    for (int i = 0; i < numCounters; i++)
    {
        if (::EffectSetting* counter = EffectSetting::LookupByCode(counterArray[i])) { FormRefCounter::RemoveReference(this,counter); }
    }
    
    // handler (includes mgefParam)
    GetHandler().UnlinkHandler(); // unlink handler object

    // cost callback
    if (costCallback)
    { 
        FormRefCounter::RemoveReference(this,(TESForm*)costCallback); 
        costCallback = (Script*)((TESForm*)costCallback)->formID;
    }

    // set linked flag
    formFlags &= ~kFormFlags_Linked;
}
// conversion
struct VanillaEffectData
{
    // members
    UInt32  ehCode;
    UInt32  staticFlags;
    UInt8   hostility;
    UInt32  groupFormID; // first default group - vanilla effects don't have > 1
    SInt32  groupWeight;
};
class VanillaEffectsMap : public std::map<UInt32,VanillaEffectData>
{
public:    
    inline void insert(UInt32 sMgefCode, UInt32 sEHCode, UInt32 staticFlags, UInt32 groupFormID, SInt32 groupWeight, UInt8 hostility)
    {
        VanillaEffectData entry;
        entry.ehCode = Swap32(sEHCode);
        entry.staticFlags = staticFlags;
        entry.hostility = hostility;  
        entry.groupFormID = groupFormID;
        entry.groupWeight = groupWeight;
        std::map<UInt32,VanillaEffectData>::insert(std::pair<UInt32,VanillaEffectData>(Swap32(sMgefCode),entry));
    }
    VanillaEffectsMap()
    {
        // don't put output statements here, as the output log may not yet be initialized        
        insert('ABAT','ABSB',0x00100027,0x00000000,0,Magic::kHostility_Hostile);
        insert('ABFA','ABSB',0x00000025,0x00000000,0,Magic::kHostility_Hostile);
        insert('ABHE','ABSB',0x00000025,0x00000000,0,Magic::kHostility_Hostile);
        insert('ABSK','ABSB',0x00080027,0x00000000,0,Magic::kHostility_Hostile);
        insert('ABSP','ABSB',0x00000025,0x00000000,0,Magic::kHostility_Hostile);
        insert('BA01','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA02','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA03','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA04','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA05','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA06','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA07','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA08','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA09','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BA10','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BABO','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BACU','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BAGA','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BAGR','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BAHE','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BASH','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('BRDN','MDAV',0x00000073,0x00000000,0,Magic::kHostility_Hostile);
        insert('BW01','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW02','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW03','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW04','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW05','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW06','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW07','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW08','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW09','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BW10','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BWAX','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BWBO','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BWDA','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BWMA','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('BWSW','SMBO',0x00010112,MagicGroup::kFormID_BoundWeaponLimit,1,Magic::kHostility_Beneficial);
        insert('CALM','CALM',0x40000066,0x00000000,0,Magic::kHostility_Neutral);
        insert('CHML','CHML',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('CHRM','MDAV',0x00000062,0x00000000,0,Magic::kHostility_Neutral);
        insert('COCR','COCR',0x40000062,0x00000000,0,Magic::kHostility_Neutral);
        insert('COHU','COHU',0x40000062,0x00000000,0,Magic::kHostility_Neutral);
        insert('CUDI','CURE',0x000001F0,0x00000000,0,Magic::kHostility_Beneficial);
        insert('CUPA','CURE',0x000001F0,0x00000000,0,Magic::kHostility_Beneficial);
        insert('CUPO','CURE',0x000001F0,0x00000000,0,Magic::kHostility_Beneficial);
        insert('DARK','DARK',0x80000072,0x00000000,0,Magic::kHostility_Neutral);
        insert('DEMO','DEMO',0x40000066,0x00000000,0,Magic::kHostility_Neutral);
        insert('DGAT','MDAV',0x00100075,0x00000000,0,Magic::kHostility_Hostile);
        insert('DGFA','MDAV',0x00000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('DGHE','MDAV',0x20000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('DGSP','MDAV',0x00000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('DIAR','DIAR',0x00000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('DISE','ACTV',0x00000000,0x00000000,0,Magic::kHostility_Neutral);
        insert('DIWE','DIWE',0x00000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('DRAT','MDAV',0x00100077,0x00000000,0,Magic::kHostility_Hostile);
        insert('DRFA','MDAV',0x00000077,0x00000000,0,Magic::kHostility_Hostile);
        insert('DRHE','MDAV',0x00000077,0x00000000,0,Magic::kHostility_Hostile);
        insert('DRSK','MDAV',0x00080077,0x00000000,0,Magic::kHostility_Hostile);
        insert('DRSP','MDAV',0x00000077,0x00000000,0,Magic::kHostility_Hostile);
        insert('DSPL','DSPL',0x000000F0,0x00000000,0,Magic::kHostility_Neutral);
        insert('DTCT','DTCT',0x80000012,0x00000000,0,Magic::kHostility_Beneficial);
        insert('DUMY','MDAV',0x20000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('FIDG','MDAV',0x20000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('FISH','SHLD',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FOAT','MDAV',0x00100072,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FOFA','MDAV',0x00000072,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FOHE','MDAV',0x00000072,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FOMM','MDAV',0x00000012,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FOSK','MDAV',0x00080072,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FOSP','MDAV',0x00000072,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FRDG','MDAV',0x00000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('FRNZ','FRNZ',0x40000062,0x00000000,0,Magic::kHostility_Neutral);
        insert('FRSH','SHLD',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('FTHR','MDAV',0x00000076,0x00000000,0,Magic::kHostility_Beneficial);
        insert('INVI','INVI',0x00000172,0x00000000,0,Magic::kHostility_Beneficial);
        insert('JRSH','ACTV',0x00000170,0x00000000,0,Magic::kHostility_Neutral);
        insert('LGHT','LGHT',0x80000072,0x00000000,0,Magic::kHostility_Neutral);
        insert('LISH','SHLD',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('LOCK','LOCK',0x000000E0,0x00000000,0,Magic::kHostility_Neutral);
        insert('MYHL','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('MYTH','SMBO',0x00020112,MagicGroup::kFormID_BoundHelmLimit,1,Magic::kHostility_Beneficial);
        insert('NEYE','NEYE',0x00000112,0x00000000,0,Magic::kHostility_Beneficial);
        insert('OPEN','OPEN',0x000000C0,0x00000000,0,Magic::kHostility_Neutral);
        insert('PARA','PARA',0x00000173,0x00000000,0,Magic::kHostility_Hostile);
        insert('POSN','ACTV',0x00000000,0x00000000,0,Magic::kHostility_Neutral);
        insert('RALY','MDAV',0x00000062,0x00000000,0,Magic::kHostility_Neutral);
        insert('REAN','REAN',0x10000360,0x00000000,0,Magic::kHostility_Neutral);
        insert('REAT','MDAV',0x00100070,0x00000000,0,Magic::kHostility_Beneficial);
        insert('REDG','MDAV',0x0000001A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('REFA','MDAV',0x00000070,0x00000000,0,Magic::kHostility_Beneficial);
        insert('REHE','MDAV',0x00000070,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RESP','MDAV',0x00000070,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RFLC','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSDI','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSFI','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSFR','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSMA','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSNW','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSPA','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSPO','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSSH','MDAV',0x0000007A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('RSWD','MDAV',0x0000017A,0x00000000,0,Magic::kHostility_Beneficial);
        insert('SABS','MDAV',0x00000072,0x00000000,0,Magic::kHostility_Beneficial);
        insert('SEFF','SEFF',0x00000170,0x00000000,0,Magic::kHostility_Neutral);
        insert('SHDG','MDAV',0x20000075,0x00000000,0,Magic::kHostility_Hostile);
        insert('SHLD','SHLD',0x0000007A,MagicGroup::kFormID_ShieldVFX,1,Magic::kHostility_Beneficial);
        insert('SLNC','MDAV',0x00000173,0x00000000,0,Magic::kHostility_Hostile);
        insert('STMA','MDAV',0x00000112,0x00000000,0,Magic::kHostility_Neutral);
        insert('STRP','STRP',0x00000163,0x00000000,0,Magic::kHostility_Hostile);
        insert('SUDG','SUDG',0x00000014,0x00000000,0,Magic::kHostility_Neutral);
        insert('TELE','TELE',0x80000242,0x00000000,0,Magic::kHostility_Neutral);
        insert('TURN','TURN',0x40000063,0x00000000,0,Magic::kHostility_Hostile);
        insert('VAMP','VAMP',0x10000092,0x00000000,0,Magic::kHostility_Neutral);
        insert('WABR','MDAV',0x00000172,0x00000000,0,Magic::kHostility_Beneficial);
        insert('WAWA','MDAV',0x00000172,0x00000000,0,Magic::kHostility_Beneficial);
        insert('WKDI','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('WKFI','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('WKFR','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('WKMA','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('WKNW','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('WKPO','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('WKSH','MDAV',0x0000007F,0x00000000,0,Magic::kHostility_Hostile);
        insert('Z001','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z002','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z003','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z004','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z005','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z006','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z007','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z008','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z009','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z010','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z011','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z012','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z013','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z014','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z015','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z016','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z017','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z018','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z019','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('Z020','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZCLA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZDAE','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZDRE','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZDRL','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZFIA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZFRA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZGHO','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZHDZ','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZLIC','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSCA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSKA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSKC','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSKE','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSKH','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSPD','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZSTA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZWRA','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZWRL','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZXIV','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
        insert('ZZOM','SMAC',0x00040112,MagicGroup::kFormID_SummonCreatureLimit,1,Magic::kHostility_Beneficial);
    }
} vanillaEffectsMap;
UInt8 EffectSetting::GetDefaultHostility(UInt32 mgefCode)
{
    VanillaEffectsMap::iterator it = vanillaEffectsMap.find(mgefCode); // find entry in vanilla effects map
    if (it == vanillaEffectsMap.end()) return Magic::kHostility_Neutral; // effect is not vanilla, return default hostility
    else return it->second.hostility;      
}
UInt32 EffectSetting::GetDefaultMgefFlags(UInt32 mgefCode)
{
    VanillaEffectsMap::iterator it = vanillaEffectsMap.find(mgefCode); // find entry in vanilla effects map
    if (it == vanillaEffectsMap.end()) return 0; // effect is not vanilla, return default mgef flags (zero, see notes in constructor)
    else return it->second.staticFlags; 
}
UInt32 EffectSetting::GetDefaultHandlerCode(UInt32 mgefCode)
{
    VanillaEffectsMap::iterator it = vanillaEffectsMap.find(mgefCode); // find entry in vanilla effects map
    if (it == vanillaEffectsMap.end()) return EffectHandler::kACTV; // effect is not vanilla, use ACTV handler
    else return it->second.ehCode; 
}
void EffectSetting::GetDefaultMagicGroup(UInt32 mgefCode, UInt32& groupFormID, SInt32& groupWeight)
{
    VanillaEffectsMap::iterator it = vanillaEffectsMap.find(mgefCode); // find entry in vanilla effects map
    if (it == vanillaEffectsMap.end()) 
    {
        groupFormID = 0;
        groupWeight = 0;
    }
    else
    {
        groupFormID = it->second.groupFormID;
        groupWeight = it->second.groupWeight;
    }
}
bool EffectSetting::RequiresObmeMgefChunks()
{
    // define return codes - must evaluate to boolean
    enum
    {
        kOBMENotRequired        = 0,
        kReqOBME_General        = 1,
        kReqOBME_MgefCode,
        kReqOBME_EditorID,
        kReqOBME_School,
        kReqOBME_ResistAV,
        kReqOBME_MgefFlags,
        kReqOBME_ObmeFlags, // except 'Beneficial'
        kReqOBME_MagicGroup,
        kReqOBME_CounterEffects,
        kReqOBME_BeneficialFlag,
        kReqOBME_Handler,
        kReqOBME_CostCallback,
        kReqOBME_DispelFactor,
    };

    // mgefCode - must be vanilla
    if (!IsMgefCodeVanilla(mgefCode)) return BoolEx(10);

    // editorID - must be the mgefCode reinterpreted as a string
    char codestr[5] = {{0}};
    *(UInt32*)codestr = mgefCode;  
    if (GetEditorID() == 0 || strcmp(GetEditorID(),codestr) != 0) return BoolEx(20);

    // school - must be a vanilla magic school
    if (school > Magic::kSchool__MAX) return BoolEx(30);

    // resistanceAV - must be a vanilla AV
    if (GetResistAV() > ActorValues::kActorVal__MAX && GetResistAV() != ActorValues::kActorVal__UBOUND) return BoolEx(35);
    
    // non-overridable mgef flags must match vanilla values
    if ((mgefFlags & ~kMgefFlag__Overridable) != GetDefaultMgefFlags(mgefCode)) return BoolEx(40);

    // OBME flags (all except beneficial must be zero)
    if (mgefObmeFlags & ~kMgefObmeFlag_Beneficial) return BoolEx(45);

    // beneficial flag - must be zero unless default hostility is beneficial
    if ((mgefObmeFlags & kMgefObmeFlag_Beneficial) && GetDefaultHostility(mgefCode) != Magic::kHostility_Beneficial) return BoolEx(46);

    // magic groups must match vanilla groups, if any    
    UInt32 gpDispFormID = MagicGroup::kFormID_ActiveEffectDisplay;  // formID of default display group
    UInt32 gpDefFormID = 0; SInt32 gpDefWeight = 0;
    GetDefaultMagicGroup(mgefCode,gpDefFormID,gpDefWeight); // formID + weight for additional default group
    for (MagicGroupList::GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (!entry->group || !entry->group->formID) return BoolEx(50);  // invalid group
        if (entry->group->formID == gpDispFormID)   // found default display group
        {
            if (entry->weight != 1) return BoolEx(51);
            else gpDispFormID = 0;
        }
        else if (entry->group->formID == gpDefFormID)   // found default group
        {
            if (entry->weight != gpDefWeight) return BoolEx(52);
            else gpDefFormID = 0;
        }
        else return BoolEx(55); // non-default group
    }
    if (gpDefFormID || gpDispFormID) return BoolEx(59); // required default group(s) not found

    // counter array - can contain no non-vanilla effects
    for (int i = 0; counterArray && i < numCounters; i++)
    {
        if (!IsMgefCodeVanilla(counterArray[i])) return BoolEx(kReqOBME_CounterEffects);
    }    

    // Effect Handler - must differ from default handler object
    UInt32 defHandlerCode = GetDefaultHandlerCode(mgefCode);
    if (defHandlerCode != GetHandler().HandlerCode()) return BoolEx(kReqOBME_Handler);
    MgefHandler* defHandler = MgefHandler::Create(defHandlerCode,*this);
    bool handlerNoMatch = defHandler->CompareTo(GetHandler());
    delete defHandler;
    if (handlerNoMatch) return BoolEx(kReqOBME_Handler);

    // cost callback must be null
    if (costCallback) return BoolEx(kReqOBME_CostCallback);

    // dispel factor must be 1.0
    if (dispelFactor != 1.0) return BoolEx(kReqOBME_DispelFactor);

    return BoolEx(kOBMENotRequired);
}
// fields
bool EffectSetting::GetFlag(UInt32 flagShift) const
{
    if (flagShift < 0x20) return (mgefFlags & (1<<flagShift));
    flagShift -= 0x20;
    if (flagShift <= 0x20) return (mgefObmeFlags & (1<<flagShift));
    return false; // invalid flag shift
}
void EffectSetting::SetFlag(UInt32 flagShift, bool newval)
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
UInt8 EffectSetting::GetHostility() const
{
    if (GetFlag(kMgefFlagShift_Hostile)) return Magic::kHostility_Hostile;
    else if (GetFlag(kMgefFlagShift_Beneficial)) return Magic::kHostility_Beneficial;
    else return Magic::kHostility_Neutral;
}
void EffectSetting::SetHostility(UInt8 newval)
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
UInt32 EffectSetting::GetResistAV()
{
    if (resistAV == ActorValues::kActorVal__NONE || resistAV == ActorValues::kActorVal__MAX) return ActorValues::kActorVal__UBOUND;
    else return resistAV;
}
void EffectSetting::SetResistAV(UInt32 newResistAV)
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
bool EffectSetting::IsMgefCodeVanilla(UInt32 mgefCode)
{
    return (vanillaEffectsMap.find(mgefCode) != vanillaEffectsMap.end());   // find entry in vanilla effects map
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
    UInt32 mask = kMgefCode_DynamicFlag | (TESDataHandler::dataHandler ? (TESDataHandler::dataHandler->nextFormID >> 0x18) : 0xFF);
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
void EffectSetting::ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode)
{
    _DMESSAGE("%s : Replacing '%4.4s'{%08X} -> '%4.4s'{%08X}",GetDebugDescEx().c_str(),&oldMgefCode,oldMgefCode,&newMgefCode,newMgefCode);
    // counter effects
    for (int i = 0; i < numCounters; i++)
    {
        if (counterArray[i] == oldMgefCode) counterArray[i] = newMgefCode;
    }
}
// effect handler
MgefHandler& EffectSetting::GetHandler()
{    
    if (!effectHandler)
    {
        // this effect has no handler, create a default handler
        effectHandler = MgefHandler::Create(GetDefaultHandlerCode(mgefCode),*this);
        // check if default handler not yet implemented - should never happen in release version
        if (!effectHandler) effectHandler = MgefHandler::Create(EffectHandler::kACTV,*this); 
    }
    return *effectHandler;
}
void EffectSetting::SetHandler(MgefHandler* handler)
{
    if (effectHandler) delete effectHandler;    // clear current handler
    effectHandler = handler;
}

};  // end namespace OBME