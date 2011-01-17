#include "OBME/EffectSetting.h"
#include "OBME/EffectSetting.rc.h"
#include "OBME/OBME_version.h"
#include "OBME/LoadOrderResolution.h"
#include "OBME/Magic.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/MagicItemForm.h" // TODO - replace with OBME/MagicItemForm.h
#include "API/Magic/MagicItemObject.h" // TODO - replace with OBME/MagicItemForm.h

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

#include "Utilities/Memaddr.h"

#include <set>
#include <typeinfo>

#pragma warning (disable:4800) // forced typecast of int to bool

// local declaration of module handle.
extern HMODULE hModule;

namespace OBME {

// memory patch & hook addresses
memaddr EffectSetting_Vtbl                      (0x00A32B14,0x0095C914);
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
    loadFlags = 0;    // no fields loaded
    UInt32 loadVersion = VERSION_VANILLA_OBLIVION;

    // these parameters belong in the effect handler, but were stored in the OBME chunk in v1 - require temporary storage
    UInt32 _handlerCode = EffectSettingHandler::GetDefaultHandlerCode(mgefCode);
    UInt32 _handlerParamA = 0;
    UInt32 _handlerParamB = 0;   
    UInt32 _handlerFlags = 0;
    UInt32 _handlerFlagMask = 0x00190004; // 'Param' flags, now repurposed

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
            _DMESSAGE("EDID chunk: '%4.4s'",&mgefCode);
            break;

        // obme data, added in v1.beta (so version is guaranteed > VERSION_OBME_LASTUNVERSIONED)
        case 'OBME': 
            if (loadFlags & ~kMgefChunk_Edid)
            {
                gLog.PushStyle();
                _ERROR("Unexpected chunk 'OBME' - must immediately follow chunk 'EDID'");
                gLog.PopStyle();
            }
            EffectSettingObmeChunk obme;
            memset(&obme,0,sizeof(obme));
            file.GetChunkData(&obme,sizeof(obme));
             // version
            loadVersion = obme.obmeRecordVersion;
            // effect handler code
            _handlerCode = obme.effectHandler;            
            // flags
            mgefObmeFlags = obme.mgefObmeFlags & ~_handlerFlagMask; // 'Param' now deprecated
            // handler parameters
            if (loadVersion <= VERSION_OBME_LASTV1)
            {       
                _handlerParamB = obme._mgefParamB; // 'MgefParamB': not resolved, that must be done when it is added to the actual handler
                _handlerFlags = obme.mgefObmeFlags & _handlerFlagMask; // 'Param' flags
            }
            // done   
            loadFlags |= kMgefChunk_Obme;
            _DMESSAGE("OBME chunk: version {%08X}",obme.obmeRecordVersion);
            break;

        // obme auxiliary data - deprecated in favor of the OBME chunk as of v1.beta4
        case 'DATX':
            if (loadFlags & ~kMgefChunk_Edid)
            {
                gLog.PushStyle();
                _ERROR("Unexpected chunk 'DATX' - must immediately follow chunk 'EDID'");
                gLog.PopStyle();
            }
            EffectSettingDatxChunk datx;
            memset(&datx,0,sizeof(datx));
            file.GetChunkData(&datx,sizeof(datx));
            // early beta didn't write version or resolution info, assume latest unversioned build
            loadVersion = VERSION_OBME_LASTUNVERSIONED; 
            // effect handler code
            _handlerCode = datx.effectHandler; 
            // flags
            mgefObmeFlags = obme.mgefObmeFlags & ~_handlerFlagMask; // 'Param' now deprecated
            // handler parameters  
            // NOTE: mgefParamB is not resolved, that must be done when it is added to the actual handler later
            _handlerParamB = datx.mgefParamB; // 'MgefParamB': not resolved, that must be done when it is added to the actual handler
            _handlerFlags = datx.mgefFlagOverrides & _handlerFlagMask;
            // done
            loadFlags |= kMgefChunk_Datx;
            _DMESSAGE("DATX chunk: assumed version {%08X}",loadVersion);
            break;

        // actual editor id
        case 'EDDX':            
            char edid[0x200];
            memset(edid,0,sizeof(edid));
            file.GetChunkData(edid,sizeof(edid));
            SetEditorID(edid);
            loadFlags |= kMgefChunk_Eddx;
            _DMESSAGE("EDDX chunk: '%s'",edid);
            break;

        // name
        case 'FULL':            
            TESFullName::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Full;
            _DMESSAGE("FULL chunk: '%s'",name.c_str());
            break;

