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
