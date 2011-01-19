/*
    OBME version info

    NOTE: For simplicity, the current OBME_Interface version is hardcoded into OBME_Interface.h.  Be sure
    to update that version whenever the major/minor/beta version of OBME itself changes.     
*/
#pragma once

//  General version control for OBME
const int OBME_MAJOR_VERSION    = 0x02;     // Major version
const int OBME_MINOR_VERSION    = 0x00;     // Minor version (release)
const int OBME_BETA_VERSION     = 0x01;     // Minor version (alpha/beta), FF is post-beta
const int OBME_COSAVE_VERSION   = 0x00;     // Cosave record version
const int OBME_ACTVEFF_VERSION  = 0x00;     // ActiveEffect save version
const int OBME_MGEF_VERSION     = 0x00;     // EffectSetting save version
const int OBME_EFIT_VERSION     = 0x00;     // EffectItem save version
const int OBME_MGIT_VERSION     = 0x00;     // MagicItem save version

// macros for combining version info into one 32bit number.  Pass zero for overall build version
#define MAKE_VERSION(major,minor,beta,record) (((UInt8)major << 0x18) | ((UInt8)minor << 0x10) | ((UInt8)beta << 0x08) | (UInt8)record)
#define OBME_VERSION(recordT)                MAKE_VERSION(OBME_MAJOR_VERSION,OBME_MINOR_VERSION,OBME_BETA_VERSION,recordT)

#define VERSION_VANILLA_OBLIVION        0x00000000  // indicates an object initialized by vanilla code
#define VERSION_OBME_LASTUNVERSIONED    MAKE_VERSION(1,0,3,0xFF)  // last version of OBME to use old/no versioning scheme (v1.beta3)