        // description
        case 'DESC':            
            TESDescription::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Desc;
            _DMESSAGE("DESC chunk: '%s'",GetDescription(this,Swap32('DESC')));
            break;

        // icon
        case 'ICON':            
            TESIcon::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Icon;
            _DMESSAGE("ICON chunk: path '%s'",texturePath.c_str());
            break;

        // model
        case 'MODL':
        case 'MODB':
        case 'MODT':            
            TESModel::LoadComponent(*this,file);
            loadFlags |= kMgefChunk_Modl;
            _DMESSAGE("%4.4s chunk: path '%s'",&chunktype,ModelPath());
            break;

        // main effect data
        case 'DATA':
            // clear counter array
            SetCounterEffects(0,NULL);
            // load data
            EffectSettingDataChunk data;
            memset(&data,0,sizeof(data));
            file.GetChunkData(&data,sizeof(data));
            // VFX forms, AFX forms
            ResolveModValue(data.light,file,kResolutionType_FormID);
            ResolveModValue(data.effectShader,file,kResolutionType_FormID);
            ResolveModValue(data.enchantShader,file,kResolutionType_FormID);
            ResolveModValue(data.castingSound,file,kResolutionType_FormID);
            ResolveModValue(data.boltSound,file,kResolutionType_FormID);
            ResolveModValue(data.hitSound,file,kResolutionType_FormID);
            ResolveModValue(data.areaSound,file,kResolutionType_FormID);
            // resistance AV
            if (data.resistAV == ActorValues::kActorVal__MAX || data.resistAV == 0xFFFFFFFF) data.resistAV = ActorValues::kActorVal__NONE;
            ResolveModValue(data.resistAV,file,kResolutionType_EnumExtension);
            // school
            ResolveModValue(data.school,file,kResolutionType_EnumExtension);
            // handler parameters
            if (data.mgefFlags & (kMgefFlag_UseWeapon | kMgefFlag_UseArmor | kMgefFlag_UseActor))
            {
                ResolveModValue(data.mgefParam,file,kResolutionType_FormID); // param is a summoned formID
            }
            else if (data.mgefFlags & kMgefFlag_UseActorValue)
            {
                ResolveModValue(data.mgefParam,file,kResolutionType_EnumExtension); // param is a modified avCode
            }
            else if (loadVersion >= VERSION_OBME_LASTUNVERSIONED && loadVersion <= VERSION_OBME_LASTV1)
            {       
                _handlerParamA = data.mgefParam; // 'MgefParamA': not resolved, that must be done when it is added to the actual handler
                data.mgefParam = 0;
            }
            else data.mgefParam = 0;
            // mgefFlags
            if (loadVersion == VERSION_VANILLA_OBLIVION)
            {
                // vanilla records can only override flags under this mask
                data.mgefFlags &= 0x0FE03C00;
                data.mgefFlags |= (mgefFlags & ~0x0FE03C00);
            }
            data.mgefFlags &= ~kMgefFlag_PCHasEffect;   // clear PCHasEffect flag
            // copy data to object
            memcpy(&mgefFlags,&data,sizeof(data));
            // OBME added fields
            if (loadVersion == VERSION_VANILLA_OBLIVION)
            {
                // clear obme flags
                mgefObmeFlags = 0;
                // set default hostility (since hostile flag can't be overriden by vanilla records anyway)
                SetHostility(GetDefaultHostility(mgefCode));
            }
            else if (loadVersion <= VERSION_OBME_LASTUNVERSIONED)
            {
                // no fields yet that weren't in DATX chunk
            }
            else if (loadVersion <= VERSION_OBME_LASTV1)
            {
                // no fields yet that weren't in OBME v1 chunk
            }
            // done
            loadFlags |= kMgefChunk_Data;
            _DMESSAGE("DATA chunk: flags {%08X}",mgefFlags);
            break;

        // counter effects by code
        case 'ESCE':            
            if (loadFlags & kMgefChunk_Data)
            {
                if (numCounters > 0)
                {
                    // a data chunk is required in order to know size of counter array
                    UInt32* counters = new UInt32[numCounters];
                    memset(counters,0,numCounters * sizeof(UInt32)); 
                    file.GetChunkData(counters,numCounters * sizeof(UInt32));
                    SetCounterEffects(numCounters,counters);
                    // resolve counter effect codes
                    for (int c = 0; c < numCounters; c++) ResolveModValue(counterArray[c],file,kResolutionType_MgefCode);
                }
                // done
                loadFlags |= kMgefChunk_Esce;
                _DMESSAGE("ESCE chunk: %i counters",numCounters);
            }
            else 
            {
                gLog.PushStyle();
                _ERROR("Unexpected chunk 'ESCE' - must come after chunk 'DATA'");
                gLog.PopStyle();
            }
            break;

