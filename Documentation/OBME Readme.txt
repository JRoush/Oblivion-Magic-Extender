Name: Oblivion Magic Extender
Version: v1.1
Date: 2010-10
Category: Magic - Gameplay
Author: JRoush
Source: http://www.tesnexus.com/downloads/file.php?id=31981
Forum: tba

=======================
   Requirements:
=======================
NOTE: using OBME with the wrong verison of Oblivion or the CS will, *at best*, cause it to crash.
-   Oblivion v1.2.0416
-   TESCS v1.2.404 (only if you are planning to make mods using OBME)
-   Oblivion Script Extender (obse) v0018 or higher.  v0019 recommended.

=======================
    Description:
=======================
OBME extends the Oblivion magic system to make it more general and open to mod makers.  It is a pair of plugin libraries for OBSE that hack the game/CS to change it's behavior.  This is the same principle behind OBSE itself - it's quite safe, and legal.  OBME does not alter Oblivion.exe or TESConstructionSet.exe in any way.  The key features are:  
-   More control in editing Magic Effects and Effect Items in the CS
-   Creating new magic effects, both in the CS and from scripts in-game
-   *Any* effect item may now override select properties of it's magic effect:
    Name, School, VFXCode, Hostility, Icon, Base Cost,  Resistance AV, and numerous of other properties.
-   Expanded ModifyActorValue handler to specify what modifier is affected
-   Expanded SummonActor effect handler to allow custom summoning limits
-   Expanded ScriptEffect handler to allow scripts specified for an entire magic effect,
    not just individual effect items
-   Expanded Dispel effect handler to allow partial dispelling of effects, to selectively 
    dispel effects based on magic effect, effect handler, hostility, and magic item type
-   When an active effect is dispelled by recasting the spell or exceeding a summoning limit, its
    remaining magicka may be "reclaimed" to strengthen the effect that replaces it.
    
OBME is a modding *resource*.  All of it's features are modular, and are disabled by default.  A 3rd party mod like the included examples is required to actually use any of these features.  For more details and technical information, please see the accompanying document 'OBME Features.pdf'.  This file has background information and technical details.  

==========================================
*** IMPORTANT: Compatibility with obse ***
==========================================
The following v0018 obse functions will not work with dynamic effect codes.  This is because they cannot return values > 0x80000000.  This issue should be fixed in a later release of obse.
    -   GetMagicEffectCounters(C)
    -   GetNthMagicEffectCounter(C)
    -   GetNthEffectItemScriptVisualEffect (use GetNthEIHandlerParam instead)
    -   GetActiveEffectCodes
The following v0018 obse functions will not return correct results for new scripted effects.  This is because they check effect code instead of effect handler.  This issue may be fixed in a future release of obse.
    -   IsNthEffectItemScripted
    -   MagicItemHasEffectItemScript
The following v0018 obse functions do not work with OBME, *but have been made compatible in obse v0019*.  Install obse v0019 and these functions will work fine.
    -   IsSpellHostile
    -   IsPoison
    -   CopyNthEffectItem
    -   CopyAllEffectItems
    -   GetScriptActiveEffectIndex
The following v0018 obse functions are patched by OBME to force them to be compatible.  The obse team is not responsible for problems with these functions while you have OBME installed.
    -   GetMagicEffectCode
    -   GetNthActiveEffectCode
    -   GetNthEffectItemCode
    -   AddEffectItem
    -   AddEffectItemC
    -   AddFullEffectItem
    -   AddFullEffectItemC
    -   GetSpellMagickaCost
    -   GetEnchantmentCost
    -   GetSpellMasteryLevel
    -   GetSpellSchool
The following v0018 obse functions are deprecated by new OBME functions.  They still work as intended, but the replacement functions have expanded functionality.  For details, see the relevent sections of the features pdf.
    -   Get(Set)MagicEffectOtherActorValue(C)   ... use Get(Set)MagicEffectHandlerParam
    -   Get(Set)MagicEffectUsedObject(C)        ... use Get(Set)MagicEffectHandlerParam 
    -   Is(Set)MagicEffectDetrimental(C)        ... use Get(Set)MagicEffectHandlerParam 
    -   Is(Set)MagicEffectHostile(C)            ... use Get(Set)MagicEffectHostility 
    -   Get(Set)NthEffectItemActorValue         ... use Get(Set|Clear)NthEIHandlerParam 
    -   Get(Set)NthEffectItemScript             ... use Get(Set|Clear)NthEIHandlerParam 
    -   Get(Set)NthEffectItemScriptVisualEffect ... use Get(Set|Clear)NthEIVFXCode 
    -   Get(Set)NthEffectItemScriptSchool       ... use Get(Set|Clear)NthEISchool 
    -   Is(Set)NthEffectItemScriptHostile       ... use Get(Set|Clear)NthEIHostility
    -   Get(Set|SetEX|Mod)NthEffectItemScriptName ... use Get(Set|Clear)NthEIName
