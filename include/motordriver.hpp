#pragma once

#include <stdint.h>
#include <iostream>
#include <memory>

class MotorDriver {
public:
    MotorDriver(bool doInit);
    ~MotorDriver();

    bool init();
    bool enablePowerLines();

    bool zoomUp(int steps);
    bool zoomDown(int steps);
    bool zoomDir(int dir, int steps);
    bool zoomHome();
    bool zoomAbsolute(int steps);
    bool zoomRelative(int steps);
    int  zoomAbsoluteLocation() { return zoom_abs_location; }

    bool focusUp(int steps);
    bool focusDown(int steps);
    bool focusDir(int dir, int steps);
    bool focusHome();
    bool focusAbsolute(int steps);
    bool focusRelative(int steps);
    int  focusAbsoluteLocation() { return focus_abs_location; }

    bool irisUp(int steps);
    bool irisDown(int steps);
    bool irisDir(int dir, int steps);
    bool irisHome();
    bool irisAbsolute(int steps);
    bool irisRelative(int steps);
    int  irisAbsoluteLocation() { return iris_abs_location; }

    bool printZoom();
    bool printFocus();
    bool printIris();
    bool printPower();

private:
    enum class MotorDirection { UP, DOWN };
    bool initExpanders();
    bool writeReg(uint8_t addr, uint8_t regaddr, uint8_t data);
    bool readReg(uint8_t addr, uint8_t regaddr, uint8_t& res);
    bool printReg(uint8_t addr, uint8_t regaddr);

    bool enableZoom(MotorDirection dir);
    bool disableZoom();
    bool zoom(int steps);
    bool zoomLimit();

    bool enableFocus(MotorDirection dir);
    bool disableFocus();
    bool focus(int steps);
    bool focusLimit();

    bool enableIris(MotorDirection dir);
    bool disableIris();
    bool iris(int steps);

    int fd_;
    static int addr1, addr2;
    char exp1_gpioa, exp1_gpiob, exp2_gpioa, exp2_gpiob;
    bool zoom_init;
    int zoom_abs_location;
    bool focus_init;
    int focus_abs_location;
    bool iris_init;
    int iris_abs_location;
};

using MotorDriverPtr = std::shared_ptr<MotorDriver>;
