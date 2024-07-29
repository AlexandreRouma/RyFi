#include "device.h"
#include <map>
#include <format>

#include "flog/flog.h"

namespace dev {
    std::map<std::string, std::unique_ptr<Driver>> drivers;

    Receiver::~Receiver() {}

    Transmitter::~Transmitter() {}

    std::shared_ptr<Receiver> Driver::openRX(const std::string& identifier) {
        throw std::runtime_error("This driver does not support receiving");
    }

    std::shared_ptr<Transmitter> Driver::openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in) {
        throw std::runtime_error("This driver does not support transmitting");
    }

    void registerDriver(const std::string& name, std::unique_ptr<Driver> driver) {
        // Check that no driver with the given name already exists
        if (drivers.find(name) != drivers.end()) {
            throw std::runtime_error(std::format("Cannot add driver, a driver with the name '{}' already exists", name));
        }

        // Add the driver to the list
        drivers[name] = std::move(driver);
    }

    std::vector<std::string> listDrivers() {
        // Create the list
        std::vector<std::string> list;

        // Append each driver's name
        for (const auto& [ name, drv ] : drivers) {
            list.push_back(name);
        }

        // Return the generated list
        return list;
    }

    std::vector<Info> list() {
        // Create the list
        std::vector<Info> list;

        // Go through each driver
        for (auto& [name, driver] : drivers) {
            // Request a list of devices
            auto devList = driver->list();

            // Append the devices to the list
            for (const auto& inf : devList) {
                list.push_back(inf);
            }
        }

        // Return the generated list
        return list;
    }

    std::map<std::string, std::unique_ptr<Driver>>::iterator selectDevice(const std::string& device, std::string& ident) {
        // Find the column
        auto it = device.find(':');

        // If none was found
        std::string driver;
        if (it == std::string::npos) {
            // Set the driver as the given device and clear the identifier
            driver = device;
            ident.clear();
        }
        else {
            // Split the device string into the two parts
            driver = device.substr(0, it);
            ident = device.substr(it+1);
        }

        // Fetch the driver or throw error
        auto drvit = drivers.find(driver);
        if (drvit == drivers.end()) {
            throw std::runtime_error(std::format("Unknown device driver: '{}'", driver));
        }

        // If no identifier was given
        if (ident.empty()) {
            // Get the list of available devices from the driver
            auto list = drvit->second->list();

            // Throw an error if none is available
            if (list.empty()) { throw std::runtime_error("Could not find any device using the selected driver"); }

            // Select the first one
            ident = list[0].identifier;
        }

        // Return the driver entry
        return drvit;
    }

    std::shared_ptr<Receiver> openRX(const std::string& device) {
        // Get the driver and identifier
        std::string ident;
        auto drvit = selectDevice(device, ident);

        // Open the device for RX
        return drvit->second->openRX(ident);
    }

    std::shared_ptr<Transmitter> openTX(const std::string& device, dsp::stream<dsp::complex_t>* in) {
        // Get the driver and identifier
        std::string ident;
        auto drvit = selectDevice(device, ident);

        // Open the device for TX
        return drvit->second->openTX(ident, in);
    }
}