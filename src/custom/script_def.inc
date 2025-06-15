// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the definition of custom script commands in this file.
 * Format:
 *	BUILDIN_DEF(command_name,"parameters"),
 **/

//BUILDIN_DEF(example,""),

	// Autoattack System
	BUILDIN_DEF(aa_getskillinfostring, "iv"),
	BUILDIN_DEF(aa_setautoattack, "s"),
	BUILDIN_DEF(autoattack_fromitem, "i"),
	BUILDIN_DEF(aa_getautoattackstring, "i??"),
	BUILDIN_DEF(aa_getautoattackint, "i?"),
	BUILDIN_DEF(aa_getstate, ""),
	
	// Battle Stats System
	BUILDIN_DEF(getstatusstats, "s?"),  
	BUILDIN_DEF(gettraitstats, "s?"),
	BUILDIN_DEF(getbonusphysical, "s?"),
	BUILDIN_DEF(getbonusmagic, "s?"),
	BUILDIN_DEF(getelementalresist, "s?"),
	BUILDIN_DEF(getraceresist, "s?"),
	BUILDIN_DEF(getskillbonus, "s?"),
	
	// Deposit System
	BUILDIN_DEF(getdepositstore, ""),
	BUILDIN_DEF(getdepositlist, "i?"),
	BUILDIN_DEF(getdepositbonus, "?"),