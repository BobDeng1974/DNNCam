// System includes
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <chrono>
#include <thread>
#include <iostream>

#include "motordriver.hpp"

using namespace std;

// Addresses of the chips on the I2C bus
int MotorDriver::addr1 = 0x20;
int MotorDriver::addr2 = 0x21;

#define STEPPER_SLEEP_TIME_MS 1

#define ZOOM_DIR        0x02
#define ZOOM_ENABLE     0x04
#define ZOOM_STEP       0x08
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
#define REG_GPIOA_EXP1 0x90
#define REG_GPIOB_EXP1 0x90
#define REG_GPIOA_EXP2 0x90
#define REG_GPIOB_EXP2 0x00

#define DO_WRITE(addr,reg,data)   \
    {cout << "Write Addr: " << hex << addr << " reg " << hex << reg << " val " << hex << data << endl; if (false == writeReg(addr,reg,data)) { cerr << "Write failed for addr: " << addr << " reg: " << reg << endl; return false;}}

MotorDriver::MotorDriver() : fd_(-1)
{}

MotorDriver::~MotorDriver()
{}

bool MotorDriver::init()
{
    cout << "Initializing i2c device" << endl;
    std::string devicename("/dev/i2c-6");
    if ((fd_ = open(devicename.c_str(), O_RDWR)) < 0) {
        cerr << "Unable to open device: " << devicename << endl;
        return false;
    }
    uint64_t funcs;
    if (ioctl(fd_, I2C_FUNCS, &funcs) < 0) {
        cout << "Unable to get I2C_FUNCS" << endl;
        return false;
    }
    if (funcs & I2C_FUNC_I2C) {
        cout << "Supports I2C_RDWR" << endl;
    } else if (funcs & I2C_FUNC_SMBUS_WORD_DATA) {
        cout << "Does not support I2C_RDWR" << endl;
    } else {
        cerr << "Unable to get valid FUNC" << endl;
        return false;
    }
    return initExpanders();
}

bool MotorDriver::writeReg(uint8_t addr, uint8_t regaddr, char data)
{
    if (ioctl(fd_, I2C_SLAVE, addr) < 0) {
        cout << "Unable to access slave: " << addr << endl;
        return false;
    }
    char buf[10];
    buf[0] = regaddr;
    buf[1] = data;
    if (write(fd_, buf, 2) != 2) {
        cerr << "Unable to write data" << endl;
        return false;
    }
    return true;
}

bool MotorDriver::initExpanders()
{
    cout << "IN initExpanders" << endl;
    printf("TEST\n");
    char buf[10] = {0};
    if (-1 == fd_) {
        cerr << "i2c device not initialized" << endl;
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
        cerr << "Unable to enable power lines" << endl;
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

bool MotorDriver::enableZoom()
{
    exp1_gpioa |= ZOOM_ENABLE;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    return true;
}

bool MotorDriver::disableZoom()
{
    exp1_gpioa &= ~ZOOM_ENABLE;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    return true;
}

bool MotorDriver::zoomIn(int steps)
{
    // assuming pin high is zoom in
    exp1_gpioa |= ZOOM_DIR;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    zoom(steps);
}

bool MotorDriver::zoomOut(int steps)
{
    // assuming pin low is zoom out
    exp1_gpioa &= ~ZOOM_DIR;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    zoom(steps);
}

bool MotorDriver::zoom(int steps)
{
    for(int i = 0; i < steps; ++i) {
        cout << "Zoom one step" << endl;
        char before = exp1_gpioa;
        exp1_gpioa |= ZOOM_STEP;
        DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
        exp1_gpioa = before;
        DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
    }
}

bool MotorDriver::enableFocus()
{
    exp1_gpiob |= FOCUS_ENABLE;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    return true;
}

bool MotorDriver::disableFocus()
{
    exp1_gpiob &= ~FOCUS_ENABLE;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    return true;
}

bool MotorDriver::focusIn(int steps)
{
    // assuming pin high is focus in
    exp1_gpiob |= FOCUS_DIR;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    focus(steps);
}

bool MotorDriver::focusOut(int steps)
{
    // assuming pin low is focus out
    exp1_gpiob &= ~FOCUS_DIR;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    focus(steps);
}

bool MotorDriver::focus(int steps)
{
    for(int i = 0; i < steps; ++i) {
        cout << "Focus one step" << endl;
        char before = exp1_gpiob;
        exp1_gpiob |= FOCUS_STEP;
        DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
        exp1_gpiob = before;
        DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
    }
}

bool MotorDriver::enableIris()
{
    exp2_gpioa |= IRIS_ENABLE;
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    return true;
}

bool MotorDriver::disableIris()
{
    exp2_gpioa &= ~IRIS_ENABLE;
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    return true;
}

bool MotorDriver::irisIn(int steps)
{
    // assuming pin high is iris in
    exp2_gpioa |= IRIS_DIR;
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    iris(steps);
}

bool MotorDriver::irisOut(int steps)
{
    // assuming pin low is zoom out
    exp2_gpioa &= ~IRIS_DIR;
    DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
    iris(steps);
}

bool MotorDriver::iris(int steps)
{
    for(int i = 0; i < steps; ++i) {
        cout << "Iris one step" << endl;
        char before = exp2_gpioa;
        exp2_gpioa |= IRIS_STEP;
        DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
        exp1_gpioa = before;
        DO_WRITE(addr2, REG_GPIOA, exp2_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
    }
}
