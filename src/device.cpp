#include "device.h"
#include <map>
#include <format>

namespace dev {
    std::map<std::string, std::unique_ptr<Driver>> drivers;

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

    std::shared_ptr<Receiver> openRX(const std::string& driver, const std::string& identifier) {
        // Fetch the driver or throw error
        auto drvit = drivers.find(driver);
        if (drvit == drivers.end()) {
            throw std::runtime_error(std::format("Unknown device driver: '{}'", driver));
        }

        // Open the device for RX
        return drvit->second->openRX(identifier);
    }

    std::shared_ptr<Transmitter> openTX(const std::string& driver, const std::string& identifier, dsp::stream<dsp::complex_t>* in) {
        // Fetch the driver or throw error
        auto drvit = drivers.find(driver);
        if (drvit == drivers.end()) {
            throw std::runtime_error(std::format("Unknown device driver: '{}'", driver));
        }

        // Open the device for RX
        return drvit->second->openTX(identifier, in);
    }
}