// System includes
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <chrono>
#include <thread>
#include <iostream>

#include "motordriver.hpp"
#include "configuration.hpp"

using namespace std;

// Addresses of the chips on the I2C bus
int MotorDriver::addr1 = 0x20;
int MotorDriver::addr2 = 0x21;

#define STEPPER_SLEEP_TIME_MS 1

#define ZOOM_LIMIT      0x01
#define ZOOM_DIR        0x02
#define ZOOM_ENABLE     0x04
#define ZOOM_STEP       0x08
#define FOCUS_LIMIT     0x01
#define FOCUS_DIR       0x02
#define FOCUS_ENABLE    0x04
#define FOCUS_STEP      0x08
#define IRIS_DIR        0x02
#define IRIS_ENABLE     0x04
#define IRIS_STEP       0x08

#define POWER_5V_EN        0x80
#define POWER_4V_EN        0x40
#define POWER_2_8V_EN      0x20
#define POWER_1_8V_EN      0x10
#define POWER_1_2V_EN      0x08

#define REG_IOCON 0x0A
#define REG_IODIRA 0x00
#define REG_IODIRB 0x01
#define REG_GPPUA 0x0C
#define REG_GPPUB 0x0D
#define REG_GPIOA 0x12
#define REG_GPIOB 0x13

#define REG_IOCON_VAL 0x00
// Output direction is value 0
#define REG_IODIRA_EXP1 0x01
#define REG_IODIRB_EXP1 0x01
#define REG_IODIRA_EXP2 0x01
#define REG_IODIRB_EXP2 0x00 // all outputs
#define REG_GPPUA_VAL 0xFF // enable all pull-ups
#define REG_GPPUB_VAL 0xFF
#define REG_GPIOA_EXP1 0x94 //sleep, reset and enable active low
#define REG_GPIOB_EXP1 0x94
#define REG_GPIOA_EXP2 0x94
#define REG_GPIOB_EXP2 0x00

#define DO_WRITE(addr,reg,data)   \
    {if (false == writeReg(addr,reg,data)) { bl_log_error("Write failed for addr: " << hex << addr << " reg: " << hex << reg << dec); return false;}}

MotorDriver::MotorDriver(bool doInit) : fd_(-1),
                             zoom_init(false),
                             zoom_abs_location(-1),
                             focus_init(false),
                             focus_abs_location(-1),
                             iris_init(false),
                             iris_abs_location(-1)

{
    if (true == doInit) {
        init();
        enablePowerLines();

        // Home our zoom and then move to start position
        if (false == (zoom_init = zoomHome())) {
            bl_log_error("Unable to home zoom");
        } else if (false == zoomAbsolute(Configuration::zoom_start())) {
            bl_log_error("Unable to zoom to start position: " << Configuration::zoom_start());
        }

        // Home our focus and then move to start position
        if (false == (focus_init = focusHome())) {
            bl_log_error("Unable to home focus");
        } else if (false == focusAbsolute(Configuration::focus_start())) {
            bl_log_error("Unable to focus to start position: " << Configuration::focus_start());
        }
    } else {
        zoom_init = true;
        zoom_abs_location = 0;
        focus_init = true;
        focus_abs_location = 0;
        iris_init = true;
        iris_abs_location = 0;
    }
}

MotorDriver::~MotorDriver()
{}

bool MotorDriver::init()
{
    bl_log_info("Initializing i2c device");
    std::string devicename("/dev/i2c-6");
    if ((fd_ = open(devicename.c_str(), O_RDWR)) < 0) {
        bl_log_error("Unable to open device: " << devicename);
        return false;
    }
    uint64_t funcs;
    if (ioctl(fd_, I2C_FUNCS, &funcs) < 0) {
        bl_log_error("Unable to get I2C_FUNCS");
        return false;
    }
    if (funcs & I2C_FUNC_I2C) {
        bl_log_info("Supports I2C_RDWR");
    } else if (funcs & I2C_FUNC_SMBUS_WORD_DATA) {
        bl_log_info("Does not support I2C_RDWR");
    } else {
        bl_log_error("Unable to get valid FUNC");
        return false;
    }
    return initExpanders();
}

bool MotorDriver::writeReg(uint8_t addr, uint8_t regaddr, uint8_t data)
{
    //bl_log_info(__FUNCTION__ << ": 0x" << hex << std::setw(2) << std::setfill('0') << (unsigned)addr << " reg 0x" << std::setw(2) << std::setfill('0') << (unsigned)regaddr << " val 0x" << std::setw(2) << std::setfill('0') << (unsigned)data << dec); 
    if (ioctl(fd_, I2C_SLAVE, addr) < 0) {
        bl_log_error("Unable to access slave: " << addr);
        return false;
    }
    char buf[10];
    buf[0] = regaddr;
    buf[1] = data;
    if (write(fd_, buf, 2) != 2) {
        bl_log_error("Unable to write data");
        return false;
    }
    return true;
}

