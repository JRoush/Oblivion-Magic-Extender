#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/SpellItem.h"
#include "API/Magic/MagicItemForm.h" // TODO - replace with OBME::MagicItemForms
#include "API/Magic/MagicItemObject.h" // TODO - replace with OBME::MagicItemObjects

#include "API/TES/TESDataHandler.h"
#include "API/TESFiles/TESFile.h"
#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"
#include "Components/EventManager.h"
#include "Components/FormRefCounter.h"
#include "Utilities/Memaddr.h"

#pragma warning (disable:4800) // forced typecast of int to bool

// local declaration of module handle.
extern HMODULE hModule;

namespace OBME {

// memory patch & hook addresses
#ifdef OBLIVION // bitmask of virtual methods that need to be manually copied from OBME vtbl to vanilla vtbl
    const UInt32 TESForm_PatchMethods[2] = { 0x28000290, 0x00006000 };
    const UInt32 TESForm_VtblSize = 0xDC;
#else
    const UInt32 TESForm_PatchMethods[3] = { 0x50052000, 0xE000C000, 0x000032E8 };
    const UInt32 TESForm_VtblSize = 0x138;
#endif
memaddr TESForm_Vtbl                            (0x00A32B14,0x0095C914);    // EffectSetting::TESFormIDListView::{vtbl}
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

// constructor, destructor
void PatchEffectSettingFormVtbl(void* obmeFormVtbl)
{
    // copy all methods from vanilla TESForm::vtbl to the compiler-generated vtbl, except those that are explicitly overriden
    _MESSAGE("Patching Game/CS TESFormIDListView vtbl from <%p>",obmeFormVtbl);
    memaddr thisVtbl((UInt32)obmeFormVtbl);
    gLog.Indent();
    for (int i = 0; i < TESForm_VtblSize/4; i++)
    {
        if ((TESForm_PatchMethods[i/0x20] >> (i%0x20)) & 1) { _VMESSAGE("Offset 0x%04X *not* patched",i*4); } 
        else thisVtbl.SetVtblEntry(i*4,TESForm_Vtbl.GetVtblEntry(i*4));
    }        
    gLog.Outdent(); 
}
EffectSetting::EffectSetting()
: ::EffectSetting(), MagicGroupList(), effectHandler(0), mgefObmeFlags(0), costCallback(0), dispelFactor(1.0)
{    
    _VMESSAGE("Constructing Mgef <%p>",this);    

    if (static bool runonce = true)
    {
        // patch form vtbl
        PatchEffectSettingFormVtbl(memaddr::GetObjectVtbl(this));
        runonce  =  false;
    }

    // game constructor uses some odd initialization values
    // NOTE: be *very* careful about changing default initializations
    // the game occassionally uses a default-constructed EffectSetting, and the default values are
    // hardcoded all over the place.  In particular,  a default-created effect is used as a comparison 
    // template for filtering effects before spellmaking, enchanting, etc.  Do NOT change the mgefFlags 
    // initialization from zero, or the default school from 6, as this will cause some effects to be
    // incorrectly filtered out.
    resistAV = ActorValues::kActorVal__UBOUND; // standardize the 'No Resistance' value to 0xFFFFFFFF, which is actually used in Resistance system
    mgefCode = kMgefCode_None;
}
EffectSetting::~EffectSetting()
{
    _VMESSAGE("Destructing Mgef <%p>",this);
    FormRefCounter::ClearReferences(this);  // clear auxiliary ref use info for this effect setting
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
    // mark effect as unlinked, so that Link() may be called after datahandler creation to update reference counts
    mgef->formFlags &= ~TESForm::kFormFlags_Linked;
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
    // display group (unlinked)
    mgef->AddMagicGroup((MagicGroup*)MagicGroup::kFormID_ActiveEffectDisplay,1);
    // default group (unlinked)
    UInt32 defGroup; SInt32 defWeight;
    EffectSetting::GetDefaultMagicGroup(mgefCode,defGroup,defWeight);
    if (defGroup) mgef->AddMagicGroup((MagicGroup*)defGroup,defWeight);
    // default handler
    mgef->SetHandler(MgefHandler::Create(EffectSetting::GetDefaultHandlerCode(mgefCode),*mgef));
    // check if default mgefs have already been linked
    if (EffectSetting::defaultVFXEffect) mgef->LinkForm();  // if defaults are already linked, link this as well
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
            TESFileFormats::ResolveModValue(mgefCode,*file,TESFileFormats::kResType_MgefCode);
            // search for already created effect with this code
            ::EffectSetting* mgef = dynamic_cast<::EffectSetting*>(form);
            if (mgef && mgef->mgefCode == mgefCode)
            {
                // effect w/ same formid & mgefcode was found by datahandler
            }
            else 
            {
                if (form)
                {                    
                    // formid belongs to an existing object that is not MGEF or has a different effect code
                    _ERROR("Cannot load MGEF record into existing %s ",
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
void LinkDefaultEffects()
{
    _DMESSAGE("Linking default magic effects ...");
    NiTMapIterator pos = EffectSettingCollection::collection.GetFirstPos();
    while (pos)
    {
        UInt32 code;
        OBME::EffectSetting* mgef;
        EffectSettingCollection::collection.GetNext(pos,code,*(::EffectSetting**)&mgef);
        if (mgef) mgef->LinkForm(); // LinkForm will update reference counts & construct pointers    
    }
    
    // cache pointers to meta effects
    EffectSetting::defaultVFXEffect     = (EffectSetting*)EffectSettingCollection::LookupByCode(Swap32('SEFF'));
    EffectSetting::diseaseVFXEffect     = (EffectSetting*)EffectSettingCollection::LookupByCode(Swap32('DISE'));
    EffectSetting::poisonVFXEffect      = (EffectSetting*)EffectSettingCollection::LookupByCode(Swap32('POSN'));
}
void ClearDefaultEffects()
{
    _DMESSAGE("Clearing default magic effects ...");
    // clear pointers to meta effects
    EffectSetting::defaultVFXEffect     = 0;
    EffectSetting::diseaseVFXEffect     = 0;
    EffectSetting::poisonVFXEffect      = 0;
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

    // hook creation & clearing of vanilla magic effects, so we can managed cached pointers to them
    EventManager::RegisterEventCallback(EventManager::DataHandler_PostCreateDefaults,LinkDefaultEffects);
    EventManager::RegisterEventCallback(EventManager::DataHandler_Clear,ClearDefaultEffects);

    // TODO - hook loading of player's known effect list to resolve effect codes

    // patch creation of MGEF dialog in CS - replace CS module instance with local module instance
    TESCS_CreateMgefDialog_Patch.WriteData32((UInt32)&hModule);

    // check if effects were created before OBSE patched in; they need to be destroyed & remade as OBME::EffectSettings 
    if (EffectSettingCollection::collection.GetCount() > 0)
    {
        _VMESSAGE("Found magic effects table already filled - resetting ...");        
        TESDataHandler* dataHandler = TESDataHandler::dataHandler;  // prevent TESForm::TESForm from assigning formIDs by
        TESDataHandler::dataHandler = 0;                            //  temporarily clearing global data handler        
        EffectSettingCollection::Reset();   // reset effect setting collection        
        TESDataHandler::dataHandler = dataHandler;  // restore the global data handler  
        LinkDefaultEffects(); // Link newly created effects to other default objects
    }
}

};  // end namespace OBME