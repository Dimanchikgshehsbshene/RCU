#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"

namespace board {

class BoardSensor {
public:
    BoardSensor() = default;
    ~BoardSensor() = default;

    bool Initialize();
    void Update();
    void Shutdown();

    float GetTemperature() const { return m_temperature; }
    float GetFanSpeed() const { return m_fan_speed; }

private:
    float m_temperature{0.0f};
    float m_fan_speed{0.0f};
};

} // namespace board