All v0018 obse functions not explicitly mentioned are completely compatible with OBME.  If you encounter any unexpected behavior, please report it to me immediately.
    
==============================
Compatibility with other mods:
==============================
OBME doesn't add any actual content to the game, so there is no chance of conflicts with any traditional mods.  
Mods made for OBME can be loaded even if OBME isn't installed.  Any OBME-specific features are disabled, so the mod may not be very useful.
Savegames made with OBME can be loaded after OBME is uninstalled, provided the 'Uninstallation' directions above are followed.
OBME is designed to be compatible with AddActorValues.
  
===================================================================
Compatibility with 3rd party Mod Editors (TES4Edit, WryeBash, etc):
===================================================================
OBME alters the savegame and esp format slightly.  Other programs that display or edit these files may run into trouble.
- TES4Edit v2.5.3 can load/save OBME mods just fine, but may crash if a magic effect or magic item is selected.
- WryeBash v287 can manage OBME mods just fine, but problems will arise if you try to build a bashed patch that includes magic items. 
The maintainers of these programs (Elminster and Waruddar, respectively) have graciously agreed to add support for OBME in future versions.  If you get the chance, please be sure to thank them for this.
    
========================
Questions & Bug Reports:
========================
If you have a question, *look in the included 'Features' pdf*.  It has a table of contents; you don't have to read the whole thing to find what you're looking for.  If your question isn't answered there, I would be glad to answer it on the forum thread.

To report a bug or ask for help with troubleshooting, follow these steps:
1.  Read this readme carefully.
2.  Reproduce the problem.  If it's a crash, restart the game or CS and try to make it crash again.
3.  Narrow down the cause.  If it's a conflict, try to figure out which mod it's conflicting with. 
4.  Send me an email at JRoush.TESMods@gmail.com or PM me on the TESNexus/Bethesda forums  
    - Include the problem details and the version of Oblivion, TESCS, and OBSE you are using
    - Include a list of other OBSE plugins you are using (OBGE, CSI, MenuQue, Oblivion Stutter Remover, etc.)
    - Attach the savegame or mod file that you were using when you discovered the problem
    - Attach the log files "OBME.log" and/or "OBME_CS.log", which can be found in your Oblivion folder. 
    
I'll need all of this information to have any chance of reproducing your problem.  And if I can't reproduce it, it doesn't get fixed.

Consider these examples:
 UserA: "I installed OBME and now Oblivion crashes whenever I load a save !!!"
 UserB: "I installed OBME and now Oblivion crashes whenever I load a save !!!  
         *Here is my OBSE version, load order, and OBME.log*" 
 UserC: "If I have both OBME and MyModXX.esp installed, Oblivion crashes whenever I load a save.  
        *Here is my OBSE version, load order, OBME.log, and a copy of my savegame*" 
For all I know, UserA might not be using OBSE, or maybe he botched the installation, or perhaps he's trying to play the game on an etch-a-sketch.  Reports like this are ignored, they just don't contain any useful information.  
UserB does better.  I can at least check for obvious conflicts and error messages.  Still, the odds of me reproducing the problem are small.
UserC has done his homework.  I can actually test his problem for myself, and hopefully fix it.
   
=======================
 Included Demo Mods:
=======================
  Included are several small demo mods with names like "OBME_***_example.esp".  Each demonstrates a different feature of OBME, and provides an assortment of spells, items, etc. that can be used to test it.  See the the mod descriptions for details.  When playtesting these mods, remember to wait about 5 seconds after loading the game for all scripts to fire up.
  
=======================
    Installation:
=======================
1.  If necessary, install the Oblivion Script Extender and learn how to run the game and CS using obse_loader.exe
2.  If you are using mods with OBME features but you don't currently have OBME installed: 
    make a clean backup save without these mods before proceeding (recommended, not required).
