// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the body of custom script commands in this file.
 * Format:
 *	BUILDIN_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

//BUILDIN_FUNC(example)
//{
//	ShowInfo("buildin_example: It works!\n");
//	script_pushint(st,1);
//	return 0;
//}

/// Returns infos on a skill.
///
/// aa_getskillinfostring(<id>,<skill id>)
/// aa_getskillinfostring(<id>,"<skill name>")
/// 0 = Description
BUILDIN_FUNC(aa_getskillinfostring)
{
	int skill_id, id;
	TBL_PC* sd;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;// no player attached, report source

	id = script_getnum(st, 2);
	skill_id = (script_isstring(st, 3) ? skill_name2id(script_getstr(st, 3)) : script_getnum(st, 3));

	switch (id) {
	case 0:	// skill->desc
		std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);
		if (!skill)
			return SCRIPT_CMD_SUCCESS;
		script_pushstrcopy(st, skill->desc);
		break;
	}

	return SCRIPT_CMD_SUCCESS;
}

// Start auto buff from rental items
BUILDIN_FUNC(autoattack_fromitem) {
	map_session_data* sd;
	t_tick max_duration = 86400;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;

	t_itemid item_id = script_getnum(st, 2);

	return handleAutoattack_fromitem(sd, item_id, max_duration);
}

