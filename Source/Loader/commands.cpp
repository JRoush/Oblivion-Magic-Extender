/*
    Commands execution unit for OBME loader.  Handles:
    -   Command registration
    -   Command execution through OBME interface
*/
#include "Loader/commands.h"

#include "obse/CommandTable.h"
#include "obse/ParamInfos.h"

/*--------------------------------------------------------------------------------------------*/
// Ported from CommandTable.cpp so we don't have to include the entire file
bool Cmd_Default_Execute(COMMAND_ARGS) {return true;} // nop command handler for script editor
bool Cmd_Default_Eval(COMMAND_ARGS_EVAL) {return true;} // nop command evaluator for script editor

// Test command
static ParamInfo kParams_ThreeOptionalStrings[3] =
{
	{	"stringA",	kParamType_String,	1 },
    {	"stringB",	kParamType_String,	1 },
    {	"stringC",	kParamType_String,	1 },
};
bool Cmd_obmeTest_Execute(COMMAND_ARGS)
{
    *result = 0;
    char bufferA[0x200] = {{0}};
    char bufferB[0x200] = {{0}};
    char bufferC[0x200] = {{0}};
    ExtractArgs(PASS_EXTRACT_ARGS, bufferA, bufferB, bufferC);
    g_obmeIntfc->privateCommands.OBMETest(thisObj,bufferA,bufferB,bufferC);
    return true;
}
DEFINE_COMMAND_PLUGIN(obmeTest, "test command, accepts 3 optional string arguments", 0, 3, kParams_ThreeOptionalStrings)

// EffectSetting Commands
/*
static ParamInfo kParams_SetMEHandlerIntParamC[3] = 
{
    {	"paramName", kParamType_String, 0 },
	{	"mgefCode", kParamType_Integer, 0 },
    {	"newValue", kParamType_Integer, 0 },
};
static ParamInfo kParams_SetMEHandlerRefParamC[3] = 
{
    {	"paramName", kParamType_String, 0 },
	{	"mgefCode", kParamType_Integer, 0 },
    {	"newValue", kParamType_InventoryObject, 0 },
};
DEFINE_COMMAND_PLUGIN(ResolveMgefCode, "Converts a saved Magic Effect code into a useable one", 0, 1, kParams_OneInt)
DEFINE_COMMAND_PLUGIN(CreateMgef, "Creates a new magic effect", 0, 2, kParams_OneInt_OneOptionalInt)
DEFINE_COMMAND_PLUGIN(GetMagicEffectHandlerC, "Get magic effect Effect Handler", 0, 1, kParams_OneInt)
DEFINE_COMMAND_PLUGIN(SetMagicEffectHandlerC, "Set magic effect Effect Handler", 0, 2, kParams_TwoInts)
DEFINE_COMMAND_PLUGIN(GetMagicEffectHostilityC, "Get magic effect hostility state", 0, 1, kParams_OneInt)
DEFINE_COMMAND_PLUGIN(SetMagicEffectHostilityC, "Set magic effect hostility state", 0, 2, kParams_TwoInts)
DEFINE_COMMAND_PLUGIN(GetMagicEffectHandlerParamC, "Get handler-specifc parameter by name", 0, 2, kParams_OneString_OneInt)
DEFINE_COMMAND_PLUGIN(SetMagicEffectHandlerIntParamC, "Set handler-specifc parameter by name", 0, 3, kParams_SetMEHandlerIntParamC)
DEFINE_COMMAND_PLUGIN(SetMagicEffectHandlerRefParamC, "Set handler-specifc parameter by name", 0, 3, kParams_SetMEHandlerRefParamC)
DEFINE_COMMAND_PLUGIN(OBME_GetMagicEffectCode, "", 0, 1, kParams_OneMagicEffect)
*/

// ActiveEffect commands
/*
DEFINE_COMMAND_PLUGIN(OBME_GetNthActiveEffectCode, "", 1, 1, kParams_OneInt)
*/

