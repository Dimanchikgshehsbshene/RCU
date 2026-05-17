#pragma once

#include <cstdint>
#include <string>
#include "common/common.hpp"

namespace board {

class BoardSensor {
public:
    BoardSensor();
    ~BoardSensor();

    bool initialize();
    void shutdown();

    float getTemperature() const;
    float getVoltage() const;
    float getCurrent() const;

    std::string getBoardName() const;
    std::string getBoardVersion() const;

private:
    bool initialized_;
    float temperature_;
    float voltage_;
    float current_;
    std::string board_name_;
    std::string board_version_;
};

} // namespace board