/*
	Returns info of player on autoattack

	aa_getautoattackstring(<id>{,<index>})
	0 = Auto Heal all desc - Index = skill id
	1 = HP / SP Potions
	2 = Ressurrection
	3 = auto buff skill desc
	4 = auto sit rest
	5 = auto buff items desc
	6 = teleport desc
	7 = General autoattack_potions state (disable / enable)
	8 = Return to savepoint when dead (disable / enable)
	9 = Token of siegfried use for auto res (disable / enable)
	10 = Auto accept party request
	11 = Priorize Loot / Fight - 0 Fight - 1 Loot
	12 = Monster selection
	13 = Item pickup selection
*/
BUILDIN_FUNC(aa_getautoattackstring)
{
	int index = 0, extra_index = 0, id;
	TBL_PC* sd;
	std::ostringstream os_buf;
	os_buf.str("");
	struct party_data* p = nullptr;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_FAILURE;// no player attached, report source

	id = script_getnum(st, 2);
	index = script_getnum(st, 3);
	extra_index = script_getnum(st, 4);

	switch (id) {
	case 0: // auto heal desc
		handle_autoattack_heal(os_buf, index, extra_index, st, sd);
		break;

	case 1: // HP / SP Potions - // potion_menu_list
		handle_autoattack_potions(os_buf, index, st, sd);
		break;

	case 2: // auto attack skill desc
		handle_autoattack_attack(os_buf, index, extra_index, st, sd);
		break;

	case 3:// auto buff skill desc
		handle_autoattack_buff(os_buf, index, extra_index, st, sd);
		break;

	case 4: // Auto sit to rest
		handle_autoattack_sitrest(os_buf, index, st, sd);
		break;

	case 5:	// Buff items - Used for show the buffitems_menu
		handle_autoattack_items(os_buf, index, st, sd);
		break;

	case 6: 
		handle_autoattack_teleport(os_buf, index, st, sd);
		break;

	case 7:
		handle_potions(os_buf, sd);
		break;

	case 8:
		handle_return_to_savepoint(os_buf, sd);
		break;

	case 9:
		handle_token_of_siegfried(os_buf, sd);
		break;

	case 10:
		handle_party_request(os_buf, sd);
		break;

	case 11:
		handle_priorize_loot_fight(os_buf, sd);
		break;

	case 12: // monster desc
		handle_autoattack_monsterselection(os_buf, index, extra_index, st, sd);
		break;

	case 13: // item pick up
		handle_autoattack_itempickup(os_buf, index, extra_index, st, sd);
		break;
	}

	script_pushstrcopy(st, os_buf.str().c_str());
	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(aa_getautoattackint)
{
	TBL_PC* sd;
	if (!script_rid2sd(sd)) return SCRIPT_CMD_SUCCESS; // No player attached, report source

	int id = script_getnum(st, 2);
	int num = 0; // Variable utilisee pour stocker le resultat
	std::shared_ptr<item_data> item_data;

	const uint32 char_id = sd->status.char_id;

	switch (id) {
	case 0: // Auto heal
	case 2: // Active auto attack skills
	case 3: // Active auto buffs skills
	case 4: // Active potions
	case 5: // Buff items
	case 6: // Return to save point on death
	case 7: // Token of Siegfried
	case 8: // Auto accept party request
	case 9: // Pick up item config
	case 10: // Prio item config
	case 11: // Stop melee
	case 12: // teleport use fly wing
	case 13: // teleport use skill
	case 14: // Sit regen hp
	case 15: // Sit regen sp
	case 16: // Attack aggressve monster
	case 17: // Item pick up selecton size
	case 18: // Action on end
		num = handleGetautoattackint(sd, id);
		break;
	case 1: // HP / SP Potions available in inventory
		for (int i = 0; i < MAX_INVENTORY; ++i) {
			item_data = item_db.find(sd->inventory.u.items_inventory[i].nameid);
			if (!item_data) break;
			if (item_data->type == IT_HEALING) ++num;
		}
		break;
	default:
		num = 0;
		break;
	}

	script_pushint(st, num);
	return SCRIPT_CMD_SUCCESS;
}

/*
	Save info of player on autoattack

	aa_setautoattack(<str>)
	Parse a string with ; as separator
	id;
	id = 0 - autoheal (is_active;skill_id;skill_lv;min_hp)
	id = 1 - autopotion (is_active;item_id;min_hp;min_sp)
	id = 2 - attack skill (is_active;skill_id;skill_lv)
	id = 3 - buff skills (is_active;skill_id;skill_lv)
	id = 4 - teleport flywing
	id = 5 - autoattackitems(is_active; item_id; delay)
	id = 6 - teleport skill
	id = 7 - Melee Attack
	id = 8 - Active autoattack potions
	id = 9 - Return to save point on death
	id = 10 - reset config
	id = 11 - Token of siegfried
	id = 12 - Auto accept parrty request
	id = 13 - Priorize loot / fight
	id = 14 - Sit rest hp
	id = 15 - Sit rest sp
	id = 16 - Teleport min hp
	id = 17 - Teleport delay no mob meet
	id = 18 - Ignore aggressive monster not in the list
	id = 19 - Monster selection list
	id = 20 - Item Pickup configuration
	id = 21 - Item Pickup selection
	id = 22 - Action on end
	id = 23 - Max monster surround
*/
BUILDIN_FUNC(aa_setautoattack)
{
	TBL_PC* sd;
	const char delim = ';';
	std::vector<std::string> result;
	std::string item, str;
	int id = -1;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS; // No player attached

	str = script_getstr(st, 2);
	std::stringstream ss(str);

	while (std::getline(ss, item, delim)) {
		result.push_back(item);
	}

	if (result.empty())
		return SCRIPT_CMD_FAILURE;

	// Extract the ID to process
	id = std::stoi(result[0]);

	switch (id) {
	case 0:
		handleAutoHeal(result, sd);
		break;
	case 1:
		handleAutoPotion(result, sd);
		break;
	case 2:
		handleAutoAttackSkills(result, sd);
		break;
	case 3:
		handleAutoBuffSkills(result, sd);
		break;
	case 4:
		handleTeleportFlywing(result, sd);
		break;
	case 5:
		handleAutoAttackItems(result, sd);
		break;
	case 6:
		handleTeleportSkill(result, sd);
		break;
	case 7:
		handleMeleeAttack(result, sd);
		break;
	case 8:
		handleAutoAttackPotionState(result, sd);
		break;
	case 9:
		handleReturnToSavepoint(result, sd);
		break;
	case 10:
		handleResetAutoAttackConfig(sd);
		break;
	case 11:
		handleTokenOfSiegfried(result, sd);
		break;
	case 12:
		handleAcceptPartyRequest(result, sd);
		break;
	case 13:
		handlePriorizeLootFight(result, sd);
		break;
	case 14:
		handleSitRestHp(result, sd);
		break;
	case 15:
		handleSitRestSp(result, sd);
		break;
	case 16:
		handleTeleportMinHp(result, sd);
		break;
	case 17:
		handleTeleportNoMobMeet(result, sd);
		break;
	case 18:
		handleIgnoreAggressiveMonster(result, sd);
		break;
	case 19:
		handleMonsterSelection(result, sd);
		break;
	case 20:
		handleItemPickupConfiguration(result, sd);
		break;
	case 21:
		handleItemPickupSelection(result, sd);
		break;
	case 22:
		handleActionOnEnd(result, sd);
		break;
	case 23:
		handleMonsterSurround(result, sd);
		break;
	}

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(aa_getstate)
{
	TBL_PC* sd;

	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;// no player attached, report source

	script_pushint(st, sd->state.autoattack);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getstatusstats) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");  
            return 0;  
        }  
    } else {  
        if (!script_rid2sd(target_sd)) {  // This is the correct usage  
            script_pushconststr(st, "Player not found"); 
            return 0;  
        }  
    }  
      
    status_data *status = status_get_status_data(*target_sd);  
    std::ostringstream output;  
      
    output << "|--- Status Information ---|\n";    
    output << "   -> Physical Damage (ATK): ^0000FF" << pc_leftside_atk(target_sd) << "^000000 + ^0000FF" << pc_rightside_atk(target_sd) << "^000000\n";    
    output << "   -> Magical Damage (MATK): ^0000FF" << pc_leftside_matk(target_sd) << "^000000 + ^0000FF" << pc_rightside_matk(target_sd) << "^000000\n";    
    output << "   -> Hit Rate (HIT): ^0000FF" << status->hit << "^000000\n";    
    output << "   -> Critical Rate (CRIT): ^0000FF" << status->cri / 10 << "^000000\n";    
    output << "   -> Physical Defense (DEF): ^0000FF" << pc_leftside_def(target_sd) << "^000000 + ^0000FF" << pc_rightside_def(target_sd) << "^000000\n";    
    output << "   -> Magical Defense (MDEF): ^0000FF" << pc_leftside_mdef(target_sd) << "^000000 + ^0000FF" << pc_rightside_mdef(target_sd) << "^000000\n";    
    output << "   -> Flee Rate (FLEE): ^0000FF" << status->flee << "^000000 + ^0000FF" << status->flee2 / 10 << "^000000\n";    
    output << "   -> Attack Speed (ASPD): ^0000FF" << (2000 - status->amotion) / 10 << "^000000\n";  
      
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}  
  