bool MotorDriver::readReg(uint8_t addr, uint8_t regaddr, uint8_t& res)
{
    if (ioctl(fd_, I2C_SLAVE, addr) < 0) {
        bl_log_error("Unable to access slave: " << addr);
        return false;
    }
    res = i2c_smbus_read_byte_data(fd_, regaddr);
    return true;
}

bool MotorDriver::printReg(uint8_t addr, uint8_t regaddr)
{
    uint8_t res = 0x00;
    bool ret = readReg(addr, regaddr, res);
    bl_log_info("0x" << std::hex << res << std::dec);
    return ret;
}

bool MotorDriver::printZoom()
{
    return printReg(addr1, REG_GPIOA);
}

bool MotorDriver::printFocus()
{
    return printReg(addr1, REG_GPIOB);
}

bool MotorDriver::printIris()
{
    return printReg(addr2, REG_GPIOA);
}

bool MotorDriver::printPower()
{
    return printReg(addr2, REG_GPIOB);
}

bool MotorDriver::initExpanders()
{
    bl_log_info("IN initExpanders");
    char buf[10] = {0};
    if (-1 == fd_) {
        bl_log_error("i2c device not initialized");
        return false;
    }

    DO_WRITE(addr1, REG_IOCON, REG_IOCON_VAL);
    DO_WRITE(addr1, REG_IODIRA, REG_IODIRA_EXP1);
    DO_WRITE(addr1, REG_IODIRB, REG_IODIRB_EXP1);
    DO_WRITE(addr1, REG_GPPUA, REG_GPPUA_VAL);
    DO_WRITE(addr1, REG_GPPUB, REG_GPPUB_VAL);
    DO_WRITE(addr1, REG_GPIOA, REG_GPIOA_EXP1);
    exp1_gpioa = REG_GPIOA_EXP1;
    DO_WRITE(addr1, REG_GPIOB, REG_GPIOB_EXP1);
    exp1_gpiob = REG_GPIOB_EXP1;

    DO_WRITE(addr2, REG_IODIRA, REG_IODIRA_EXP2);
    DO_WRITE(addr2, REG_IODIRB, REG_IODIRB_EXP2);
    DO_WRITE(addr2, REG_GPPUA, REG_GPPUA_VAL);
    DO_WRITE(addr2, REG_GPPUB, REG_GPPUB_VAL);
    DO_WRITE(addr2, REG_GPIOA, REG_GPIOA_EXP2);
    exp2_gpioa = REG_GPIOA_EXP2;
    DO_WRITE(addr2, REG_GPIOB, REG_GPIOB_EXP2);
    exp2_gpiob = REG_GPIOB_EXP2;
    if (false == enablePowerLines()) {
        bl_log_error("Unable to enable power lines");
        return false;
    }
    return true;
}

bool MotorDriver::enablePowerLines()
{
    // enable 4V and 1.8V power lines
    exp2_gpiob |= (POWER_4V_EN | POWER_1_8V_EN);
    DO_WRITE(addr2, REG_GPIOB, exp2_gpiob);
    return true;
}

bool MotorDriver::disableZoom()
{
    exp1_gpioa |= ZOOM_ENABLE;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    return true;
}

bool MotorDriver::enableZoom(MotorDirection dir) {
    if (((0 == Configuration::zoom_up_direction()) && (MotorDirection::UP == dir)) ||
        ((0 != Configuration::zoom_up_direction()) && (MotorDirection::DOWN == dir))) {
        exp1_gpioa &= ~ZOOM_DIR;
    } else {
        exp1_gpioa |= ZOOM_DIR;
    }
    exp1_gpioa &= ~ZOOM_ENABLE;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
}

bool MotorDriver::zoomDir(int dir, int steps)
{
    if (0 == dir) {
        exp1_gpioa &= ~ZOOM_DIR;
        exp1_gpioa &= ~ZOOM_ENABLE;
    } else {
        exp1_gpioa |= ZOOM_DIR;
        exp1_gpioa &= ~ZOOM_ENABLE;
    }
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    zoom(steps);
    disableZoom();
}

bool MotorDriver::zoomUp(int steps)
{
    enableZoom(MotorDirection::UP);
    zoom(steps);
    zoom_abs_location += steps;
    disableZoom();
    return true;
}

bool MotorDriver::zoomDown(int steps)
{
    enableZoom(MotorDirection::DOWN);
    zoom(steps);
    zoom_abs_location -= steps;
    disableZoom();
    return true;
}

