#include "OBME/LoadOrderResolution.h"
#include "API/TESFiles/TESFile.h" // TESForm::ResolveFormID()
#include "OBME/EffectSetting.h" // EffectSetting::ResolveModMgefCode();

namespace OBME {

bool ResolveModValue(UInt32& value, TESFile& file, UInt32 resolutionType)
{
    switch (resolutionType)
    {
    case kResolutionType_FormID:
        return TESForm::ResolveFormID(value,file);
    case kResolutionType_MgefCode:
        return EffectSetting::ResolveModMgefCode(value,file);
    case kResolutionType_EnumExtension:
        if (value < 0x800) return true; // enum values below 0x800 are static numeric constants, not formIDs
        else return TESForm::ResolveFormID(value,file);
    default:
        return true;
    }
}

}   //  end namespace OBME