// Script command for trait status information    
BUILDIN_FUNC(gettraitstats) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");
            return 0;  
        }  
    } else {    
        if (!script_rid2sd(target_sd)) {  
            script_pushconststr(st, "Player not found");
            return 0;  
        }  
    }  
      
    status_data *status = status_get_status_data(*target_sd);  
    std::ostringstream output;  
      
	output << "|--- Trait Status Information ---|\n";    
	output << "   -> Final Physical Damage (P.ATK): ^0000FF" << status->patk << "^000000\n";    
	output << "   -> Final PhysicalMagical Damage (S.MATK): ^0000FF" << status->smatk << "^000000\n";    
	output << "   -> Heal Plus (H.Plus): ^0000FF" << status->hplus << "^000000\n";    
	output << "   -> Critical Damage Rate (C.Rate): ^0000FF" << status->crate << "^000000\n";    
	output << "   -> Physical Resistance (RES): ^0000FF" << status->res << "^000000\n";    
	output << "   -> Magical Resistance (MRES): ^0000FF" << status->mres << "^000000\n";  
      
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}

BUILDIN_FUNC(getbonusphysical) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");  
            return 0;  
        }  
    } else {  
        if (!script_rid2sd(target_sd)) {  
            script_pushconststr(st, "No player attached");  
            return 0;  
        }  
    }  
      
    std::ostringstream output;  
      
    output << "|--- Bonus Physical Damage ---|\n";  
      
    bool found_bonuses = false;  
        
	// Size bonuses      
	if (target_sd->right_weapon.addsize[SZ_MEDIUM] + target_sd->left_weapon.addsize[SZ_MEDIUM]) {      
		output << "   -> Medium Size: ^0000FF" << (target_sd->right_weapon.addsize[SZ_MEDIUM] + target_sd->left_weapon.addsize[SZ_MEDIUM]) << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->right_weapon.addsize[SZ_ALL] + target_sd->left_weapon.addsize[SZ_ALL]) {      
		output << "   -> All Sizes: ^0000FF" << (target_sd->right_weapon.addsize[SZ_ALL] + target_sd->left_weapon.addsize[SZ_ALL]) << "^000000%\n";      
		found_bonuses = true;    
	}      
		  
	// Race bonuses      
	if (target_sd->right_weapon.addrace[RC_PLAYER_HUMAN] + target_sd->left_weapon.addrace[RC_PLAYER_HUMAN] +       
		target_sd->right_weapon.addrace[RC_PLAYER_DORAM] + target_sd->left_weapon.addrace[RC_PLAYER_DORAM]) {      
		output << "   -> Player: ^0000FF" << (target_sd->right_weapon.addrace[RC_PLAYER_HUMAN] + target_sd->left_weapon.addrace[RC_PLAYER_HUMAN] +       
									  target_sd->right_weapon.addrace[RC_PLAYER_DORAM] + target_sd->left_weapon.addrace[RC_PLAYER_DORAM]) << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->right_weapon.addrace[RC_ALL] + target_sd->left_weapon.addrace[RC_ALL]) {      
		output << "   -> All Races: ^0000FF" << (target_sd->right_weapon.addrace[RC_ALL] + target_sd->left_weapon.addrace[RC_ALL]) << "^000000%\n";      
		found_bonuses = true;    
	}      
		  
	// Class bonuses      
	if (target_sd->right_weapon.addclass[CLASS_NORMAL] + target_sd->left_weapon.addclass[CLASS_NORMAL]) {      
		output << "   -> Normal Class: ^0000FF" << (target_sd->right_weapon.addclass[CLASS_NORMAL] + target_sd->left_weapon.addclass[CLASS_NORMAL]) << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->right_weapon.addclass[CLASS_ALL] + target_sd->left_weapon.addclass[CLASS_ALL]) {      
		output << "   -> All Classes: ^0000FF" << (target_sd->right_weapon.addclass[CLASS_ALL] + target_sd->left_weapon.addclass[CLASS_ALL]) << "^000000%\n";      
		found_bonuses = true;    
	}    
      
    // Check if no bonuses were found and display message  
    if (!found_bonuses) {  
        output << "   -> No bonus physical damage found\n";    
    } 
      
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}