// EffectItem commands
/*
static ParamInfo kParams_AddEffectItem[2] = 
{
	{	"mgef", kParamType_MagicEffect, 0 },
	{	"toMagicItem", kParamType_MagicItem, 0 },
};
static ParamInfo kParams_AddEffectItemC[2] = 
{
	{	"mgefCode", kParamType_Integer, 0 },
	{	"toMagicItem", kParamType_MagicItem, 0 },
};
static ParamInfo kParams_AddFullEffectItem[6] = 
{
	{	"mgef", kParamType_MagicEffect, 0 },
	{	"magnitude", kParamType_Integer, 0 },
	{	"area", kParamType_Integer, 0 },
	{	"duration", kParamType_Integer, 0 },
	{	"range", kParamType_Integer, 0 },
	{	"toMagicItem", kParamType_MagicItem, 0 },
};
static ParamInfo kParams_AddFullEffectItemC[6] = 
{
	{	"mgefCode", kParamType_Integer, 0 },
	{	"magnitude", kParamType_Integer, 0 },
	{	"area", kParamType_Integer, 0 },
	{	"duration", kParamType_Integer, 0 },
	{	"range", kParamType_Integer, 0 },
	{	"toMagicItem", kParamType_MagicItem, 0 },
};
static ParamInfo kParams_GetNthEI[2] = 
{
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
static ParamInfo kParams_SetNthEIString[3] = 
{
	{	"string", kParamType_String, 0 },
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
static ParamInfo kParams_SetNthEIInt[3] = 
{
	{	"int", kParamType_Integer, 0 },
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
static ParamInfo kParams_SetNthEIFloat[3] = 
{
	{	"float", kParamType_Float, 0 },
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
static ParamInfo kParams_GetNthEIHandlerParam[3] = 
{
    {	"paramName", kParamType_String, 0 },
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
static ParamInfo kParams_SetNthEIHandlerIntParam[4] = 
{
    {	"paramName", kParamType_String, 0 },    
    {	"newValue", kParamType_Integer, 0 },
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
static ParamInfo kParams_SetNthEIHandlerRefParam[4] = 
{
    {	"paramName", kParamType_String, 0 },    
    {	"newValue", kParamType_InventoryObject, 0 },
	{	"magic item", kParamType_MagicItem, 0 },
	{	"which effect", kParamType_Integer, 0 },
};
DEFINE_COMMAND_PLUGIN(GetNthEIEffectName, "Get the magic effect name of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEIEffectName, "Set the magic effect name override of the nth effect item", 0, 3, kParams_SetNthEIString)
DEFINE_COMMAND_PLUGIN(ClearNthEIEffectName, "Clear the magic effect name override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEIIconPath, "Get the icon path for the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEIIconPath, "Set the icon path override of the nth effect item", 0, 3, kParams_SetNthEIString)
DEFINE_COMMAND_PLUGIN(ClearNthEIIconPath, "Clear the icon path override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEIHostility, "Get the hostility for the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEIHostility, "Set the hostility override of the nth effect item", 0, 3, kParams_SetNthEIInt)
DEFINE_COMMAND_PLUGIN(ClearNthEIHostility, "Clear the hostility override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEIBaseCost, "Get the base cost for the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEIBaseCost, "Set the base cost override of the nth effect item", 0, 3, kParams_SetNthEIFloat)
DEFINE_COMMAND_PLUGIN(ClearNthEIBaseCost, "Clear the base cost override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEIResistAV, "Get the resistance AV for the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEIResistAV, "Set the resistance AV override of the nth effect item", 0, 3, kParams_SetNthEIInt)
DEFINE_COMMAND_PLUGIN(ClearNthEIResistAV, "Clear the resistance AV override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEISchool, "Get the school for the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEISchool, "Set the school override of the nth effect item", 0, 3, kParams_SetNthEIInt)
DEFINE_COMMAND_PLUGIN(ClearNthEISchool, "Clear the school override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEIVFXCode, "Get the VFX code for the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(SetNthEIVFXCode, "Set the VFX override of the nth effect item", 0, 3, kParams_SetNthEIInt)
DEFINE_COMMAND_PLUGIN(ClearNthEIVFXCode, "Clear the VFX override of the nth effect item", 0, 2, kParams_GetNthEI)
DEFINE_COMMAND_PLUGIN(GetNthEIHandlerParam, "Get handler-specifc parameter by name", 0, 3, kParams_GetNthEIHandlerParam)
DEFINE_COMMAND_PLUGIN(SetNthEIHandlerIntParam, "Set handler-specifc parameter by name", 0, 4, kParams_SetNthEIHandlerIntParam)
DEFINE_COMMAND_PLUGIN(SetNthEIHandlerRefParam, "Set handler-specifc parameter by name", 0, 4, kParams_SetNthEIHandlerRefParam)
DEFINE_COMMAND_PLUGIN(ClearNthEIHandlerParam, "Get handler-specifc parameter by name", 0, 3, kParams_GetNthEIHandlerParam)
DEFINE_COMMAND_PLUGIN(OBME_AddEffectItem, "", 0, 2, kParams_AddEffectItem)
DEFINE_COMMAND_PLUGIN(OBME_AddEffectItemC, "", 0, 2, kParams_AddEffectItemC)
DEFINE_COMMAND_PLUGIN(OBME_AddFullEffectItem, "", 0, 6, kParams_AddFullEffectItem)
DEFINE_COMMAND_PLUGIN(OBME_AddFullEffectItemC, "", 0, 6, kParams_AddFullEffectItemC)
DEFINE_COMMAND_PLUGIN(OBME_GetNthEffectItemCode, "", 0, 2, kParams_GetNthEI)
*/