        // effect handler, added in v2 (so version is guaranteed > VERSION_OBME_LASTV1)
        case 'EHND':
            if (loadFlags & kMgefChunk_Obme)
            {
                EffectSettingHandler* handler = EffectSettingHandler::Create(_handlerCode,*this);  // attempt to create handler from _handlerCode
                if (!handler)
                {
                    // invalid handler code
                    gLog.PushStyle();
                    _ERROR("Unrecognized EffectHandler code '%4.4s' (%08X)", &_handlerCode, _handlerCode); 
                    gLog.PopStyle();
                } 
                SetHandler(handler); // set handler to new handler, or a default if the new handler code was invalid
                GetHandler().LoadHandlerChunk(file);
                loadFlags |= kMgefChunk_Ehnd;
                _DMESSAGE("EHND chunk: handler '%s' {%08X}",GetHandler().HandlerName(),GetHandler().HandlerCode()); 
            }
            else 
            {
                gLog.PushStyle();
                _ERROR("Unexpected chunk 'EHND' - must come after chunk 'OBME'");
                gLog.PopStyle();
            }
            break;

        // unrecognized chunk type
        default:
            gLog.PushStyle();
            _WARNING("Unexpected chunk '%4.4s' {%08X} w/ size %08X", &chunktype, chunktype, file.currentChunk.chunkLength);
            gLog.PopStyle();
            break;

        }
        // continue to next chunk
    } 

    // handle case of missing counter list
    if ((loadFlags & kMgefChunk_Data) && (~loadFlags & kMgefChunk_Esce) && numCounters)
    {
        gLog.PushStyle();
        _ERROR("Expected chunk 'ESCE' with %i counter effects not found", numCounters);
        numCounters = 0;    // zero out number of counters
        gLog.PopStyle();
    }

    // Handle case of missing DATA chunk with OBME/DATX chunks
    if ((loadFlags & (kMgefChunk_Datx | kMgefChunk_Obme)) && (~loadFlags & kMgefChunk_Data))
    {
        gLog.PushStyle();
        _ERROR("Expected chunk 'DATA' not found - records with 'OBME' or 'DATX' chunk must contain a 'DATA' chunk as well");
        gLog.PopStyle();
    }

    // if no EDDX chunk loaded, use mgefCode as editorID
    if (~loadFlags & kMgefChunk_Eddx)
    {
        char codestr[5] = {{0}};
        *(UInt32*)codestr = mgefCode;
        SetEditorID(codestr);
        _DMESSAGE("Set EditorID: '%s'",GetEditorID());
    }

    // if missing EHND chunk, construct a default handler
    if ((~loadFlags & kMgefChunk_Ehnd) && (loadFlags & (kMgefChunk_Obme | kMgefChunk_Datx | kMgefChunk_Data)))
    {
        EffectSettingHandler* handler = EffectSettingHandler::Create(_handlerCode,*this);  // attempt to create handler from _handlerCode
        if (!handler)
        {
            // invalid handler code
            gLog.PushStyle();
            _ERROR("Unrecognized EffectHandler code '%4.4s' (%08X)", &_handlerCode, _handlerCode); 
            gLog.PopStyle();
        } 
        SetHandler(handler); // set handler to new handler, or a default if the new handler code was invalid
        // TODO - use _handlerParamA, _handlerParamB, _handlerFlags, depending on handler and version
        _DMESSAGE("Created Handler: '%s' {%08X}",GetHandler().HandlerName(),GetHandler().HandlerCode()); 
    }

    // done
    gLog.Outdent();
    return true;
}
void EffectSetting::SaveFormChunks()
{
    bool requiresOBME = RequiresObmeMgefChunks();
    _VMESSAGE("Saving effect '%s' {%08X} '%s'/%08X (requires OBME chunks = %i)", name.c_str(), mgefCode, GetEditorID(), formID, requiresOBME); 
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
        obme.obmeRecordVersion = OBME_RECORD_VERSION(OBME_MGEF_VERSION);
        obme.effectHandler = GetHandler().HandlerCode();
        if (mgefFlags & (kMgefFlag_UseWeapon | kMgefFlag_UseArmor | kMgefFlag_UseActor))
        {
            obme._mgefParamResType = kResolutionType_FormID; // param is a summoned formID
        }
        else if (mgefFlags & kMgefFlag_UseActorValue)
        {
            obme._mgefParamResType = kResolutionType_EnumExtension; // param is a modified avCode
        }
        obme.mgefObmeFlags = mgefObmeFlags;
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
        _DMESSAGE("DATA chunk: flags {%08X}",mgefFlags);
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
    _VMESSAGE("Linking effect '%s' {%08X} '%s'/%08X", name.c_str(), mgefCode, GetEditorID(), formID); 
    ::EffectSetting::LinkForm();
    // link handler object
    GetHandler().LinkHandler();
}
void EffectSetting::CopyFrom(TESForm& copyFrom)
{  
    // convert form to mgef
    EffectSetting* mgef = (EffectSetting*)dynamic_cast<::EffectSetting*>(&copyFrom);
    if (!mgef)
    {
        _WARNING("Attempted copy from '%s' form '%s'/%08X",TESForm::GetFormTypeName(copyFrom.GetFormType()),copyFrom.GetEditorID(),copyFrom.formID);
        return;
    }
    _DMESSAGE("Copying ...");

    // copy effect code
    if (mgefCode != mgef->mgefCode)
    {
        if (formFlags & TESForm::kFormFlags_Temporary) mgefCode = mgef->mgefCode;   // this form is temporary, simply copy effect code
        else if (mgef->formFlags & TESForm::kFormFlags_Temporary)
        {
            // other form is temporary, but this one is not
            EffectSettingCollection::collection.RemoveAt(mgefCode); // unregister current code
            mgefCode = mgef->mgefCode; // copy code
            if (IsMgefCodeValid(mgefCode)) EffectSettingCollection::collection.SetAt(mgefCode,this); // register under new code
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
    EffectSettingHandler* newHandler = EffectSettingHandler::Create(mgef->GetHandler().HandlerCode(),*this);
    newHandler->CopyFrom(mgef->GetHandler());
    SetHandler(newHandler);
}
bool EffectSetting::CompareTo(TESForm& compareTo)
{    
    // convert form to mgef
    EffectSetting* mgef = (EffectSetting*)dynamic_cast<::EffectSetting*>(&compareTo);
    if (!mgef)
    {
        _WARNING("Attempted compare to '%s' form '%s'/%08X",TESForm::GetFormTypeName(compareTo.GetFormType()),compareTo.GetEditorID(),compareTo.formID);
        return true;
    }
    _DMESSAGE("Comparing ...");

    // compare resistances
    if (GetResistAV() != mgef->GetResistAV()) return true; // resistances don't match

    // compare counter effects
    if (numCounters != mgef->numCounters) return true; // counter effect count doesn't match
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
            if (!found)  return true; // unmatched counter effect
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
    if (noMatch) return true; // other base fields did not match    

    // compare flags
    if (mgefObmeFlags != mgef->mgefObmeFlags) return true;

    // compare handler
    return GetHandler().CompareTo(mgef->GetHandler());
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
    _DMESSAGE("");
    HWND ctl;

    // build tabs
    ctl = GetDlgItem(dialog,IDC_MGEF_TABS);
    for (UInt32 i = 0; i < kTabCount; i++) InsertTabSubwindow(dialog,ctl,i);
    // select first tab
    TabCtrl_SetCurSel(ctl,0);
    ShowTabSubwindow(dialog,0,true);

    // GENERAL
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
        // counter list columns
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        TESListView::ClearColumns(ctl);
        TESListView::ClearItems(ctl);
        SetupFormListColumns(ctl);
        SetWindowLong(ctl,GWL_USERDATA,1); // sorted by editorid
        // counter effect add list
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectSetting,true,false);

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
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)ActorValues::kActorVal__NONE);
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
        while (EffectSettingHandler::GetNextHandler(ehCode,ehName))
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

    switch (uMsg)
    {
    case WM_USERCOMMAND:
    case WM_COMMAND:
        UInt32 commandCode;
        commandCode = wParam >> 0x10;
        switch ((UInt16)wParam) // control ID
        {
        case IDC_MGEF_DYNAMICMGEFCODE:
            // dynamic code checkbox clicked
            _DMESSAGE("Dynamic code CM %i", commandCode);   
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
                return false;     
            }
            break;
        case IDC_MGEF_ADDCOUNTER:  
            _DMESSAGE("Add counter effect CM %i", commandCode);
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
                return false;
            }
            break;
        case IDC_MGEF_REMOVECOUNTER:
            _DMESSAGE("Remove counter effect CM %i", commandCode);
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Remove counter effect");  
                ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
                for (int i = ListView_GetNextItem(ctl,-1,LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(ctl,i-1,LVNI_SELECTED))
                {
                    ListView_DeleteItem(ctl,i);
                }
                return false;
            }
            break;
        case IDC_MGEF_HANDLER:
            _DMESSAGE("Handler Combo CM %i", commandCode);
            if (commandCode == CBN_SELCHANGE)
            {
                _VMESSAGE("Handler changed");
                ctl = GetDlgItem(dialog,IDC_MGEF_HANDLER);
                // create a new handler if necessary
                UInt32 ehCode;
                ehCode = (UInt32)TESComboBox::GetCurSelData(ctl);
                if (ehCode != GetHandler().HandlerCode()) SetHandler(EffectSettingHandler::Create(ehCode,*this));
                // change dialog templates if necessary
                DialogExtraSubwindow* extraSubwindow = (DialogExtraSubwindow*)GetWindowLong(ctl,GWL_USERDATA);
                if (extraSubwindow && GetHandler().DialogTemplateID() != extraSubwindow->dialogTemplateID)
                {
                    _DMESSAGE("Destroying current handler subwindow <%p> ...",extraSubwindow->subwindow);
                     ExtraDataList_RemoveData(TESDialog::GetDialogExtraList(dialog),extraSubwindow,true);
                     extraSubwindow = 0;
                }
                // create a new subwindow if necessary
                if (GetHandler().DialogTemplateID() && !extraSubwindow)
                {                    
                    TESDialog::Subwindow* subwindow = new TESDialog::Subwindow;                    
                    _DMESSAGE("Creating new handler subwindow <%p> ... ",subwindow);
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
                    _DMESSAGE("Attaching handler subwindow to dialog ...");
                    TESDialog::BuildSubwindow(GetHandler().DialogTemplateID(),subwindow);
                    // add subwindow extra data to parent
                    extraSubwindow = TESDialog::AddDialogExtraSubwindow(dialog,GetHandler().DialogTemplateID(),subwindow);                    
                }
                _DMESSAGE("Caching handler subwindow data <%p> ...",extraSubwindow);
                // store current dialog template in handler selection window UserData
                SetWindowLong(ctl,GWL_USERDATA,(UInt32)extraSubwindow);
                // set handler in dialog
                GetHandler().SetInDialog(dialog);
                return false;
            }
            break;
        }
        break;
    case WM_NOTIFY:       
        NMHDR* nmhdr;
        nmhdr = (NMHDR*)lParam;
        switch (nmhdr->idFrom)
        {
        case IDC_MGEF_TABS:
            // Notification from tab control
            _DMESSAGE("Tab Control NM %i",nmhdr->code);
            if (nmhdr->code == TCN_SELCHANGING)
            {
                // current tab is about to be unselected
                _VMESSAGE("Tab hidden");
                ShowTabSubwindow(dialog,TabCtrl_GetCurSel(nmhdr->hwndFrom),false);
                return false;
            }
            else if (nmhdr->code == TCN_SELCHANGE)
            {
                // current tab was just selected
                _VMESSAGE("Tab Shown");
                ShowTabSubwindow(dialog,TabCtrl_GetCurSel(nmhdr->hwndFrom),true);  
                return false;
            }
            break;
        }
        break;
    }
    return ::EffectSetting::DialogMessageCallback(dialog,uMsg,wParam,lParam,result);
}
void EffectSetting::SetInDialog(HWND dialog)
{
    _DMESSAGE("");
    if (!dialogHandle)
    {
        // first time initialization
        InitializeDialog(dialog);
        dialogHandle = dialog;
    }

    HWND ctl;
    char buffer[0x50];
    
    // GENERAL
        // effect code
        CheckDlgButton(dialog,IDC_MGEF_DYNAMICMGEFCODE,IsMgefCodeDynamic(mgefCode));
        _DMESSAGE("Sending check box click cmd ...");         
        SendNotifyMessage(dialog, WM_USERCOMMAND,(BN_CLICKED << 0x10) | IDC_MGEF_DYNAMICMGEFCODE,  // trigger BN_CLICKED event
                                    (LPARAM)GetDlgItem(dialog,IDC_MGEF_DYNAMICMGEFCODE));
        // school
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_SCHOOL),(void*)school);
        // effect item description callback
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK),0/*TODO*/);
        // counter effects
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        TESListView::ClearItems(ctl);
        for (int i = 0; i < numCounters; i++)
        {
            TESListView::InsertItem(ctl,EffectSetting::LookupByCode(counterArray[i]));
        }
        ListView_SortItems(ctl,TESFormIDListView::FormListComparator,GetWindowLong(ctl,GWL_USERDATA)); // (re)sort items
        // counter effect add list
        TESComboBox::SetCurSel(GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES),0);

    // DYNAMICS
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

    // FX
        // Light, Hit + Enchant shaders
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_LIGHT),light);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_EFFECTSHADER),effectShader);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_ENCHANTSHADER),enchantShader);
        // Sounds
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_CASTINGSOUND),castingSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_BOLTSOUND),boltSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HITSOUND),hitSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_AREASOUND),areaSound);   

    // HANDLER
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HANDLER),(void*)GetHandler().HandlerCode()); 
        SendNotifyMessage(dialog, WM_USERCOMMAND,(CBN_SELCHANGE << 0x10) | IDC_MGEF_HANDLER,  // trigger CBN_SELCHANGED event
                                    (LPARAM)GetDlgItem(dialog,IDC_MGEF_HANDLER));

    // FLAGS
        for (int i = 0; i < 0x40; i++) CheckDlgButton(dialog,IDC_MGEF_FLAGEXBASE + i,GetFlag(i));

    // BASES
    ::TESFormIDListView::SetInDialog(dialog);
}
void EffectSetting::GetFromDialog(HWND dialog)
{
    _DMESSAGE("");

    // call base method - retrieves most vanilla effectsetting fields, and calls GetFromDialog for components
    // will fail to retrieve AssocItem and Flags, as the control IDs for these fields have been changed
    UInt32 resistance = resistAV;
    ::EffectSetting::GetFromDialog(dialog);

    HWND ctl;
    char buffer[0x50];

    // GENERAL
        // effect code
        GetWindowText(GetDlgItem(dialog,IDC_MGEF_MGEFCODE),buffer,sizeof(buffer));
        mgefCode = strtoul(buffer,0,16);  // TODO - validation?
        // effect item description callback
        /*TODO = */ TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK));
        // counter effects
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        if (counterArray) MemoryHeap::FormHeapFree(counterArray);  // clear current counter array
        if (numCounters = ListView_GetItemCount(ctl))
        {      
            // build array of effect codes
            counterArray = (UInt32*)MemoryHeap::FormHeapAlloc(numCounters * 4);
            for (int i = 0; i < numCounters; i++)
            {
                EffectSetting* mgef = (EffectSetting*)TESListView::GetItemData(ctl,i);
                if (mgef) counterArray[i] = mgef->mgefCode;
            }
        }

    // DYNAMICS
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

    // HANDLER
        UInt32 ehCode = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_HANDLER));
        if (ehCode != GetHandler().HandlerCode()) SetHandler(EffectSettingHandler::Create(ehCode,*this));          
        GetHandler().GetFromDialog(dialog);

    // FLAGS
    for (int i = 0; i < 0x40; i++) {if (GetDlgItem(dialog,IDC_MGEF_FLAGEXBASE + i)) { SetFlag(i,IsDlgButtonChecked(dialog,IDC_MGEF_FLAGEXBASE + i)); }}

}
void EffectSetting::CleanupDialog(HWND dialog)
{
    _DMESSAGE("");
    ::EffectSetting::CleanupDialog(dialog);
}
void EffectSetting::SetupFormListColumns(HWND listView)
{
    _DMESSAGE("");
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
    case 'WL': // weakness
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
    // mgefCode - must be vanilla
    if (!IsMgefCodeVanilla(mgefCode)) return true;
    _DMESSAGE("Code is Vanilla");

    // school - must be a vanilla magic school
    if (school > Magic::kSchool__MAX) return true;
    _DMESSAGE("School is Vanilla");

    // resistanceAV - must be a vanilla AV
    if (GetResistAV() > ActorValues::kActorVal__MAX && GetResistAV() != ActorValues::kActorVal__NONE) return true;
    _DMESSAGE("ResistAV is Vanilla");
    
    // editorID - must be the mgefCode reinterpreted as a string
    char codestr[5] = {{0}};
    *(UInt32*)codestr = mgefCode;  
    if (GetEditorID() == 0 || strcmp(GetEditorID(),codestr) != 0) return true;
    _DMESSAGE("EditorID is Vanilla");

    // counter array - can contain no non-vanilla effects
    for (int i = 0; counterArray && i < numCounters; i++)
    {
        if (!IsMgefCodeVanilla(counterArray[i])) return true;
    }
    _DMESSAGE("Counter Effects are Vanilla");

    // OBME flags (all except beneficial must be zero)
    if (mgefObmeFlags & ~kMgefObmeFlag_Beneficial) return true;
    _DMESSAGE("Unused OBME flags are Vanilla");

    // beneficial flag - must be zero unless default hostility is beneficial
    if ((mgefObmeFlags & kMgefObmeFlag_Beneficial) && GetDefaultHostility(mgefCode) != Magic::kHostility_Beneficial) return true;
    _DMESSAGE("Beneficial flag is Vanilla");

    // Effect Handler - must differ from default handler object
    UInt32 defHandlerCode = EffectSettingHandler::GetDefaultHandlerCode(mgefCode);
    if (defHandlerCode != GetHandler().HandlerCode()) return true;
    EffectSettingHandler* defHandler = EffectSettingHandler::Create(defHandlerCode,*this);
    bool handlerNoMatch = defHandler->CompareTo(GetHandler());
    delete defHandler;
    _DMESSAGE("Handler is %i vanilla",!handlerNoMatch);
    return handlerNoMatch;
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
    if (resistAV == ActorValues::kActorVal__MAX || resistAV == 0xFFFFFFFF) return ActorValues::kActorVal__NONE;
    else return resistAV;
}
inline void EffectSetting::SetResistAV(UInt32 newResistAV)
{
    if (newResistAV == ActorValues::kActorVal__MAX || newResistAV == 0xFFFFFFFF) resistAV = ActorValues::kActorVal__NONE;
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
// effect table
bool EffectSetting::IsMgefCodeValid(UInt32 mgefCode)
{
    return (mgefCode != kMgefCode_None && mgefCode != kMgefCode_Invalid);
}
bool EffectSetting::IsMgefCodeDynamic(UInt32 mgefCode)
{
    return (mgefCode & kMgefCode_DynamicFlag);
}

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
bool EffectSetting::ResolveModMgefCode(UInt32& mgefCode, TESFile& file)
{
    // returns invalid code on failure
    if (!IsMgefCodeValid(mgefCode)) return false;        
    if (!IsMgefCodeDynamic(mgefCode)) return true;   // code is not dynamic
    UInt32 fmid = (mgefCode << 0x18) | 0x950;                   // use a dummy formid
    if (ResolveFormID(fmid,file)) 
    {
        mgefCode = (mgefCode & 0xFFFFFF00) | (fmid >> 0x18);
        return IsMgefCodeValid(mgefCode);
    }
    else 
    {
        mgefCode = kMgefCode_Invalid;
        return false;
    }    
}
// effect handler
EffectSettingHandler& EffectSetting::GetHandler()
{
    if (!effectHandler)
    {
        // this effect has no handler, create a default handler
        effectHandler = EffectSettingHandler::Create(EffectHandlerBase::GetDefaultHandlerCode(mgefCode),*this);
        if (!effectHandler) effectHandler = EffectSettingHandler::Make(*this);  // default handler not yet implemented - should never happen in release version
    }
    return *effectHandler;
}
void EffectSetting::SetHandler(EffectSettingHandler* handler)
{
    if (effectHandler) delete effectHandler;    // clear current handler
    if (!handler)
    {
        // no handler specified, create a default handler
        handler = EffectSettingHandler::Create(EffectHandlerBase::GetDefaultHandlerCode(mgefCode),*this);
        if (!handler) handler = EffectSettingHandler::Make(*this);  // default handler not yet implemented - should never happen in release version
    }
    effectHandler = handler;
}
// constructor, destructor
EffectSetting::EffectSetting()
{    
    _DMESSAGE("Constructing Mgef <%p>",this);    
    if (static bool needsVtblOverride = true)
    {
        needsVtblOverride = false;
        _VMESSAGE("Patching game/CS vtbl ...");
        // runs first time an OBME::EffectSetting is constructed, & patches game vtbl
        memaddr obmeVtbl((UInt32)memaddr::GetObjectVtbl(this));
        memaddr oDtor(0x10,0x34);       EffectSetting_Vtbl.SetVtblEntry(oDtor,obmeVtbl.GetVtblEntry(oDtor)); // ~EffectSetting();
        memaddr oLoadForm(0x1C,0x40);   EffectSetting_Vtbl.SetVtblEntry(oLoadForm,obmeVtbl.GetVtblEntry(oLoadForm)); // LoadForm
        memaddr oSaveForm(0x24,0x48);   EffectSetting_Vtbl.SetVtblEntry(oSaveForm,obmeVtbl.GetVtblEntry(oSaveForm)); // SaveForm
        memaddr oLinkForm(0x6C,0x70);   EffectSetting_Vtbl.SetVtblEntry(oLinkForm,obmeVtbl.GetVtblEntry(oLinkForm)); // LinkForm
        memaddr oCopyFrom(0xB4,0xB8);   EffectSetting_Vtbl.SetVtblEntry(oCopyFrom,obmeVtbl.GetVtblEntry(oCopyFrom)); // CopyFrom
        memaddr oCompareTo(0xB8,0xBC);  EffectSetting_Vtbl.SetVtblEntry(oCompareTo,obmeVtbl.GetVtblEntry(oCompareTo)); // CompareTo
        #ifndef OBLIVION
        EffectSetting_Vtbl.SetVtblEntry(0x10C,obmeVtbl.GetVtblEntry(0x10C)); // DialogMessageCallback
        EffectSetting_Vtbl.SetVtblEntry(0x114,obmeVtbl.GetVtblEntry(0x114)); // SetInDialog
        EffectSetting_Vtbl.SetVtblEntry(0x118,obmeVtbl.GetVtblEntry(0x118)); // GetFromDialog
        EffectSetting_Vtbl.SetVtblEntry(0x11C,obmeVtbl.GetVtblEntry(0x11C)); // CleanupDialog
        EffectSetting_Vtbl.SetVtblEntry(0x124,obmeVtbl.GetVtblEntry(0x124)); // SetupFormListColumns
        EffectSetting_Vtbl.SetVtblEntry(0x130,obmeVtbl.GetVtblEntry(0x130)); // GetFormListDispInfo
        EffectSetting_Vtbl.SetVtblEntry(0x134,obmeVtbl.GetVtblEntry(0x134)); // CompareInFormList
        #endif  
    }
    // replace compiler-generated vtbl with existing game vtbl
    EffectSetting_Vtbl.SetObjectVtbl(this);
    // game constructor uses some odd initialization values
    // NOTE: be *very* careful about changing default initializations
    // the game occassionally uses a default-constructed EffectSetting, and the default values are
    // hardcoded all over the place.  In particular,  a default-created effect is used as a comparison 
    // template for filtering effects before spellmaking, enchanting, etc.  Do NOT change the mgefFlags 
    // initialization from zero, or the default school from 6, as this will cause some effects to be
    // incorrectly filtered out.
    resistAV = ActorValues::kActorVal__NONE;    // standardize the 'no resistav' value
    mgefCode = kMgefCode_None;
    // initialize new fields
    effectHandler = 0; SetHandler(0); // default handler
    mgefObmeFlags = 0x0;
    loadFlags = 0xFFFFFFFF;   // all fields 'loaded'
}
EffectSetting::~EffectSetting()
{
    _DMESSAGE("Destructing Mgef <%p>",this);
    if (effectHandler) delete effectHandler;
}

// hooks
EffectSetting* EffectSetting_Create_Hndl()
{
    _DMESSAGE("Creating Mgef ...");
    return new EffectSetting;
}
EffectSetting* EffectSettingCollection_Add_Hndl(UInt32 mgefCode, const char* name, UInt32 school, float baseCost, UInt32 param, 
                                                UInt32 mgefFlags, UInt32 resistAV, UInt16 numCounters, UInt32 firstCounter)       
{   
    // Note: this function doesn't perform integrity checks on the mgefFlags like the vanilla code
    // clear existing effect
    EffectSetting* mgef = (EffectSetting*)EffectSettingCollection::LookupByCode(mgefCode);
    if (mgef) delete mgef;
    // create new effect
    mgef = new EffectSetting; 
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
    mgef->numCounters = numCounters;
    // get default hostility
    mgef->SetHostility(EffectSetting::GetDefaultHostility(mgefCode));
    // get default handler
    mgef->SetHandler(0);
    // done
    _DMESSAGE("Adding Mgef to Collection: '%s' {%08X} '%4.4s'/%08X w/ host %i, handler '%s'", 
        name, mgefCode, namebuffer, mgef->formID, mgef->GetHostility(),mgef->GetHandler().HandlerName());
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
    _DMESSAGE("mgef <%p>",mgef);
    if (!EffectSetting::IsMgefCodeValid(mgef->mgefCode))
    {
        // mgef needs to be given a valid code
        mgef->mgefCode = EffectSetting::GetUnusedDynamicCode();
    }
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
            ResolveModValue(mgefCode,*file,kResolutionType_MgefCode);
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
                    _ERROR("Cannot load MGEF record into existing %s Form '%s'/%08X",
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

    // TODO - hook loading of palyer's known effect list to resolve effect codes

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