BUILDIN_FUNC(getbonusmagic) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");  
            return 0;  
        }  
    } else {  
        if (!script_rid2sd(target_sd)) {  
            script_pushconststr(st, "No player attached");  
            return 0;  
        }  
    }  
      
    std::ostringstream output;  
      
    output << "|--- Bonus Magic Damage ---|\n";  
      
    bool found_bonuses = false;  
        
	// Size bonuses for magic damage      
	if (target_sd->indexed_bonus.magic_addsize[SZ_MEDIUM]) {      
		output << "   -> Medium Size: ^0000FF" << target_sd->indexed_bonus.magic_addsize[SZ_MEDIUM] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addsize[SZ_ALL]) {      
		output << "   -> All Sizes: ^0000FF" << target_sd->indexed_bonus.magic_addsize[SZ_ALL] << "^000000%\n";      
		found_bonuses = true;    
	}      
		  
	// Race bonuses for magic damage      
	if (target_sd->indexed_bonus.magic_addrace[RC_PLAYER_HUMAN] + target_sd->indexed_bonus.magic_addrace[RC_PLAYER_DORAM]) {      
		output << "   -> Player: ^0000FF" << (target_sd->indexed_bonus.magic_addrace[RC_PLAYER_HUMAN] + target_sd->indexed_bonus.magic_addrace[RC_PLAYER_DORAM]) << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addrace[RC_ALL]) {      
		output << "   -> All Races: ^0000FF" << target_sd->indexed_bonus.magic_addrace[RC_ALL] << "^000000%\n";      
		found_bonuses = true;    
	}      
		  
	// Class bonuses for magic damage      
	if (target_sd->indexed_bonus.magic_addclass[CLASS_NORMAL]) {      
		output << "   -> Normal Class: ^0000FF" << target_sd->indexed_bonus.magic_addclass[CLASS_NORMAL] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addclass[CLASS_ALL]) {      
		output << "   -> All Classes: ^0000FF" << target_sd->indexed_bonus.magic_addclass[CLASS_ALL] << "^000000%\n";      
		found_bonuses = true;    
	}      
		  
	// Element bonuses for magic damage      
	if (target_sd->indexed_bonus.magic_addele[ELE_NEUTRAL]) {      
		output << "   -> Neutral Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_NEUTRAL] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_WATER]) {      
		output << "   -> Water Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_WATER] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_EARTH]) {      
		output << "   -> Earth Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_EARTH] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_FIRE]) {      
		output << "   -> Fire Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_FIRE] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_WIND]) {      
		output << "   -> Wind Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_WIND] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_POISON]) {      
		output << "   -> Poison Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_POISON] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_HOLY]) {      
		output << "   -> Holy Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_HOLY] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_DARK]) {      
		output << "   -> Dark Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_DARK] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_GHOST]) {      
		output << "   -> Ghost Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_GHOST] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_UNDEAD]) {      
		output << "   -> Undead Element: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_UNDEAD] << "^000000%\n";      
		found_bonuses = true;    
	}      
	if (target_sd->indexed_bonus.magic_addele[ELE_ALL]) {      
		output << "   -> All Elements: ^0000FF" << target_sd->indexed_bonus.magic_addele[ELE_ALL] << "^000000%\n";      
		found_bonuses = true;    
	}    
      
    // Check if no bonuses were found and display message  
    if (!found_bonuses) {  
        output << "   -> No bonus magic damage found\n";    
    }
	
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}

