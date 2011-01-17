/*
    Commands header for OBME loader
    Interface between main loader unit and commands execution unit
*/
#pragma once

#include "obse/PluginAPI.h"
#include "OBME/OBME_interface.h"

// global OBSE interface objects
extern PluginHandle                 g_pluginHandle;
extern const OBSEInterface*         g_obseIntfc;
extern OBSEArrayVarInterface*       g_arrayIntfc;
extern OBSEStringVarInterface*      g_stringIntfc;
extern OBSEScriptInterface*         g_scriptIntfc;
extern OBSECommandTableInterface*   g_cmdIntfc;

// global obme interface
extern OBME_Interface*              g_obmeIntfc;

// OBME opcodes
const int OBME_OPCODEBASE       = 0x2600;
const int OBME_OPCODEBASE_MGEF  = 0x2600;
const int OBME_OPCODEBASE_EFIT  = 0x2610;
const int OBME_OPCODEMAX        = 0x265F;
const int OBME_OPCODEBASE_DEBUG = 0x2700;

// registers new OBME commands
void Register_OBME_Commands();

// overwrites broken or incompatible OBSE commands
void Overwrite_OBSE_Commands();

// parses CSE console commands
void CSEPrintCallback(const char* Message, const char* Prefix);
