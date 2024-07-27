#include "cli.h"
#include <stdexcept>
#include <string.h>
#include <format>

namespace cli {
    const std::vector<std::string> trueStrings = { "TRUE", "Y", "YES", "ON", "1" };
    const std::vector<std::string> falseStrings = { "FALSE", "N", "NO", "OFF", "0" };

    Value::Value() : _type(VAL_TYPE_INVALID) {}

    Value::Value(const char* value) : _type(VAL_TYPE_STRING) { string = value; }
    Value::Value(const std::string& value) : _type(VAL_TYPE_STRING) { string = value; }
    Value::Value(uint8_t value) : _type(VAL_TYPE_UNSIGNED_INTEGER) { uinteger = value; }
    Value::Value(uint16_t value) : _type(VAL_TYPE_UNSIGNED_INTEGER) { uinteger = value; }
    Value::Value(uint32_t value) : _type(VAL_TYPE_UNSIGNED_INTEGER) { uinteger = value; }
    Value::Value(uint64_t value) : _type(VAL_TYPE_UNSIGNED_INTEGER) { uinteger = value; }
    Value::Value(int8_t value) : _type(VAL_TYPE_SIGNED_INTEGER) { sinteger = value; }
    Value::Value(int16_t value) : _type(VAL_TYPE_SIGNED_INTEGER) { sinteger = value; }
    Value::Value(int32_t value) : _type(VAL_TYPE_SIGNED_INTEGER) { sinteger = value; }
    Value::Value(int64_t value) : _type(VAL_TYPE_SIGNED_INTEGER) { sinteger = value; }
    Value::Value(float value) : _type(VAL_TYPE_FLOATING) { floating = value; }
    Value::Value(double value) : _type(VAL_TYPE_FLOATING) { floating = value;; }
    Value::Value(bool value) : _type(VAL_TYPE_BOOLEAN) { boolean = value; }

    Value::Value(ValueType type, const std::string& str) : _type(type) {
        std::string upperStr;

        switch (type) {
        case VAL_TYPE_STRING:
            // Copy the string directly
            string = str;
            return;
        case VAL_TYPE_UNSIGNED_INTEGER:
            // Parse signed integer or throw error
            try {
                uinteger = std::stoull(str);
            }
            catch (const std::exception& e) {
                throw std::runtime_error(std::format("Expected an unsigned integer value, got '{}'", str));
            }
            return;
        case VAL_TYPE_SIGNED_INTEGER:
            // Parse signed integer or throw error
            try {
                sinteger = std::stoll(str);
            }
            catch (const std::exception& e) {
                throw std::runtime_error(std::format("Expected a signed integer value, got '{}'", str));
            }
            return;
        case VAL_TYPE_FLOATING:
            // Parse float or throw error
            try {
                floating = std::stod(str);
            }
            catch (const std::exception& e) {
                throw std::runtime_error(std::format("Expected a floating point value, got '{}'", str));
            }
            return;
        case VAL_TYPE_BOOLEAN:
            // Convert to upper case
            for (char c : str) { upperStr += std::toupper(c); }

            // Check for a true value
            if (std::find(trueStrings.begin(), trueStrings.end(), upperStr) != trueStrings.end()) {
                boolean = true;
                return;
            }

            // Check for false strings
            if (std::find(falseStrings.begin(), falseStrings.end(), upperStr) != falseStrings.end()) {
                boolean = false;
                return;
            }

            // Invalid, throw error
            throw std::runtime_error(std::format("Expected a boolean value, got '{}'", str));
            return;
        default:
            throw std::runtime_error("Unsupported type");
        }
    }

    Value::Value(const Value& b) : _type(b._type) {
        // Copy the member of the designated type
        switch (_type) {
        case VAL_TYPE_STRING:
            string = b.string; break;
        case VAL_TYPE_UNSIGNED_INTEGER:
            uinteger = b.uinteger; break;
        case VAL_TYPE_SIGNED_INTEGER:
            sinteger = b.sinteger; break;
        case VAL_TYPE_FLOATING:
            floating = b.floating; break; 
        case VAL_TYPE_BOOLEAN:
            boolean = b.boolean; break; 
        default:
            break;
        }
    }

    Value::Value(Value&& b) : _type(b._type) {
        // Move the member of the designated type
        switch (_type) {
        case VAL_TYPE_STRING:
            string = std::move(b.string); break;
        case VAL_TYPE_UNSIGNED_INTEGER:
            uinteger = b.uinteger; break;
        case VAL_TYPE_SIGNED_INTEGER:
            sinteger = b.sinteger; break;
        case VAL_TYPE_FLOATING:
            floating = b.floating; break; 
        case VAL_TYPE_BOOLEAN:
            boolean = b.boolean; break;
        default:
            break;
        }
    }

