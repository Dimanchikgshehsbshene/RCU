#pragma once

#include <cstdint>
#include <string>

#include "../common/include/common.hpp"

namespace board {

class BoardSensor {
public:
    BoardSensor() = default;
    ~BoardSensor() = default;

    bool Initialize();
    float GetTemperature() const;
    float GetVoltage() const;
    float GetCurrent() const;

private:
    bool initialized_ = false;
    float temperature_ = 0.0f;
    float voltage_ = 0.0f;
    float current_ = 0.0f;
};

} // namespace board
