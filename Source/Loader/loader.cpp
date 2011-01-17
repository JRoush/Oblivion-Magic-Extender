/*
    Main unit for OBME loader.  Handles:
    -   DLL entry point
    -   OBSE plugin interface
    -   OBSE,CSE messaging interface
    -   Serialization interface
    -   global debugging log
*/
#include "obse/PluginAPI.h"
#include "Loader/commands.h"
#include "OBME/OBME_interface.h"
#include "OBME/OBME_version.h"
#include "OBME/CSE_Interface.h"

/*--------------------------------------------------------------------------------------------*/
// global debugging log
OutputTarget*   _gLogFile = NULL;
_declspec(dllexport) OutputLog _gLog;
OutputLog&      gLog = _gLog;

/*--------------------------------------------------------------------------------------------*/
// global interfaces and handles
PluginHandle                    g_pluginHandle          = kPluginHandle_Invalid;    // identifier for this plugin, for internal use by obse
const OBSEInterface*            g_obseIntfc             = NULL; // master obse interface, for generating more specific intefaces below
OBSEArrayVarInterface*          g_arrayIntfc            = NULL; // obse array variable interface
OBSEStringVarInterface*         g_stringIntfc           = NULL; // obse string variable interface
OBSEScriptInterface*            g_scriptIntfc           = NULL; // obse script interface
OBSECommandTableInterface*      g_cmdIntfc              = NULL; // obse script command interface
OBSESerializationInterface*     g_serializationIntfc    = NULL; // obse cosave serialization interface
OBSEMessagingInterface*         g_messagingIntfc        = NULL; // obse inter-plugin messaging interface
HMODULE                         g_hExportInjector       = 0;    // windows module handle for ExportInjector library
HMODULE                         g_hSubmodule            = 0;    // windows module handle for submodule dll
OBME_Interface*                 g_obmeIntfc             = NULL; // obme interface
CSEInterface*                   g_cseIntfc              = NULL; // CSE master interface, if CSE is present
CSEConsoleInterface*            g_cseConsoleInfc        = NULL; // CSE console interface

/*--------------------------------------------------------------------------------------------*/
// CSE console output target
// this is a custom OutputTarget that forwards (plain text) output to the CSE console, if CSE is present 
class CSEConsoleTarget : public BufferTarget
{
public:
    // interface
    virtual void    WriteOutputLine(const OutputStyle& style, time_t time, int channel, const char* source, const char* text)
    {
        if (!g_cseConsoleInfc || !g_cseConsoleInfc->PrintToConsole) return;  // bad CSE console interface
        BufferTarget::WriteOutputLine(consoleStyle,time,channel,source,text); // generate output line, without timestamp or source
        g_cseConsoleInfc->PrintToConsole("OBME",LastOutputLine());  // write output line to CSE console
    }
    // static output style
    OutputStyle  consoleStyle;
} _CSETarget;

/*--------------------------------------------------------------------------------------------*/
// Serialization routines
static void SaveCallback(void * reserved)
{
    _MESSAGE("Writing to cosave ...");
    g_serializationIntfc->OpenRecord('HEAD',OBME_RECORD_VERSION(OBME_COSAVE_VERSION));
	g_serializationIntfc->WriteRecordData("OBME by JRoush", 14);
}
static void LoadCallback(void * reserved)
{
    _MESSAGE("Loading from cosave ...");
    UInt32	type, version, length;
	char	buf[512];
    gLog.Indent();
	while(g_serializationIntfc->GetNextRecordInfo(&type, &version, &length))
	{
        switch(type)
		{
			case 'HEAD':
				g_serializationIntfc->ReadRecordData(buf, length);
				buf[length] = 0;
                _DMESSAGE("  HEADER RECORD, Version(%08X): %s", version, buf);
				break;
			default:
                _DMESSAGE("  Record Type[%.4s] Version(%08X) Length(%08X)", &type, version, length);
				break;
		}
	}
    gLog.Outdent();
}
static void PreloadCallback(void * reserved)
{	
    _DMESSAGE("Preload Game callback ...");
}
static void NewGameCallback(void * reserved)
{
    _DMESSAGE("New Game callback ...");
}