3.  Copy the contents of archive "Oblivion\" folder to the "Oblivion\" folder on you computer.  

Because it uses OBSE plugin libraries, it doesn't need to be 'activated'.  Once installed, it is always active.  If you want to use the demo mods, however, remember to activate them in your mod list before running Oblivion.
  
=======================
   Uninstallation:
=======================
1.  It is ***strongly recommended*** that you uninstall any mods that use OBME and make a clean save before proceeding.
2.  Delete 'OBME.dll' and 'OBME_CS.dll' from your "Oblivion\Data\OBSE\Plugins\" folder. 

=======================
      History:
=======================
=== v1.1 2010/10 ===
-   Fixed several small bugs in the Get/SetHandlerParam script functions
-   OBME will now dispatch a message to all other obse plugins on PostLoad, containing a pointer to an interface object.  See Source\OBME\RSH\Commands\OBME_Interface.h for details.
-   OBME v1.1 is fully compatible with Custom Spell Icons (CSI) v???
=== v1.0 2010/08 ===
-   Fixed issue where ModAV effects couldn't modify base actor values below zero, which was breaking some ability weakness effects.
-   Removed dependancy on AddActorValues.  OBME is still compatible with AAV, but does not requires it and is no longer packaged with it.
-   Standardized format of drop-down lists for magic effects and handlers in CS
-   Several small bugfixes
-   Removed overrides to CopyThEI, CopyAllEI, and GetScriptActiveEffectIndex obse functions, as v0019 made these overrides unnecessary
-   Added overrides to GetSpellMagickaCost, GetSpellMasteryLevel, GetSpellSchool, and GetEnchantmentCost
=== v1.4 2010/08 ===
-   Fixed issue with Icon & Model file dialogs causing CS crashes (now uses the built-in dialogs)
-   Fixed issue where starting game while CS was running might cause a CTD 
-   Added complete version info to all OBME-modified records
-   Fixed issue of broken Chameleon,Invisibility,NightEye,Darkness,Paralyze,Calm,Frency,DetectLife, and Telekinesis effect items created prior to v1.beta3.Hotfix.
-   Fixed issue where duplicating an effect setting in CS or using CreateMgef wouln't copy some handler-specific parameters
-   Changed Dispel handler targeting so that it applies to all magic targets (vanilla handler only works on PC & NPCs)
-   Changed file formats for compatibility with new versions of WryeBash and TES4Edit
-   Disabled automatic recalculation of player's AVs on reload - code didn't work well in vanilla, would be utterly broken with obme
-   Added script functions Get(Set)MagicEffectHandlerParamC, Get(Set/Clear)NthEffectItemHandlerParam, Get(Set/Clear)NthEffectItemSchool, Get(Set/Clear)NthEffectItemVFXCode.
=== v1.beta3 2010/07 ===
-   Implemented assigned opcode base for new script functions: 0x2600
-   Identified the uses of two of the 'misc' magic effect flags: 'Persist On Death', and 'Explodes with Force'
-   Expanded size and functionality of effect item 'overrides' object.  EfixParam and EfitParam are no longer forced to same value.
-   Replaced 'Effect Item' dialog in the CS with an improved version.
-   Altered format of the ScriptEffect handler - eliminated 'UseEfitScipt' flag & made EfitParam a 'user parameter' that is generally ignored by OBME.  If non-zero, efixParam is used as script formID; otherwise mgefParamA is used.
-   Implemented EffectItem overrides to hostility, effect name, school, base cost, icon, vfx code, resistance AV, persist-on-death, and handler-specific params (including the 'recovers' flags for some handlers).
-   Altered save format for magic items: additional override parameters for EffectItems now saved in EFII and EFIX chunks
-   OBME is now compatible with AddActorValues.  ResistanceAVs and the avs modified by the ModActorValue handler may now be mod-added token items.
-   Altered format of ModifyActorValue effect handler.  The param fields are now a range of actor values.  An additional field determines which part (base/max/offset/damage) of the actor value to modify.  An additional flag reproduces the special treatement for Ability effects.
-   Altered save format for ActiveEffects to store more specific conversion info.  The (remote) possibility that new magic effects could corrupt save files should now be gone completely.
-   Added script functions GetMagicEffectHandler,SetMagicEffectHandler to manipulate handlers on magic effects
-   Added script functions GetMagicEffectHostility,SetMagicEffectHostility to manipulate new 3-state hostility of magic effects
-   Added new script functions to manipulate 'override' fields of effect items: (Get/Set/Clear)NthEIEffectName,(Get/Set/Clear)NthEIIconPath, (Get/Set/Clear)NthEIHostility,(Get/Set/Clear)NthEIBaseCost,(Get/Set/Clear)NthEIResistAV.  Not yet added: similar functions for VFXCode,School,Handler-specifc params like script/AV, and individual flags.
-   Where appropriate, vanilla magic effects may now have the 'Beneficial' hostility state
=== v1.beta2 2010/06 ===
-   Decoupled mgefCode from editorID.  Effects now may have arbitrary editorids.
-   Changed the mod-file format for MGEF record.  OBME-related info is now stored as additional chunks in the MGEF records.
-   Changed mod-file format for magic item records (SPEL,ENCH,ALCH,INGR,SGST).  Any effect item chunk (EFIT) may now be followed by extra script-effect data chunks (SCIT/FULL).  Additional OBME-related data is stored in addition chunks.
-   Fixed formID resolution.  All fields in EffectSetting that are saved as formids now use full-on formID resolution.
-   Fixed broken 'CureEffect' handler; the vanilla (and only vanilla) cure effects now work.
-   Added support for 'dynamic' effect codes in the range (0x80000000 - 0xFFFFFFFE).  Automatically resolved: EffectSetting mgefCode and counter-effect codes, EffectItem mgefCodes and VFX code, and the list of the PC's known effects saved in the savegame.
-   Added the ResolveMgefCode script function.
-   Changed savegame routine to allow saving of all 'Created' base forms, regardless of type
-   Added script command CreateMgef
-   Fixed the issue where effects without all three OnSelf/OnTouch/OnTarget ranges wouldn't appear for spellmaking/enchanting.
-   Implemented 'reclamation' for effects removed by recasting and summoning limits - a fraction of the remaining magicka is applied to the new effect.  Fraction is controlled by a new game setting 'fMagicReclaimFactor'.
-   Added a new game setting 'fMagicPersistentDuration' to control the effective duration of persistent effects (e.g. abilities, diseases, etc).
-   Expanded SummonActor effect handler to accept a global variable as a summoning limit
-   Expanded the Dispel effect handler to allow for partial dispelling.  Handler will now also apply only to specified magic item types, magic effects, effect handlers, and hostility states.
-   Patched the OBSE script commands GetMagicEffectCode,GetScriptActiveEffectIndex,GetNthActiveEffectCode,GetNthEffectItemCode,AddEffectItem,AddEffectItemC,AddFullEffectItem, & AddFullEffectItemC
=== v1.beta1 2010/06 ===
-   Replaced 'Magic Effects' dialog in CS with a much improved version
-   Added a debugging window to the CS (not for release builds)
-   Implemented the 'mgefParamIsForm' flag; effects with this flag will attempt to resolve their mgefParam like a formid
-   Changed SEFF handler to allow script formids to be passed in mgefParam
-   Added a demo package for 'Footloose', using the SEFF handler
=== v0.poc 2010/05 ===
-   Very basic proof of concept.  Game and CS will recognize new MGEF records in mods and append them to the effect table.
-   Added a demo mod for 'Poison Damage', using the MDAV handler

=======================
      Credits:
=======================
-   Bethesda Softworks, whose game has sucked up far, far too much of my free time
-   The OBSE team for their incredible modding resource.
-   scruggsywuggsy the ferret, who has been making a considerable effort behind the scenes to make OBSE compatible with OBME.
-   everyone on the TESNexus and Bethesda forums who took an active interest in the project - DragoonWraith, tejon, Waruddar, shadeMe, kyoma, and many others.
-   special thanks to SpeedyB, Kamikaze, and Statttis for helping track down and solve bugs
-   Statttis, for doing the extensive legwork necessary to implement effectitem icon overrides.

=======================
     Tools Used:
=======================
-   Oblivion Script Extender (OBSE): http://obse.silverlock.org
-   TES4Edit: http://www.tesnexus.com/downloads/file.php?id=11536
-   TES Construction Set: http://www.tesnexus.com/downloads/file.php?id=11367
-   Microsoft Visual Studio 2008: find a download yourself ;)
-   IDA Pro v4.9: http://www.datarescue.com/idabase/ida.htm
-   Perforce SCM: http://www.perforce.com/perforce/downloads/index.html
