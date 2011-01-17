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
#define OBME_RECORD_VERSION(recordT)    ((OBME_MAJOR_VERSION << 0x18) | (OBME_MINOR_VERSION << 0x10) | \
                                        (OBME_BETA_VERSION << 0x08) | recordT)

#define VERSION_VANILLA_OBLIVION        0x00000000      // indicates an object initialized by vanilla code
#define VERSION_OBME_LASTUNVERSIONED    0x01000301      // last version of OBME to use old/no versioning scheme
#define VERSION_OBME_LASTV1             0x01FFFFFF      // version 1.x.x.x 
                