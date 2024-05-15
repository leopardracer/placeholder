#include "vm_host.h"

#include <intx/intx.hpp>
#include <evmone/evmone.h>
#include <ethash/keccak.hpp>

extern "C" {

evmc_host_context* vm_host_create_context(evmc_tx_context tx_context, std::shared_ptr<nil::blueprint::handler_base> handler)
{
    auto host = new VMHost{tx_context, handler};
    return host->to_context();
}

void vm_host_destroy_context(evmc_host_context* context)
{
    delete evmc::Host::from_context<VMHost>(context);
}
}

evmc::Result VMHost::handle_call(const evmc_message& msg)
{
    evmc_vm * vm = evmc_create_evmone();
    auto account_iter = accounts.find(msg.code_address);
    auto &sender_acc = accounts[msg.sender];
    if (account_iter == accounts.end())
    {
        // Create account
        accounts[msg.code_address] = {};
    }
    auto& acc = account_iter->second;
    if (msg.kind == EVMC_CALL) {
        auto value_to_transfer = intx::be::load<intx::uint256>(msg.value);
        auto balance = intx::be::load<intx::uint256>(sender_acc.balance);
        // Balance was already checked in evmone, so simply adjust it
        sender_acc.balance = intx::be::store<evmc::uint256be>(balance - value_to_transfer);
        acc.balance = intx::be::store<evmc::uint256be>(
            value_to_transfer + intx::be::load<intx::uint256>(acc.balance));
    }
    if (acc.code.empty())
    {
        return evmc::Result{EVMC_SUCCESS, msg.gas, 0, msg.input_data, msg.input_size};
    }
    // TODO: handle precompiled contracts
    auto res = evaluate(handler, vm, &get_interface(), to_context(),
        EVMC_LATEST_STABLE_REVISION, &msg, acc.code.data(), acc.code.size());
    return res;
}

evmc::Result VMHost::handle_create(const evmc_message& msg)
{
    evmc::address new_contract_address = calculate_address(msg);
    if (accounts.find(new_contract_address) != accounts.end())
    {
        // Address collision
        return evmc::Result{EVMC_FAILURE};
    }
    accounts[new_contract_address] = {};
    if (msg.input_size == 0)
    {
        return evmc::Result{EVMC_SUCCESS, msg.gas, 0, new_contract_address};
    }
    evmc::VM vm{evmc_create_evmone()};
    evmc_message init_msg(msg);
    init_msg.kind = EVMC_CALL;
    init_msg.recipient = new_contract_address;
    init_msg.sender = msg.sender;
    init_msg.input_size = 0;
    auto res = evaluate(handler, vm.get_raw_pointer(), &get_interface(), to_context(),
        EVMC_LATEST_STABLE_REVISION, &init_msg, msg.input_data, msg.input_size);

    if (res.status_code == EVMC_SUCCESS)
    {
        accounts[new_contract_address].code =
            std::vector<uint8_t>(res.output_data, res.output_data + res.output_size);
    }
    res.create_address = new_contract_address;
    return res;
}

evmc::address VMHost::calculate_address(const evmc_message& msg)
{
    // TODO: Implement for CREATE opcode, for now the result is only correct for CREATE2
    // CREATE requires rlp encoding
    auto seed = intx::be::load<intx::uint256>(msg.create2_salt);
    auto hash = intx::be::load<intx::uint256>(ethash::keccak256(msg.input_data, msg.input_size));
    auto sender = intx::be::load<intx::uint256>(msg.sender);
    auto sum = 0xff + seed + hash + sender;
    auto rehash = ethash::keccak256(reinterpret_cast<const uint8_t *>(&sum), sizeof(sum));
    // Result address is the last 20 bytes of the hash
    evmc::address res;
    std::memcpy(res.bytes, rehash.bytes + 12, 20);
    return res;
}