    Value& Value::operator=(const Value& b) {
        // Update the type
        _type = b._type;

        // Copy the member of the designated type
        switch (_type) {
        case VAL_TYPE_STRING:
            string = b.string; break;
        case VAL_TYPE_UNSIGNED_INTEGER:
            uinteger = b.uinteger; string.clear(); break;
        case VAL_TYPE_SIGNED_INTEGER:
            sinteger = b.sinteger; string.clear(); break;
        case VAL_TYPE_FLOATING:
            floating = b.floating; string.clear(); break;
        case VAL_TYPE_BOOLEAN:
            boolean = b.boolean; string.clear(); break;
        default:
            break;
        }

        // Return self
        return *this;
    }

    Value& Value::operator=(Value&& b) {
        // Update the type
        _type = b._type;

        // Move the member of the designated type
        switch (_type) {
        case VAL_TYPE_STRING:
            string = std::move(b.string); break;
        case VAL_TYPE_UNSIGNED_INTEGER:
            uinteger = b.uinteger; string.clear(); break;
        case VAL_TYPE_SIGNED_INTEGER:
            sinteger = b.sinteger; string.clear(); break;
        case VAL_TYPE_FLOATING:
            floating = b.floating; string.clear(); break;
        case VAL_TYPE_BOOLEAN:
            boolean = b.boolean; string.clear(); break;
        default:
            break;
        }

        // Return self
        return *this;
    }

    Value::operator const std::string&() const {
        if (_type != VAL_TYPE_STRING) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return string;
    }