bool MotorDriver::zoomAbsolute(int loc)
{
    bool ret = false;
    if (false == zoom_init) {
        bl_log_error("Unable to zoom to absolute position, zoom not homed");
        return false;
    } else if (loc < 0) {
        bl_log_error("Invalid absolute zoom location: " << loc);
        return false;
    } else if (loc == zoom_abs_location) {
        return true;
    } else if (loc < zoom_abs_location) {
        ret = zoomDown(zoom_abs_location - loc);
    } else {
        ret = zoomUp(loc - zoom_abs_location);
    }

    if (false == ret) {
        zoom_init = false;
        zoom_abs_location = -1;
    }
    return ret;
}

bool MotorDriver::zoomRelative(int steps)
{
    bool ret = false;
    if (false == zoom_init) {
        bl_log_error("Unable to zoom to relative position, zoom not homed");
        return false;
    } else if (steps == 0) {
        return true;
    } else if (steps < 0) {
        ret = zoomDown(abs(steps));
    } else {
        ret = zoomUp(steps);
    }
    
    if (false == ret) {
        zoom_init = false;
        zoom_abs_location = -1;
    }
    return ret;
}

bool MotorDriver::zoomHome()
{
    bool ret = false;
    const int steps = Configuration::zoom_home_max_steps();
    const int stepsize = Configuration::zoom_home_step_size();
    if (0 == Configuration::zoom_home_direction()) {
        exp1_gpioa &= ~ZOOM_DIR;
    } else {
        exp1_gpioa |= ZOOM_DIR;
    }
    exp1_gpioa &= ~ZOOM_ENABLE;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    int counter = 0;
    while ((!Configuration::zoom_has_limit() || !zoomLimit()) && (counter < steps)) {
        zoom(stepsize);
	    counter += stepsize;
    }
    if (counter <= steps) {
        bl_log_error("Reached zoom homing step limit of " << steps);
        zoom_abs_location = -1;
        zoom_init = false;
    } else {
        bl_log_info("Found zoom limit");
        zoom_abs_location = 0;
        ret = true;
    }
    disableZoom();
    return ret;
}

bool MotorDriver::zoomLimit()
{
    uint8_t res = 0x00;
    bool ret = readReg(addr1, REG_GPIOA, res);
    return (res & ZOOM_LIMIT);
}

bool MotorDriver::zoom(int steps)
{
    for(int i = 0; i < steps; ++i) {
        char before = exp1_gpioa;
        exp1_gpioa |= ZOOM_STEP;
        DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
        exp1_gpioa = before;
        DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
    }
}

bool MotorDriver::enableFocus(MotorDirection dir) {
    if (((0 == Configuration::focus_up_direction()) && (MotorDirection::UP == dir)) ||
        ((0 != Configuration::focus_up_direction()) && (MotorDirection::DOWN == dir))) {
        exp1_gpiob &= ~FOCUS_DIR;
    } else {
        exp1_gpiob |= FOCUS_DIR;
    }
    exp1_gpiob &= ~FOCUS_ENABLE;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
}

bool MotorDriver::disableFocus()
{
    exp1_gpiob |= FOCUS_ENABLE;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    return true;
}

bool MotorDriver::focusDir(int dir, int steps)
{
    if (0 == dir) {
        exp1_gpiob &= ~FOCUS_DIR;
        exp1_gpiob &= ~FOCUS_ENABLE;
    } else {
        exp1_gpiob |= FOCUS_DIR;
        exp1_gpiob &= ~FOCUS_ENABLE;
    }
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    focus(steps);
    disableFocus();
}

bool MotorDriver::focusUp(int steps)
{
    enableFocus(MotorDirection::UP);
    focus(steps);
    focus_abs_location += steps;
    disableFocus();
    return true;
}

bool MotorDriver::focusDown(int steps)
{
    enableFocus(MotorDirection::DOWN);
    focus(steps);
    focus_abs_location -= steps;
    disableFocus();
    return true;
}

bool MotorDriver::focusAbsolute(int loc)
{
    bool ret = false;
    if (false == focus_init) {
        bl_log_error("Unable to focus to absolute position, focus not homed");
        return false;
    } else if (loc < 0) {
        bl_log_error("Invalid absolute focus location: " << loc);
        return false;
    } else if (loc == focus_abs_location) {
        return true;
    } else if (loc < focus_abs_location) {
        ret = focusDown(focus_abs_location - loc);
    } else {
        ret = focusUp(loc - focus_abs_location);
    }

    if (false == ret) {
        focus_init = false;
        focus_abs_location = -1;
    }
    return ret;
}

bool MotorDriver::focusRelative(int steps)
{
    bool ret = false;
    if (false == focus_init) {
        bl_log_error("Unable to focus to relative position, focus not homed");
        return false;
    } else if (steps == 0) {
        return true;
    } else if (steps < 0) {
        ret = focusDown(abs(steps));
    } else {
        ret = focusUp(steps);
    }

    if (false == ret) {
        focus_init = false;
        focus_abs_location = -1;
    }
    return ret;
}