BUILDIN_FUNC(getelementalresist) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");  
            return 0;  
        }  
    } else {  
        if (!script_rid2sd(target_sd)) {  
            script_pushconststr(st, "No player attached");  
            return 0;  
        }  
    }  
      
    std::ostringstream output;  
      
    output << "|--- Elemental Resistance ---|\n";  
      
    // Define elemental resistance info array similar to your original implementation  
    struct {  
        const char *name;  
        int value;  
    } ele_resist_info[] = {  
        { "Neutral", target_sd->indexed_bonus.subele_script[ELE_NEUTRAL] },  
        { "Water", target_sd->indexed_bonus.subele_script[ELE_WATER] },  
        { "Earth", target_sd->indexed_bonus.subele_script[ELE_EARTH] },  
        { "Fire", target_sd->indexed_bonus.subele_script[ELE_FIRE] },  
        { "Wind", target_sd->indexed_bonus.subele_script[ELE_WIND] },  
        { "Poison", target_sd->indexed_bonus.subele_script[ELE_POISON] },  
        { "Holy", target_sd->indexed_bonus.subele_script[ELE_HOLY] },  
        { "Dark", target_sd->indexed_bonus.subele_script[ELE_DARK] },  
        { "Ghost", target_sd->indexed_bonus.subele_script[ELE_GHOST] },  
        { "Undead", target_sd->indexed_bonus.subele_script[ELE_UNDEAD] },  
        { "ALL", target_sd->indexed_bonus.subele_script[ELE_ALL] },  
    };  
      
    // Output only non-zero resistance values  
	bool found_bonuses = false;
	
	for (int i = 0; i < 11; i++) {    
		if (ele_resist_info[i].value != 0) {    
			output << "   -> " << ele_resist_info[i].name << ":  ^0000FF" << ele_resist_info[i].value << "^000000%\n";  
			found_bonuses = true;  
		}  
	}  
	  
	if (!found_bonuses) {  
		output << "   -> No element resist bonuses found\n";  
	} 
      
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}

