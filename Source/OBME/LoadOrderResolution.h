/*
    Common definitions for encoding resolution info with saved data in mod files

    TODO - move this into a COEF component?
*/
#pragma once

// argument classes
class   TESFile;

namespace OBME {

    enum ResolutionTypes
    {
        kResolutionType_None          = 0x00,   // data does nto depend on load order
        kResolutionType_FormID        = 0x01,   // highest byte is index in load order, iif >= 0x800
        kResolutionType_MgefCode      = 0x02,   // lowest byte is index in load order, iif >= 0x080000000
        kResolutionType_EnumExtension = 0x03,   // treated the same as formIDs, but values < 0x800 are static enumeration codes
    };

    bool ResolveModValue(UInt32& value, TESFile& file, UInt32 resolutionType); // resolve value from mod file

}   //  end namespace OBME