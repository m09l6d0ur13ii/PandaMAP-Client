#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

MACRO_CONFIG_INT(PmDummyHammer, pm_dummy_hammer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_INSENSITIVE, "Hammer Chicka")
MACRO_CONFIG_INT(PmDummyHammerDelay, pm_dummy_hammer_delay, 25, 1, 1000, CFGFLAG_CLIENT | CFGFLAG_INSENSITIVE, "Delay PM pm_dummy_hammer and cl_dummy_hammer hits")
MACRO_CONFIG_INT(PmDummyKeepHook, pm_dummy_keep_hook, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_INSENSITIVE, "Keep hook pressed during PM dummy hammer hit (0 = off, 1 = on)")
MACRO_CONFIG_INT(PmDummyKeepMove, pm_dummy_keep_move, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_INSENSITIVE, "(0 = off, 1 = on)") // TODO: add Desc
MACRO_CONFIG_INT(PmDummyKeepJump, pm_dummy_keep_jump, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_INSENSITIVE, "(0 = off, 1 = on)") // TODO: add Desc