/*--------------------------------------------------------------------------------------------*/
// Messaging API
void GeneralMessageHandler(OBSEMessagingInterface::Message* msg)
{
    if (msg->sender && strcmp(msg->sender,"CSE") == 0 && msg->type == 'CSEI')
    {
        // CSE interface message
        _VMESSAGE("Received CSE interface message");         
        g_cseIntfc = (CSEInterface*)msg->data;
        if (g_cseIntfc) g_cseConsoleInfc = (CSEConsoleInterface*)g_cseIntfc->InitializeInterface(CSEInterface::kCSEInterface_Console);
        if (g_obmeIntfc && g_cseConsoleInfc) 
        {
            _VMESSAGE("Attached to CSE console");
            gLog.AttachTarget(_CSETarget);   // attach CSE console target to output log
            _CSETarget.LoadRulesFromINI("Data\\obse\\Plugins\\OBME\\OBME.ini","CSEConsole.Log"); // load target rules for CSE console
            _CSETarget.consoleStyle.includeTime = _CSETarget.consoleStyle.includeSource = false; // setup output style for console
            g_cseConsoleInfc->RegisterCallback(CSEPrintCallback); // register parser for CSE console output
        }
        return;
    }
    _VMESSAGE("Received unknown message type=%i from '%s' w/ data <%p> len=%04X", msg->type, msg->sender, msg->data, msg->dataLen);
}
void OBSEMessageHandler(OBSEMessagingInterface::Message* msg)
{
    switch (msg->type)
	{
	case OBSEMessagingInterface::kMessage_ExitGame:
		_VMESSAGE("Received 'exit game' message");
		break;
	case OBSEMessagingInterface::kMessage_ExitToMainMenu:
		_VMESSAGE("Received 'exit game to main menu' message");
		break;
	case OBSEMessagingInterface::kMessage_PostLoad:
		_VMESSAGE("Received 'post-load' message"); 
		break;
	case OBSEMessagingInterface::kMessage_LoadGame:
	case OBSEMessagingInterface::kMessage_SaveGame:
		_VMESSAGE("Received 'save/load game' message with file path %s", msg->data);
		break;
	case OBSEMessagingInterface::kMessage_Precompile: 
		_VMESSAGE("Received 'pre-compile' message.");
		break;
	case OBSEMessagingInterface::kMessage_PreLoadGame:
		_VMESSAGE("Received 'pre-load game' message with file path %s", msg->data);
		break;
	case OBSEMessagingInterface::kMessage_ExitGame_Console:
		_VMESSAGE("Received 'quit game from console' message");
		break;
    case OBSEMessagingInterface::kMessage_PostLoadGame:
        _VMESSAGE("Received 'post-load game' message");
        break;
    case 9: // TODO - use appropriate constant
        _VMESSAGE("Received post-post-load message");
        if (g_obmeIntfc)
        {       
            // Dispatch OBME interface to all listeners
            _MESSAGE("Dispatching OBME interface to all listeners ... %i",
                g_messagingIntfc->Dispatch(g_pluginHandle,g_obmeIntfc->kMessageType,g_obmeIntfc,sizeof(void*),NULL));
        }
        // request a CSE interface
        _VMESSAGE("Requesting CSE interface ... ");
        g_messagingIntfc->Dispatch(g_pluginHandle, 'CSEI', NULL, 0, "CSE");
        break;
	default:
		_VMESSAGE("Received OBSE message type=%i from '%s' w/ data <%p> len=%04X", msg->type, msg->sender, msg->data, msg->dataLen);
		break;
	}
}

