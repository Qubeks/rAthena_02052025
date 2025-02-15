#include "deposit.hpp"

#include <stdlib.h>

#include "../common/malloc.hpp"
#include "../common/nullpo.hpp"
#include "../common/showmsg.hpp"
#include "../common/strlib.hpp"
#include "../common/utils.hpp"

#include "pc.hpp"
#include "storage.hpp"

const std::string DepositDatabase::getDefaultLocation()
{
	return std::string(db_path) + "/deposit_db.yml";
}

uint64 DepositDatabase::parseBodyNode(const ryml::NodeRef& node)
{
	uint16 stor_id;

	if (!this->asUInt16(node, "StorageId", stor_id))
		return 0;

	std::shared_ptr<s_deposit_stor> deposit = this->find(stor_id);
	bool exists = deposit != nullptr;

	if (!exists)
	{
		if (!this->nodesExist(node, { "Items" }))
			return 0;

		deposit = std::make_shared<s_deposit_stor>();
		deposit->stor_id = stor_id;

		if (this->nodeExists(node, "Bound"))
		{
			bool bound;

			if (!this->asBool(node, "Bound", bound))
				return 0;

			deposit->bound = bound;
		}
		else
		{
			if (!exists)
				deposit->bound = true;
		}
	}

	if (this->nodeExists(node, "Withdraw"))
	{
		bool withdraw;

		if (!this->asBool(node, "Withdraw", withdraw))
			return 0;

		deposit->withdraw = withdraw;
	}
	else
	{
		if (!exists)
			deposit->withdraw = true;
	}

	if (this->nodeExists(node, "WithdrawFee"))
	{
		int32 withdraw_fee;

		if (!this->asInt32(node, "WithdrawFee", withdraw_fee))
			return 0;

		deposit->withdraw_fee = withdraw_fee;
	}
	else
	{
		if (!exists)
			deposit->withdraw_fee = 0;
	}

	for (const ryml::NodeRef& it : node["Items"])
	{
		std::string item_name;

		if (!this->asString(it, "Item", item_name))
			return 0;

		std::shared_ptr<item_data> item = item_db.search_aegisname(item_name.c_str());

		if (item == nullptr)
		{
			this->invalidWarning(it["Item"], "Deposit item %s does not exist, skipping.\n", item_name.c_str());
			continue;
		}

		std::shared_ptr<s_deposit_item> entry = nullptr;
		bool new_entry = true;

		for (std::shared_ptr<s_deposit_item> ditem : deposit->items)
		{
			if (ditem->nameid == item->nameid)
			{
				entry = ditem;
				new_entry = false;
				break;
			}
		}

		if (new_entry)
		{
			entry = std::make_shared<s_deposit_item>();
			entry->nameid = item->nameid;
		}

		if (this->nodeExists(it, "Amount"))
		{
			uint16 amount;

			if (!this->asUInt16(it, "Amount", amount))
				return 0;

			if (amount > MAX_AMOUNT)
			{
				this->invalidWarning(it["Amount"], "Amount exceeds MAX_AMOUNT. Capping...\n");
				amount = MAX_AMOUNT;
			}

			entry->amount = amount;
		}
		else
		{
			if (new_entry)
				entry->amount = 1;
		}

		if (this->nodeExists(it, "Refine"))
		{
			if (item->flag.no_refine) {
				this->invalidWarning(it["Refine"], "Item %s is not refineable.\n", item->name.c_str());
				return 0;
			}

			uint16 refine;

			if (!this->asUInt16(it, "Refine", refine))
				return 0;

			if (refine < 0 || refine > MAX_REFINE)
			{
				this->invalidWarning(it["Refine"], "Refine level %hu is invalid, skipping.\n", refine);
				return 0;
			}

			entry->refine = refine;
		}
		else
		{
			if (new_entry)
				entry->refine = 0;
		}

		if (this->nodeExists(it, "Withdraw"))
		{
			bool withdraw;

			if (!this->asBool(it, "Withdraw", withdraw))
				return 0;

			entry->withdraw = withdraw;
		}
		else
		{
			if (new_entry)
				entry->withdraw = false;
		}

		if (this->nodeExists(it, "WithdrawFee"))
		{
			int32 withdraw_fee;

			if (!this->asInt32(it, "WithdrawFee", withdraw_fee))
				return 0;

			entry->withdraw_fee = withdraw_fee;
		}
		else
		{
			if (!exists)
				entry->withdraw_fee = 0;
		}

		if (this->nodeExists(it, "Script"))
		{
			std::string script;

			if (!this->asString(it, "Script", script))
				return 0;

			if (!new_entry && item->script)
			{
				script_free_code(item->script);
				entry->script = nullptr;
			}

			entry->script = parse_script(script.c_str(), this->getCurrentFile().c_str(), this->getLineNumber(it["Script"]), SCRIPT_IGNORE_EXTERNAL_BRACKETS);
		}
		else
		{
			if (new_entry)
				entry->script = nullptr;
		}

		if (new_entry)
			deposit->items.push_back(entry);
	}

	if (!exists)
		this->put(stor_id, deposit);

	return 1;
}