    Value::operator uint8_t() const {
        if (_type != VAL_TYPE_UNSIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (uint8_t)uinteger;
    }

    Value::operator uint16_t() const {
        if (_type != VAL_TYPE_UNSIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (uint16_t)uinteger;
    }

    Value::operator uint32_t() const {
        if (_type != VAL_TYPE_UNSIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (uint32_t)uinteger;
    }

    Value::operator uint64_t() const {
        if (_type != VAL_TYPE_UNSIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return uinteger;
    }

    Value::operator int8_t() const {
        if (_type != VAL_TYPE_SIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (int8_t)sinteger;
    }

    Value::operator int16_t() const {
        if (_type != VAL_TYPE_SIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (int16_t)sinteger;
    }

    Value::operator int32_t() const {
        if (_type != VAL_TYPE_SIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (int32_t)sinteger;
    }

    Value::operator int64_t() const {
        if (_type != VAL_TYPE_SIGNED_INTEGER) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return sinteger;
    }

    Value::operator float() const {
        if (_type != VAL_TYPE_FLOATING) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return (float)floating;
    }

    Value::operator double() const {
        if (_type != VAL_TYPE_FLOATING) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return floating;
    }

    Value::operator bool() const {
        if (_type != VAL_TYPE_BOOLEAN) { throw std::runtime_error("Cannot cast value due to type mismatch"); }
        return boolean;
    }

    Interface::Interface() {}

    Interface::Interface(const Interface& b) {
        // Copy all members
        aliases = b.aliases;
        arguments = b.arguments;
        subcommands = b.subcommands;
    }

    Interface::Interface(Interface&& b) {
        // Move all members
        aliases = std::move(b.aliases);
        arguments = std::move(b.arguments);
        subcommands = std::move(b.subcommands);
    }

    Interface& Interface::operator=(const Interface& b) {
        // Copy all members
        aliases = b.aliases;
        arguments = b.arguments;
        subcommands = b.subcommands;

        // Return self
        return *this;
    }

    Interface& Interface::operator=(Interface&& b) {
        // Move all members
        aliases = std::move(b.aliases);
        arguments = std::move(b.arguments);
        subcommands = std::move(b.subcommands);

        // Return self
        return *this;
    }

    void Interface::arg(const std::string& name, char alias, Value defValue, const std::string& description) {
        // Check if an argument with that name already exists
        if (arguments.find(name) != arguments.end()) {
            throw std::runtime_error("An argument with the given name already exists");
        }

        // If an alias was given
        if (alias) {
            // Check if an alias with that character already exists
            if (aliases.find(alias) != aliases.end()) {
                throw std::runtime_error("An argument with the given alias already exists");
            }

            // Save the alias
            aliases[alias] = name;
        }

        // Save the argument
        Argument arg = { defValue, description };
        arguments[name] = arg;
    }

    void Interface::subcmd(const std::string& name, const Interface& interface, const std::string& description) {
        // Check if a subcommand of that name does not exist yet
        if (subcommands.find(name) != subcommands.end()) {
            throw std::runtime_error("A subcommand with the given name already exists");
        }

        // Save the interface of the subcommand
        SubCommand scmd = { interface, description };
        subcommands[name] = scmd;
    }

    Command::Command() {}

    Command::Command(const std::string& command, const Interface& interface) {
        // Save the command
        this->command = command;

        // Go through all defined arguments
        for (const auto& [name, arg] : interface.arguments) {
            // Initialize the argument with the default value
            arguments[name] = arg.defValue;
        }
    }

    Command::Command(const Command& b) {
        // Copy all members
        subcommand = b.subcommand;
        command = b.command;
        arguments = b.arguments;
        values = b.values;
    }

    Command::Command(Command&& b) {
        // Move all members
        subcommand = std::move(b.subcommand);
        command = std::move(b.command);
        arguments = std::move(b.arguments);
        values = std::move(b.values);
    }

    Command& Command::operator=(const Command& b) {
        // Copy b since it could be allocated by this->subcommand
        Command bcpy = b;

        // Move all members from the copy to self
        subcommand = std::move(bcpy.subcommand);
        command = std::move(bcpy.command);
        arguments = std::move(bcpy.arguments);
        values = std::move(bcpy.values);

        // Return self
        return *this;
    }

    Command& Command::operator=(Command&& b) {
        // Move b since it could be allocated by this->subcommand
        Command bcpy = std::move(b);

        // Move all members from the copy to self
        subcommand = std::move(bcpy.subcommand);
        command = std::move(bcpy.command);
        arguments = std::move(bcpy.arguments);
        values = std::move(bcpy.values);

        // Return self
        return *this;
    }

    Command::operator const std::string&() const {
        return command;
    }

    bool Command::operator==(const std::string& b) const {
        return (command == b);
    }

    bool Command::operator==(const char* b) const {
        return !strcmp(command.c_str(), b);
    }

    const Value& Command::operator[](const std::string& arg) const {
        // Get value from argument list
        return arguments.at(arg);
    }

    bool isValidBoolean(const std::string& str) {
        // Convert to upper case
        std::string upperStr;
        for (char c : str) { upperStr += std::toupper(c); }
        return std::find(trueStrings.begin(), trueStrings.end(), upperStr) != trueStrings.end() ||
                std::find(falseStrings.begin(), falseStrings.end(), upperStr) != falseStrings.end();
    }

    void parseArgument(Command& cmd, std::string& argName, ValueType type, int& argc, char**& argv) {
        // If the argument is a boolean
        if (type == VAL_TYPE_BOOLEAN) {
            // If no value follows it's not a valid boolean
            printf("'%s'\n", *argv);
            if (!argc || !isValidBoolean(*argv)) {
                // Assume the value is true
                cmd.arguments[argName] = true;
                return;
            }
        }

        // Expect a value
        if (!argc) { throw std::runtime_error("Expected a value"); }

        // Pop the value
        std::string value = *(argv++); argc--;

        // Parse and set the value
        cmd.arguments[argName] = Value(type, value);
    }

    Command parse(const Interface& interface, int argc, char** argv) {
        // Pop the command name
        std::string cmdName = *(argv++); argc--;

        // Initialize the command
        Command cmd(cmdName, interface);

        // Process until no arguments are left
        while (argc) {
            // Pop the argument
            std::string arg = *(argv++); argc--;

            // If the argument gives a long name
            if (arg.starts_with("--")) {
                // Verify that the command admits the given argument
                std::string argName = arg.substr(2);
                auto it = interface.arguments.find(argName);
                if (it == interface.arguments.end()) {
                    throw std::runtime_error(std::format("Unknown argument: '{}'", argName));
                }

                // Parse the argument
                parseArgument(cmd, argName, it->second.defValue.type, argc, argv);
                continue;
            }

            // Otherwise if the argument gives aliases
            if (arg.starts_with("-")) {
                // Iterate through each alias
                for (int i = 1; i < arg.size(); i++) {
                    // Get the alias
                    char c = arg[i];

                    // Verify that the command admits the given alias
                    auto it = interface.aliases.find(c);
                    if (it == interface.aliases.end()) {
                        throw std::runtime_error(std::format("Unknown argument: '{}'", c));
                    }

                    // Fetch the argument descriptor
                    std::string argName = it->second;
                    const auto& desc = interface.arguments.at(argName);

                    // If the argument is not at the end of the compound argument
                    if (i < arg.size()-1) {
                        // if the argument is a boolean
                        if (desc.defValue.type == VAL_TYPE_BOOLEAN) {
                            // Assume the value is true
                            cmd.arguments[argName] = true;
                            continue;
                        }

                        // Non-boolean arguments are not allowed before the end of the compound argument
                        throw std::runtime_error(std::format("Non boolean argument '{}' can only be at the end of a compound argument", c));
                    }

                    // Parse the argument
                    parseArgument(cmd, argName, desc.defValue.type, argc, argv);
                }
                continue;
            }

            // If the interface needs a subcommand
            if (!interface.subcommands.empty()) {
                // Verify that the command admits the given subcommand
                auto it = interface.subcommands.find(arg);
                if (it == interface.subcommands.end()) {
                    throw std::runtime_error(std::format("Unknown sub-command: '{}'", arg));
                }

                // Parse the subcommand and finish
                cmd.subcommand = std::make_shared<Command>(parse(it->second.iface, ++argc, --argv));
                break;
            }

            // Just append the argument to the value list
            cmd.values.push_back(arg);
        }

        // Return the newly parsed command
        return cmd;
    }
}