BUILDIN_FUNC(getraceresist) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");  
            return 0;  
        }  
    } else {  
        if (!script_rid2sd(target_sd)) {  
            script_pushconststr(st, "No player attached");  
            return 0;  
        }  
    }  
      
    std::ostringstream output;  
      
    output << "|--- Race Resistance ---|\n";  
      
    // Define race resistance info array similar to your original implementation  
    struct {  
        const char *name;  
        int value;  
    } race_resist_info[] = {  
        { "Formless", target_sd->indexed_bonus.subrace[RC_FORMLESS] },  
        { "Undead", target_sd->indexed_bonus.subrace[RC_UNDEAD] },  
        { "Beast", target_sd->indexed_bonus.subrace[RC_BRUTE] },  
        { "Plant", target_sd->indexed_bonus.subrace[RC_PLANT] },  
        { "Insect", target_sd->indexed_bonus.subrace[RC_INSECT] },  
        { "Fish", target_sd->indexed_bonus.subrace[RC_FISH] },  
        { "Demon", target_sd->indexed_bonus.subrace[RC_DEMON] },  
        { "DemiHuman", target_sd->indexed_bonus.subrace[RC_DEMIHUMAN] },  
        { "Angel", target_sd->indexed_bonus.subrace[RC_ANGEL] },  
        { "Dragon", target_sd->indexed_bonus.subrace[RC_DRAGON] },  
        { "Player", target_sd->indexed_bonus.subrace[RC_PLAYER_HUMAN] + target_sd->indexed_bonus.subrace[RC_PLAYER_DORAM] },  
        { "ALL", target_sd->indexed_bonus.subrace[RC_ALL] },  
        { "Normal Class", target_sd->indexed_bonus.subclass[CLASS_NORMAL] },  
    };  
      
    // Output only non-zero resistance values    
    bool found_bonuses = false;
	
    for (int i = 0; i < 13; i++) {    
        if (race_resist_info[i].value != 0) {    
            output << "   -> " << race_resist_info[i].name << ": ^0000FF" << race_resist_info[i].value << "^000000%\n";  
            found_bonuses = true;  
        }  
    }    
      
    // Check if no bonuses were found and display message  
    if (!found_bonuses) {  
        output << "   -> No race resist bonuses found\n";    
    }  
      
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}

