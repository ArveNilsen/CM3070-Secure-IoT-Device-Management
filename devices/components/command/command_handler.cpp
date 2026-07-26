#include "command/command_handler.hpp"

#include <cassert>

using dev::CommandHandler;

CommandHandler::CommandHandler(CapabilityEnforcer& enforcer, 
        DeviceConfig& config)
    : enforcer_{enforcer}, config_{config} {}

void CommandHandler::handle(std::string_view topic,
            std::span<const uint8_t> payload)
{
    assert(false && "CommandHandler::handler not implemented");
}

void CommandHandler::handle_restrict(std::span<const uint8_t> payload)
{
    assert(false && "CommandHandler::handle_restrict not implemented");
}

void CommandHandler::handle_quarantine()
{
    assert(false && "CommandHandler::handle_quarantine not implemented");
}

void CommandHandler::handle_restore(std::span<const uint8_t> payload)
{
    assert(false && "CommandHandler::handle_restore not implemented");
}

void CommandHandler::handle_revoke()
{
    assert(false && "CommandHandler::handle_revoke not implemented");
}