// Magic Item Commands 
/*
DEFINE_COMMAND_PLUGIN(OBME_GetMagicItemCost, "", 0, 1, kParams_OneMagicItem)
DEFINE_COMMAND_PLUGIN(OBME_GetSpellMagickaCost, "", 0, 1, kParams_OneSpellItem)
DEFINE_COMMAND_PLUGIN(OBME_GetSpellMasteryLevel, "", 0, 1, kParams_OneSpellItem)
DEFINE_COMMAND_PLUGIN(OBME_GetSpellSchool, "", 0, 1, kParams_OneSpellItem)
*/

// command registration
void Register_OBME_Commands()
{
    // OBME commands - do NOT change the order once released
    _MESSAGE("Registering OBME Commands from opcode base %04X ...", OBME_OPCODEBASE);
    /*
    // effect setting commands
    g_obseIntfcIntfc->SetOpcodeBase(OBME_OPCODEBASE_MGEF); 
    g_obseIntfc->RegisterCommand(&kCommandInfo_ResolveMgefCode);                               // 2600
    g_obseIntfc->RegisterCommand(&kCommandInfo_CreateMgef);                                    // 2601
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetMagicEffectHandlerC);                        // 2602
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetMagicEffectHandlerC);                        // 2603    
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetMagicEffectHostilityC);                      // 2604
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetMagicEffectHostilityC);                      // 2605
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetMagicEffectHandlerParamC);                   // 2606
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetMagicEffectHandlerIntParamC);                // 2607
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetMagicEffectHandlerRefParamC);                // 2608
        // Get(/Set)QualifiedNameString ...
        // Additional flags ...
    // effect item commands
    g_obseIntfc->SetOpcodeBase(OBME_OPCODEBASE_EFIT);        
    g_obseIntfc->RegisterTypedCommand(&kCommandInfo_GetNthEIEffectName, kRetnType_String);     // 2610
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIEffectName);                            // 2611
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIEffectName);                          // 2612
    g_obseIntfc->RegisterTypedCommand(&kCommandInfo_GetNthEIIconPath, kRetnType_String);       // 2613
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIIconPath);                              // 2614
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIIconPath);                            // 2615                   
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetNthEIHostility);                             // 2616
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIHostility);                             // 2617
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIHostility);                           // 2618
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetNthEIBaseCost);                              // 2619
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIBaseCost);                              // 261A
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIBaseCost);                            // 261B    
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetNthEIResistAV);                              // 261C
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIResistAV);                              // 261D
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIResistAV);                            // 261E
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetNthEISchool);                                // 261F
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEISchool);                                // 2620
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEISchool);                              // 2621
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetNthEIVFXCode);                               // 2622
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIVFXCode);                               // 2623
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIVFXCode);                             // 2624
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetNthEIHandlerParam);                          // 2625
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIHandlerIntParam);                       // 2626
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetNthEIHandlerRefParam);                       // 2627
    g_obseIntfc->RegisterCommand(&kCommandInfo_ClearNthEIHandlerParam);                        // 2628
        // ... for flag bits  
    // magic item commands
        // GetMagicItemTypeEx                                   // 2630 ...
        // ... meta magic commands ...
    // active effect commands
        // Get(/Set/Mod)NthActiveEffectMagicka                  // 2650 ...
    // magic target, magic caster, misc commands
        // ...                                                  // 2658 ...
    // debugging commands - generally intended for the console
    */
    #ifdef _DEBUG
     _MESSAGE("Registering Debugging Command from opcode base %04X ...", OBME_OPCODEBASE_DEBUG);
	 g_obseIntfc->SetOpcodeBase(OBME_OPCODEBASE_DEBUG);
     g_obseIntfc->RegisterCommand(&kCommandInfo_obmeTest);
    #endif
}

void Overwrite_OBSE_Commands()
{
    // TODO: add this capability, updated for obse v0019/0020
}

/*--------------------------------------------------------------------------------------------*/
// CSE command parsing
void CSEPrintCallback(const char* message, const char* prefix)
{/* 
    called whenever output is provided to CSE console, if present
    commands recognized by this parser take the form 'solutionname commandName [args...]'
    the only command for this example plugin is 'Description', which prints a description of the plugin to the log
*/
    char* context = 0;
    if (!message || _stricmp(strtok_s(const_cast<char*>(message)," \t",&context),"OBME") != 0) return; // output is not a command targeted to this plugin
    _DMESSAGE("%s %s",prefix,message);
    
    const char* command = strtok_s(0," \t",&context);
    const char* argA = strtok_s(0," \t",&context);
    const char* argB = strtok_s(0," \t",&context);
    const char* argC = strtok_s(0," \t",&context);
    if (_stricmp(command,"description") == 0)    // 'description' command
    {
        _MESSAGE("%s",g_obmeIntfc->privateCommands.Description());
    }
    else if (_stricmp(command,"test") == 0)    // 'test' command
    {        
        g_obmeIntfc->privateCommands.OBMETest(0,argA,argB,argC);
    }
}