BUILDIN_FUNC(getskillbonus) {  
    class map_session_data *target_sd;  
      
    if (script_hasdata(st, 2)) {  
        const char *player_name = script_getstr(st, 2);  
        target_sd = map_nick2sd(player_name, true);  
        if (!target_sd) {  
            script_pushconststr(st, "Player not found");  
            return 0;  
        }  
    } else {  
        if (!script_rid2sd(target_sd)) {  
            script_pushconststr(st, "No player attached");  
            return 0;  
        }  
    } 
      
    std::ostringstream output;  
    output << "|--- Skill Damage Bonuses ---|\n";  
      
    bool found_bonuses = false;  
      
    // Check the skillatk vector for skill-specific bonuses  
    for (const auto &bonus : target_sd->skillatk) {  
        const char* skill_desc = skill_get_desc(bonus.id);  // Use description instead of name  
        if (skill_desc) {  
            output << "   -> " << skill_desc << ": +^0000FF" << bonus.val << "^000000%\n";  
            found_bonuses = true;  
        }  
    }  
      
    // General skill ratio bonus  
    if (target_sd->bonus.skill_ratio != 0) {  
        output << "   -> All Skills: ^0000FF" << target_sd->bonus.skill_ratio << "^000000%\n";  
        found_bonuses = true;  
    }  
      
    if (!found_bonuses) {  
        output << "   -> No skill damage bonuses found\n";  
    }  
      
    std::string result = output.str();  
    script_pushstr(st, aStrdup(result.c_str()));  
    return 0;  
}

BUILDIN_FUNC(getdepositstore)
{
	int count = 0;

	for (const auto& pair : deposit_db)
	{
		std::shared_ptr<s_deposit_stor> stor = pair.second;

		if (stor->items.empty())
			continue;

		setd_sub_num(st, NULL, ".@stor_id", count, (uint8)stor->stor_id, NULL);
		setd_sub_str(st, NULL, ".@stor_name$", count++, storage_getName((uint8)stor->stor_id), NULL);
	}

	script_pushint(st, count);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getdepositlist)
{
	map_session_data* sd;

	if (script_hasdata(st, 3) && script_isstring(st, 3)) // Character Name
	{
		if (!script_nick2sd(3, sd))
			return SCRIPT_CMD_FAILURE;
	}
	else // Account ID
	{
		if (!script_accid2sd(3, sd))
			return SCRIPT_CMD_FAILURE;
	}

	int j = 0, k = 0;
	int stor_id = script_getnum(st, 2);
	std::shared_ptr<s_deposit_stor> stor = deposit_db.find(stor_id);
	if (stor != nullptr)
	{
		const auto& list = sd->deposit.items[stor->stor_id];
		for (std::shared_ptr<s_deposit_item> entry : stor->items)
		{
			t_itemid nameid = entry->nameid;

			auto idx = std::find_if(list.begin(), list.end(),
				[&nameid](const s_deposit_items& s) { return (s.nameid == nameid); });

			if (idx != list.end())
			{
				setd_sub_num(st, NULL, ".@amount2", j, idx->amount, NULL);
				setd_sub_num(st, NULL, ".@refine2", j, idx->refine, NULL);
				if (idx->amount >= entry->amount && idx->refine >= entry->refine)
				{
					setd_sub_num(st, NULL, ".@flag", j, 1, NULL);
					setd_sub_num(st, NULL, ".@count", 0, ++k, NULL);
				}
			}

			setd_sub_num(st, NULL, ".@amount", j, entry->amount, NULL);
			setd_sub_num(st, NULL, ".@refine", j, entry->refine, NULL);
			setd_sub_num(st, NULL, ".@nameid", j++, entry->nameid, NULL);
		}
	}

	script_pushint(st, j);

	return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(getdepositbonus)
{
	map_session_data* sd;

	if (script_hasdata(st, 2) && script_isstring(st, 2))
	{
		if (!script_nick2sd(2, sd))
			return SCRIPT_CMD_FAILURE;
	}
	else
	{
		if (!script_accid2sd(2, sd))
			return SCRIPT_CMD_FAILURE;
	}

	int j = 0;
	const auto& list = sd->deposit.bonus;
	for (const auto& it : list) {
		setd_sub_num(st, NULL, ".@type", j, it.type, NULL);
		setd_sub_num(st, NULL, ".@val1", j, it.val1, NULL);
		setd_sub_num(st, NULL, ".@val2", j, it.val2, NULL);
		j++;
	}

	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}