std::shared_ptr<s_deposit_item> DepositDatabase::findItemInStor(uint16 stor_id, t_itemid nameid)
{
	std::shared_ptr<s_deposit_stor> deposit = this->find(stor_id);
	if (deposit == nullptr)
		return nullptr;

	for (std::shared_ptr<s_deposit_item> entry : deposit->items)
	{
		if (entry->nameid == nameid)
			return entry;
	}
	return nullptr;
}

DepositDatabase deposit_db;

void deposit_counter(map_session_data* sd, int type, int val1, int val2)
{
	if (!sd)
		return;

	if (!sd->deposit.calc)
		return;

	std::vector<s_deposit_bonus>& bonus = sd->deposit.bonus;
	for (auto& it : bonus)
	{
		if (it.type == type)
		{
			if (val2 && it.val1 == val1)
			{
				it.val2 = it.val2 + val2;
				return;
			}
			else {
				it.val1 = it.val1 + val1;
				return;
			}
		}
	}

	struct s_deposit_bonus entry = {};
	entry.type = type;

	if (val2)
	{
		entry.val1 = val1;
		entry.val2 += val2;
	}
	else
		entry.val1 += val1;

	bonus.push_back(entry);

	return;
}

void deposit_save(map_session_data* sd, bool calc)
{
	struct s_storage* stor = &sd->premiumStorage;

	std::shared_ptr<s_deposit_stor> deposit = deposit_db.find(stor->stor_id);
	if (deposit == nullptr)
		return;

	sd->deposit.items[stor->stor_id].clear();

	for (int i = 0; i < stor->max_amount; i++)
	{
		struct item* it = &stor->u.items_storage[i];

		if (it->nameid == 0)
			continue;

		std::shared_ptr<s_deposit_item> entry = deposit_db.findItemInStor(stor->stor_id, it->nameid);

		if (entry != nullptr)
		{
			struct s_deposit_items new_entry = {};

			new_entry.nameid = it->nameid;
			new_entry.amount = it->amount;
			new_entry.refine = it->refine;

			sd->deposit.items[stor->stor_id].push_back(new_entry);
		}
	}

	if (calc)
		status_calc_pc(sd, SCO_NONE);
}

void do_init_deposit(void)
{
	deposit_db.load();
}

void do_final_deposit(void)
{
	deposit_db.clear();
}

void do_reload_deposit(void)
{
	deposit_db.clear();
	deposit_db.load();

	struct s_mapiterator* iter = mapit_geteachpc();
	map_session_data* sd;

	for (sd = (map_session_data*)mapit_first(iter); mapit_exists(iter); sd = (map_session_data*)mapit_next(iter))
		status_calc_pc(sd, SCO_FORCE);

	mapit_free(iter);
}