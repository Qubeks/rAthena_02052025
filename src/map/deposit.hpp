#ifndef DEPOSIT_HPP
#define DEPOSIT_HPP

#include <vector>

#include <common/database.hpp>
#include <common/db.hpp>
#include <common/malloc.hpp>
#include <common/mmo.hpp>

#include "script.hpp"
#include "status.hpp"

struct s_deposit_item {
	t_itemid nameid;
	uint16 amount;
	char refine;
	bool withdraw;
	int32 deposit_fee;
	int32 withdraw_fee;
	struct script_code* script;

	~s_deposit_item() {
		if (this->script) {
			script_free_code(this->script);
			this->script = nullptr;
		}
	}
};

struct s_deposit_stor {
	uint16 stor_id;
	bool withdraw;
	bool bound;
	int32 deposit_fee;
	int32 withdraw_fee;
	std::vector<std::shared_ptr<s_deposit_item>> items;
};

class DepositDatabase : public TypesafeCachedYamlDatabase<uint16, s_deposit_stor> {
public:
	DepositDatabase() : TypesafeCachedYamlDatabase("DEPOSIT_DB", 1, 1) {

	}

	const std::string getDefaultLocation() override;
	uint64 parseBodyNode(const ryml::NodeRef& node) override;

	// Additional
	std::shared_ptr<s_deposit_item> findItemInStor(uint16 stor_id, t_itemid nameid);
};

extern DepositDatabase deposit_db;

void deposit_counter(map_session_data* sd, int type, int val1, int val2);
void deposit_save(map_session_data* sd, bool calc = true);
void do_init_deposit(void);
void do_final_deposit(void);
void do_reload_deposit(void);

#endif