bool MotorDriver::focusHome()
{
    bool ret = false;
    const int steps = Configuration::focus_home_max_steps();
    const int stepsize = Configuration::focus_home_step_size();
    if (0 == Configuration::focus_home_direction()) {
        exp1_gpiob &= ~FOCUS_DIR;
    } else {
        exp1_gpiob |= FOCUS_DIR;
    }
    exp1_gpiob &= ~FOCUS_ENABLE;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    int counter = 0;
    while ((!Configuration::focus_has_limit() || !focusLimit()) && (counter < steps)) {
        focus(stepsize);
	    counter += stepsize;
    }
    if (counter <= steps) {
        bl_log_error("Reached focus homing step limit of " << steps);
        focus_abs_location = -1;
        focus_init = false;
    } else {
        bl_log_info("Found focus limit");
        focus_abs_location = 0;
        ret = true;
    }
    disableFocus();
    return ret;
}

bool MotorDriver::focusLimit()
{
    uint8_t res = 0x00;
    bool ret = readReg(addr1, REG_GPIOB, res);
    return (res & FOCUS_LIMIT);
}

bool MotorDriver::focus(int steps)
{
    for(int i = 0; i < steps; ++i) {
        char before = exp1_gpiob;
        exp1_gpiob |= FOCUS_STEP;
        DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
        exp1_gpiob = before;
        DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
    }
}

bool MotorDriver::enableIris(MotorDirection dir) {
    if (((0 == Configuration::iris_up_direction()) && (MotorDirection::UP == dir)) ||
        ((0 != Configuration::iris_up_direction()) && (MotorDirection::DOWN == dir))) {
        exp2_gpioa &= ~IRIS_DIR;
    } else {
        exp2_gpioa |= IRIS_DIR;
    }
    exp2_gpioa &= ~IRIS_ENABLE;
    DO_WRITE(addr1, REG_GPIOA, exp2_gpioa);
}

bool MotorDriver::disableIris()
{
    exp2_gpioa |= IRIS_ENABLE;
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    return true;
}

bool MotorDriver::irisDir(int dir, int steps)
{
    if (0 == dir) {
        exp2_gpioa &= ~IRIS_DIR;
        exp2_gpioa &= ~IRIS_ENABLE;
    } else {
        exp2_gpioa |= IRIS_DIR;
        exp2_gpioa &= ~IRIS_ENABLE;
    }
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    iris(steps);
    disableIris();
}

bool MotorDriver::irisUp(int steps)
{
    enableIris(MotorDirection::UP);
    iris(steps);
    iris_abs_location += steps;
    disableIris();
    return true;
}

bool MotorDriver::irisDown(int steps)
{
    enableIris(MotorDirection::DOWN);
    iris(steps);
    iris_abs_location -= steps;
    disableIris();
    return true;
}

bool MotorDriver::iris(int steps)
{
    for(int i = 0; i < steps; ++i) {
        char before = exp2_gpioa;
        exp2_gpioa |= IRIS_STEP;
        DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
        exp1_gpioa = before;
        DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
    }
}

bool MotorDriver::irisAbsolute(int loc)
{
    bool ret = false;
    if (false == iris_init) {
        bl_log_error("Unable to move iris to absolute position, iris not homed");
        return false;
    } else if (loc < 0) {
        bl_log_error("Invalid absolute iris location: " << loc);
        return false;
    } else if (loc == iris_abs_location) {
        return true;
    } else if (loc < iris_abs_location) {
        ret = irisDown(iris_abs_location - loc);
    } else {
        ret = irisUp(loc - iris_abs_location);
    }

    if (false == ret) {
        iris_init = false;
        iris_abs_location = -1;
    }
    return ret;
}

bool MotorDriver::irisRelative(int steps)
{
    bool ret = false;
    if (false == iris_init) {
        bl_log_error("Unable to move iris to relative position, iris not homed");
        return false;
    } else if (steps == 0) {
        return true;
    } else if (steps < 0) {
        ret = irisDown(abs(steps));
    } else {
        ret = irisUp(steps);
    }

    if (false == ret) {
        iris_init = false;
        iris_abs_location = -1;
    }
    return ret;
}

bool MotorDriver::irisHome()
{
    bool ret = false;
    const int steps = Configuration::iris_home_max_steps();
    const int stepsize = Configuration::iris_home_step_size();
    if (0 == Configuration::iris_home_direction()) {
        exp2_gpioa &= ~IRIS_DIR;
    } else {
        exp2_gpioa |= IRIS_DIR;
    }
    exp2_gpioa &= ~IRIS_ENABLE;
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    int counter = 0;
    while (counter < steps) {
        focus(stepsize);
	    counter += stepsize;
    }
    bl_log_info("At iris limit");
    iris_abs_location = 0;
    ret = true;
    disableIris();
    return ret;
}