/*--------------------------------------------------------------------------------------------*/
// OBSE plugin query
extern "C" bool _declspec(dllexport) OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{
    // attach html-formatted log file to loader output handler
    HTMLTarget* tgt = (obse->isEditor) ? new HTMLTarget("Data\\obse\\plugins\\OBME\\CS.log.html", "OBME CS Log")
                                       : new HTMLTarget("Data\\obse\\plugins\\OBME\\Game.log.html", "OBME Game Log");
    _gLogFile = tgt;
    gLog.AttachTarget(*_gLogFile);
     // load rules for loader output from INI
    tgt->LoadRulesFromINI("Data\\obse\\Plugins\\OBME\\OBME.ini",obse->isEditor ? "CS.Log" : "Game.Log");

	// fill out plugin info structure
    info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "OBME";
	info->version = OBME_RECORD_VERSION(0);
    _MESSAGE("OBSE Query (CSMode = %i) v%p (%i.%i beta%i)",obse->isEditor,OBME_RECORD_VERSION(0),OBME_MAJOR_VERSION,OBME_MINOR_VERSION,OBME_BETA_VERSION);

    // check obse version
    g_obseIntfc = obse;
    if(obse->obseVersion < OBSE_VERSION_INTEGER)
	{
		_FATALERROR("OBSE version too old (got %08X expected at least %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
		return false;
	}

    // misc version checks
    if(!obse->isEditor)
	{		
        // game version
		if(obse->oblivionVersion != OBLIVION_VERSION)
		{
			_FATALERROR("incorrect Oblivion version (got %08X need %08X)", obse->oblivionVersion, OBLIVION_VERSION);
			return false;
        }

        // serialization interface	
        g_serializationIntfc = (OBSESerializationInterface*)obse->QueryInterface(kInterface_Serialization);
		if(!g_serializationIntfc)
		{
			_FATALERROR("Serialization interface not found");
			return false;
		}
		if(g_serializationIntfc->version < OBSESerializationInterface::kVersion)
		{
			_FATALERROR("Incorrect serialization version found (got %08X need %08X)", g_serializationIntfc->version, OBSESerializationInterface::kVersion);
			return false;
		}
        
        // array var interface
		g_arrayIntfc = (OBSEArrayVarInterface*)obse->QueryInterface(kInterface_ArrayVar);
		if (!g_arrayIntfc)
		{
			_FATALERROR("Array interface not found");
			return false;
		}

        // Script Interface
		g_scriptIntfc = (OBSEScriptInterface*)obse->QueryInterface(kInterface_Script);	
        if (!g_scriptIntfc)
		{
			_FATALERROR("Script interface not found");
			return false;
		}
	}
	else
	{
		// editor version
        if(obse->editorVersion != CS_VERSION)
		{
			_FATALERROR("TESCS version %08X, expected %08X)", obse->editorVersion, CS_VERSION);
			return false;
		}
	}
    
    // load ExportInjector library
    _MESSAGE("Loading ExportInjector ...");
    g_hExportInjector = LoadLibrary("Data\\obse\\Plugins\\COEF\\API\\ExportInjector.dll");
    if (g_hExportInjector)
    {
        _DMESSAGE("ExportInjector loaded at <%p>", g_hExportInjector);
    }
    else
    {
        _FATALERROR("Could not load ExportInjector.dll.  Check that this file is installed correctly.");
        return false;
    }

    // load COEF Components library
    _MESSAGE("Loading COEF Components ...");
    const char* componentlib = obse->isEditor ? "Data\\obse\\Plugins\\COEF\\Components\\Components.CS.dll" 
                                              : "Data\\obse\\Plugins\\COEF\\Components\\Components.Game.dll" ;
    if (LoadLibrary(componentlib))
    {
        _DMESSAGE("COEF components loaded successfully");
    }
    else
    {
        _FATALERROR("Could not load Components.dll.  Check that this file is installed correctly.");
        return false;
    }

    // load submodule    
    const char* modulename = obse->isEditor ? "Data\\obse\\plugins\\OBME\\OBME.CS.dll" : "Data\\obse\\plugins\\OBME\\OBME.Game.dll";
    _MESSAGE("Loading Submodule '%s' ...", modulename);
    g_hSubmodule = LoadLibrary(modulename);
    if (g_hSubmodule)
    {
        _DMESSAGE("Submodule loaded at <%p>", g_hSubmodule);
    }
    else
    {
        _FATALERROR("Could not load submodule '%s'.  Check that this file is installed correctly.", modulename);
        return false;
    }	

	// all checks pass
	return true;
}

/*--------------------------------------------------------------------------------------------*/
// OBSE plugin load
extern "C" bool _declspec(dllexport) OBSEPlugin_Load(const OBSEInterface * obse)
{
	_MESSAGE("OBSE Load (CSMode = %i)",obse->isEditor);

    // refresh obse interface
    g_obseIntfc = obse;
	g_pluginHandle = obse->GetPluginHandle();

	if(!obse->isEditor)
	{
        // serialization interface callbacks
		g_serializationIntfc->SetSaveCallback(g_pluginHandle, SaveCallback);
		g_serializationIntfc->SetLoadCallback(g_pluginHandle, LoadCallback);
		g_serializationIntfc->SetNewGameCallback(g_pluginHandle, NewGameCallback);
		g_serializationIntfc->SetPreloadCallback(g_pluginHandle, PreloadCallback);

		// string var interface
		g_stringIntfc = (OBSEStringVarInterface*)obse->QueryInterface(kInterface_StringVar);
		g_stringIntfc->Register(g_stringIntfc); // DEPRECATED: use RegisterStringVarInterface() in GameAPI.h
	}

	// messaging interface init
	g_messagingIntfc = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
	g_messagingIntfc->RegisterListener(g_pluginHandle, "OBSE", OBSEMessageHandler); // register to recieve messages from OBSE         
    g_messagingIntfc->RegisterListener(g_pluginHandle, NULL, GeneralMessageHandler); // register general message listener

	// command table interface
	g_cmdIntfc = (OBSECommandTableInterface*)obse->QueryInterface(kInterface_CommandTable);

    // obme interface
    FARPROC pQuerySubmodule = GetProcAddress(g_hSubmodule,"Initialize");
    if (pQuerySubmodule) 
    {
        g_obmeIntfc = (OBME_Interface*)pQuerySubmodule();
        if (!g_obmeIntfc) 
        {
            _ERROR("OBME interface not found");
        }
        else if (g_obmeIntfc->version < OBME_Interface::kInterfaceVersion)
        {
            _ERROR("Incorrect OBME interface version (v%p is incompatible with current version %p)",g_obmeIntfc->version,OBME_Interface::kInterfaceVersion);
            g_obmeIntfc = 0;
        }
        else
        {
            _DMESSAGE("OBME interface found at <%p>",g_obmeIntfc);
        }
    }
    else _ERROR("Could not initialize submodule.");

    // Register commands
    Register_OBME_Commands();

	return true;
}


/*--------------------------------------------------------------------------------------------*/
// Windows dll load
extern "C" BOOL WINAPI DllMain(HANDLE  hDllHandle, DWORD dwReason, LPVOID  lpreserved)
{// called when plugin is loaded into process memory, before obse takes over
	switch(dwReason)
    {
    case DLL_PROCESS_DETACH:    // dll unloaded 
        // delete dynamically allocated log file target
        if (_gLogFile)
        {
            gLog.DetachTarget(*_gLogFile);
            delete _gLogFile;
        }
        break;
    }   
	return true;
}