#pragma once
#include "../core/api.hpp"
#include "../core/api.hpp"
#include "../core/host_manager.hpp"
using namespace std;

bool create_new_block(HostManager& hosts, Block& newBlock, PublicWalletAddress wallet);
int new_template_height(HostManager& hosts);
Block get_block_template(HostManager& hosts, PublicWalletAddress wallet);
json get_work_template(HostManager& hosts, PublicWalletAddress wallet);
