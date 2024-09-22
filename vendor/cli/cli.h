#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <unordered_map>
#include <stdint.h>

namespace cli {
    enum ValueType {
        VAL_TYPE_INVALID = -1,
        VAL_TYPE_STRING,
        VAL_TYPE_UNSIGNED_INTEGER,
        VAL_TYPE_SIGNED_INTEGER,
        VAL_TYPE_FLOATING,
        VAL_TYPE_BOOLEAN
    };

    class Value { 
    public:
        // Default constructor
        Value();

        /**
         * Create a Value object from a common type.
         * @param value Value to create the object from.
        */
        Value(const char* value);
        Value(const std::string& value);
        Value(uint8_t value);
        Value(uint16_t value);
        Value(uint32_t value);
        Value(uint64_t value);
        Value(int8_t value);
        Value(int16_t value);
        Value(int32_t value);
        Value(int64_t value);
        Value(float value);
        Value(double value);
        Value(bool value);

        /**
         * Create value by parsing a string.
         * @param type Expected value type.
         * @param str String to parse.
        */
        Value(ValueType type, const std::string& str);

        // Copy constructor
        Value(const Value& b);

        // Move constructor
        Value(Value&& b);

        // Copy assignment operator
        Value& operator=(const Value& b);

        // Move assignment operator
        Value& operator=(Value&& b);

        // Cast operator
        operator const std::string&() const;
        operator uint8_t() const;
        operator uint16_t() const;
        operator uint32_t() const;
        operator uint64_t() const;
        operator int8_t() const;
        operator int16_t() const;
        operator int32_t() const;
        operator int64_t() const;
        operator float() const;
        operator double() const;
        operator bool() const;

        // Type of the value
        const ValueType& type = _type;

    private:
        ValueType _type;
        std::string string;
        uint64_t uinteger;
        int64_t sinteger;
        double floating;
        bool boolean;
    };

    class Command;

    class Interface {
    public:
        // Default constructor
        Interface();

        // Copy constructor
        Interface(const Interface& b);

        // Move constructor
        Interface(Interface&& b);

        // Copy assignment operator
        Interface& operator=(const Interface& b);

        // Move assignment operator
        Interface& operator=(Interface&& b);

        /**
         * Define an argument.
         * @param name Long name of the argument.
         * @param alias Short name of the argument. Zero if none.
         * @param defValue Default value that the argument has if not given by the user.
         * @param description Description of the argument.
        */
        void arg(const std::string& name, char alias, Value defValue, const std::string& description);

        /**
         * Define a sub-command.
         * @param name Name of the subcommand.
         * @param interface Interface definition of the subcommand.
         * @param description Description of the subcommand.
        */
        void subcmd(const std::string& name, const Interface& interface, const std::string& description);

        // Friends
        friend Command;
        friend Command parse(const Interface& interface, int argc, char** argv);

    private:
        struct Argument {
            Value defValue;
            std::string desc;
        };

        struct SubCommand;

        std::map<char, std::string> aliases;
        std::unordered_map<std::string, Argument> arguments;
        std::unordered_map<std::string, SubCommand> subcommands;
    };

    struct Interface::SubCommand {
        Interface iface;
        std::string desc;
    };

    class Command {
    public:
        // Default constructor
        Command();

        /**
         * Create a command.
         * @param command Command run by the user.
         * @param interface Interface of the command.
        */
        Command(const std::string& command, const Interface& interface);

        // Copy constructor
        Command(const Command& b);

        // Move constructor
        Command(Command&& b);

        // Copy assignment operator
        Command& operator=(const Command& b);

        // Move assignment operator
        Command& operator=(Command&& b);

        // Cast to string operator
        operator const std::string&() const;

        // Equality operators
        bool operator==(const std::string& b) const;
        bool operator==(const char* b) const;

        const Value& operator[](const std::string& arg) const;

        // Friends
        friend Command parse(const Interface& interface, int argc, char** argv);

        std::shared_ptr<Command> subcommand;

    //private:
        std::string command;
        std::unordered_map<std::string, Value> arguments;
        std::vector<std::string> values;
    };

    /**
     * Parse a command line.
     * @param argc Argument count.
     * @param argv Argument list.
     * @param interface Command line interface.
     * @return User command.
    */
    Command parse(const Interface& interface, int argc, char** argv);
}