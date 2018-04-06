// System includes
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>

#include "motordriver.hpp"
#include "configuration.hpp"

using namespace std;

namespace BoulderAI
{

// Addresses of the chips on the I2C bus
int MotorDriver::addr1 = 0x20;  //U4 (Zoom, focus)
int MotorDriver::addr2 = 0x21;  //U5 (Power on mainboard)
int MotorDriver::addr0 = 0x22;  //U7 (Iris/Ircut)

#define STEPPER_SLEEP_TIME_MS 1

#define ZOOM_LIMIT      0x01 //pin U5.A.0
#define ZOOM_DIR        0x02 //pin U5.A.1
#define ZOOM_ENABLE     0x04 //pin U5.A.2
#define ZOOM_STEP       0x08 //pin U5.A.3
#define ZOOM_FAULT      0x10 //pin U5.A.4
#define ZOOM_MS2        0X20 //pin U5.A.5
#define ZOOM_MS1	0x40 //pin U5.A.6
#define ZOOM_SLEEP	0x80 //pin U5.A.7
#define ZOOM_AENBL 	ZOOM_ENABLE
#define ZOOM_BENBL 	ZOOM_STEP
#define ZOOM_APHASE 	ZOOM_MS1
#define ZOOM_BPHASE	ZOOM_DIR
#define ZOOM_EN1	ZOOM_MS2

#define FOCUS_LIMIT     0x01 //pin U5.B.0
#define FOCUS_DIR       0x02 //pin U5.B.1
#define FOCUS_ENABLE    0x04 //pin U5.B.2
#define FOCUS_STEP      0x08 //pin U5.B.3
#define FOCUS_FAULT     0x10 //pin U5.B.4
#define FOCUS_MS2       0X20 //pin U5.B.5
#define FOCUS_MS1       0x40 //pin U5.B.6
#define FOCUS_SLEEP     0x80 //pin U5.B.7
#define FOCUS_AENBL      FOCUS_ENABLE
#define FOCUS_BENBL      FOCUS_STEP
#define FOCUS_APHASE     FOCUS_MS1
#define FOCUS_BPHASE     FOCUS_DIR
#define FOCUS_EN1        FOCUS_MS2

#define PG		0x01 //pin U6.A.0
#define IRIS_DIR        0x02 //pin U6.A.1
#define IRIS_ENABLE     0x04 //pin U6.A.2
#define IRIS_STEP       0x08 //pin U6.A.3
#define IRIS_FAULT      0x10 //pin U6.A.4
#define IRIS_MS2        0X20 //pin U6.A.5
#define IRIS_MS1        0x40 //pin U6.A.6
#define IRIS_SLEEP      0x80 //pin U6.A.7
#define IRIS_AENBL      IRIS_ENABLE
#define IRIS_BENBL      IRIS_STEP
#define IRIS_APHASE     IRIS_MS1
#define IRIS_BPHASE     IRIS_DIR
#define IRIS_EN1        IRIS_MS2


#define POWER_5V_EN        0x80 //pin U6.B.7
#define POWER_4V_EN        0x40 //pin U6.B.6
#define POWER_2_8V_EN      0x20 //pion U6.B.5
#define POWER_1_8V_EN      0x10 //pin U6.B.4
#define POWER_1_2V_EN      0x08 //pin U6.B.3
#define PG_4V		   0x04 //pin U6.B.2
#define IRCUT_A		   0x02 //pin U6.B.1
#define IRCUT_B		   0x01 //pin U6.B.0

#define REG_IOCON 0x0A
#define REG_IODIRA 0x00
#define REG_IODIRB 0x01
#define REG_GPPUA 0x0C
#define REG_GPPUB 0x0D
#define REG_GPIOA 0x12
#define REG_GPIOB 0x13

#define REG_IOCON_VAL 0x00
// Output direction is value 0
#define REG_IODIRA_EXP1 0x11 //Fault, Pgood
#define REG_IODIRB_EXP1 0x11 //Fault, Pgood
#define REG_IODIRA_EXP0 0x00  
#define REG_IODIRB_EXP0 0xE0 //Board ID
#define REG_IODIRA_EXP2 0x11 //Fault, Pgood input, all others output
#define REG_IODIRB_EXP2 0x04 // U6.B.2 is input, all others output
#define REG_GPPUA_VAL 0x00 // disable all pull-ups
#define REG_GPPUB_VAL 0x00
#define REG_GPIOA_EXP1 (ZOOM_SLEEP ) // wake up on init
#define REG_GPIOB_EXP1 (FOCUS_SLEEP)
#define REG_GPIOA_EXP2 0x00
#define REG_GPIOB_EXP2 0x00
#define REG_GPIOA_EXP0 (IRIS_SLEEP ) // wake up on init
#define REG_GPIOB_EXP0 0x00


#define DO_WRITE(addr,reg,data)   \
    {if (false == writeReg(addr,reg,data)) { ostringstream oss; oss << "Write failed for addr: " << hex << addr << " reg: " << hex << reg << dec; _log_callback(oss.str()); return false;}}

    MotorDriver::MotorDriver(bool doInit, boost::function < void(std::string) > log_callback)
        :
        fd_(-1),
        zoom_init(false),
        zoom_abs_location(-1),
        focus_init(false),
        focus_abs_location(-1),
        iris_init(false),
        iris_abs_location(-1),
        _log_callback(log_callback)
{
    if (true == doInit) {
        init();
        enablePowerLines();

        /* TODO: update this when limits switches are working
        // Home our zoom and then move to start position
        if (false == (zoom_init = zoomHome())) {
            log_callback("Unable to home zoom");
        } else if (false == zoomAbsolute(Configuration::zoom_start())) {
            log_callback("Unable to zoom to start position: " << Configuration::zoom_start());
        }

        // Home our focus and then move to start position
        if (false == (focus_init = focusHome())) {
            log_callback("Unable to home focus");
        } else if (false == focusAbsolute(Configuration::focus_start())) {
            log_callback("Unable to focus to start position: " << Configuration::focus_start());
        }
        */
        //irisHome();
    }
}

MotorDriver::~MotorDriver()
{}

bool MotorDriver::init()
{
    _log_callback("Initializing i2c device");
    std::string devicename("/dev/i2c-6");
    if ((fd_ = open(devicename.c_str(), O_RDWR)) < 0) {
        ostringstream oss;
        oss << "Unable to open device: " << devicename;
        _log_callback(oss.str());
        return false;
    }
    uint64_t funcs;
    if (ioctl(fd_, I2C_FUNCS, &funcs) < 0) {
        _log_callback("Unable to get I2C_FUNCS");
        return false;
    }
    if (funcs & I2C_FUNC_I2C) {
        _log_callback("Supports I2C_RDWR");
    } else if (funcs & I2C_FUNC_SMBUS_WORD_DATA) {
        _log_callback("Does not support I2C_RDWR");
    } else {
        _log_callback("Unable to get valid FUNC");
        return false;
    }
    return initExpanders();
}

bool MotorDriver::writeReg(uint8_t addr, uint8_t regaddr, uint8_t data)
{
    ostringstream oss;
    oss << ": 0x" << hex << std::setw(2) << std::setfill('0') << (unsigned)addr << " reg 0x" << std::setw(2) << std::setfill('0') << (unsigned)regaddr << " val 0x" << std::setw(2) << std::setfill('0') << (unsigned)data << dec;
    _log_callback(oss.str());
    if (ioctl(fd_, I2C_SLAVE, addr) < 0) {
        ostringstream oss;
        oss << "Unable to access slave: " << addr;
        _log_callback(oss.str());
        return false;
    }
    char buf[10];
    buf[0] = regaddr;
    buf[1] = data;
    if (write(fd_, buf, 2) != 2) {
        _log_callback("Unable to write data");
        return false;
    }
    return true;
}

bool MotorDriver::readReg(uint8_t addr, uint8_t regaddr, uint8_t& res)
{
    if (ioctl(fd_, I2C_SLAVE, addr) < 0) {
        ostringstream oss;
        oss << "Unable to access slave: " << addr;
        _log_callback(oss.str());
        return false;
    }
    res = i2c_smbus_read_byte_data(fd_, regaddr);
    return true;
}

bool MotorDriver::printReg(uint8_t addr, uint8_t regaddr)
{
    uint8_t res = 0x00;
    bool ret = readReg(addr, regaddr, res);
    ostringstream oss;
    oss << "0x" << std::hex << res << std::dec;
    _log_callback(oss.str());
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
    _log_callback("IN initExpanders");
    char buf[10] = {0};
    if (-1 == fd_) {
        _log_callback("i2c device not initialized");
        return false;
    }

//    DO_WRITE(addr1, REG_IOCON, REG_IOCON_VAL);
    DO_WRITE(addr0, REG_IODIRA, REG_IODIRA_EXP0);
    DO_WRITE(addr0, REG_IODIRB, REG_IODIRB_EXP0);
    DO_WRITE(addr0, REG_GPPUA, REG_GPPUA_VAL);
    DO_WRITE(addr0, REG_GPPUB, REG_GPPUB_VAL);

    DO_WRITE(addr0, REG_GPIOA, REG_GPIOA_EXP0);
    exp0_gpioa = REG_GPIOA_EXP0;

    DO_WRITE(addr0, REG_GPIOB, REG_GPIOB_EXP0);
    exp0_gpiob = REG_GPIOB_EXP0;

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
        _log_callback("Unable to enable power lines");
        return false;
    }
    return true;
}

bool MotorDriver::enablePowerLines()
{
    // enable all power lines
    exp2_gpiob |= (POWER_5V_EN | POWER_4V_EN | POWER_2_8V_EN | POWER_1_8V_EN | POWER_1_2V_EN);
    DO_WRITE(addr2, REG_GPIOB, exp2_gpiob);
    return true;
}

bool MotorDriver::disableZoom()
{
    exp1_gpioa &= ~(ZOOM_AENBL + ZOOM_BENBL);
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
    return true;
}

bool MotorDriver::enableZoom() {
    exp1_gpioa |= ZOOM_AENBL + ZOOM_BENBL;
    DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
}

bool MotorDriver::zoomDir(int dir, int steps)
{
    if (0 == dir) {
	zoom(0-steps);
    } else {
	zoom(steps);
    }
    
   disableZoom();
}

bool MotorDriver::zoomUp(int steps)
{
    enableZoom();
    zoom(steps);
    zoom_abs_location += steps;
    disableZoom();
    return true;
}

bool MotorDriver::zoomDown(int steps)
{
    enableZoom();
    zoom(0-steps);
    zoom_abs_location -= steps;
    disableZoom();
    return true;
}

bool MotorDriver::zoomAbsolute(int loc)
{
    bool ret = false;
    if (false == zoom_init) {
        _log_callback("Unable to zoom to absolute position, zoom not homed");
        return false;
    } else if (loc < 0) {
        ostringstream oss;
        oss << "Invalid absolute zoom location: " << loc;
        _log_callback(oss.str());
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
    if (steps == 0) {
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
    //const int steps = Configuration::zoom_home_max_steps();
    const int steps = 20000;
    const int stepsize = Configuration::zoom_home_step_size();
//    enableZoom();
    int counter = 0;
    ostringstream oss;
    oss << "conf zoom limit: " << Configuration::zoom_has_limit() << " zoom limit pre while " << zoomLimit() << " stepsize " << stepsize;
    _log_callback(oss.str());
    while ((!Configuration::zoom_has_limit() || !zoomLimit()) && (counter < steps)) {
        oss.str("");
        oss << "zoom limit during: " << zoomLimit();
        _log_callback(oss.str());
        zoom(stepsize);
        counter += stepsize;
    }
    oss.str("");
    oss << "zoom limit after: " << zoomLimit();
    _log_callback(oss.str());
    if (counter <= steps) {
        oss.str("");
        oss << "Reached zoom homing step limit of " << steps;
        _log_callback(oss.str());
        zoom_abs_location = -1;
        zoom_init = true;
    } else {
        _log_callback("Found zoom limit");
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
    ostringstream oss;
    oss << "gpioa: 0x" << hex << (int)res << dec;
    _log_callback(oss.str());
    return (res & ZOOM_LIMIT);
}

bool MotorDriver::zoom(int steps)
{
    char mask;
    char phase;
    char out;
    for(int i = 0; i < abs(steps); ++i) {
        phase = 0;
        if (steps >0) {
            phase = 3- (i%4);
        }
        else {
            phase = i%4;
        }
        out = 0;
        switch(phase){
		case 0:
			out = ZOOM_APHASE;
            break;
		case 1:
            out = ZOOM_APHASE+ ZOOM_BPHASE;
            break;
		case 2:
            out = ZOOM_BPHASE;
            break;	
		case 3:
			out=0;
            break;
        }
        mask = exp1_gpioa & (~(ZOOM_APHASE + ZOOM_BPHASE));  //set before to the mask of phase a and b and the current output register
        exp1_gpioa = out + mask; 
        DO_WRITE(addr1, REG_GPIOA, exp1_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));   
    }

    zoomLimit();
}

bool MotorDriver::enableFocus() {

    exp1_gpiob |= FOCUS_AENBL + FOCUS_BENBL;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
}

bool MotorDriver::disableFocus()
{
    exp1_gpiob &= ~(FOCUS_AENBL+FOCUS_BENBL) ;
    DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
    return true;
}

bool MotorDriver::focusDir(int dir, int steps)
{
    if (0 == dir) {
	focus(0-steps);
    } else {
	focus(steps);
    }
    disableFocus();
}

bool MotorDriver::focusUp(int steps)
{
    enableFocus();
    focus(steps);
    focus_abs_location += steps;
    disableFocus();
    return true;
}

bool MotorDriver::focusDown(int steps)
{
    enableFocus();
    focus(0-steps);
    focus_abs_location -= steps;
    disableFocus();
    return true;
}

bool MotorDriver::focusAbsolute(int loc)
{
    bool ret = false;
    if (false == focus_init) {
        _log_callback("Unable to focus to absolute position, focus not homed");
        return false;
    } else if (loc < 0) {
        ostringstream oss;
        oss << "Invalid absolute focus location: " << loc;
        _log_callback(oss.str());
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
    if (steps == 0) {
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
    int stepsize;
    //enableFocus();
    if (0 == Configuration::focus_home_direction()) {
  	stepsize = 0-Configuration::focus_home_step_size();
    }
    else {
        stepsize = Configuration::focus_home_step_size();
    }
    int counter = 0;
    /*while ((!Configuration::focus_has_limit() || !focusLimit()) && (counter < steps)) {
        focus(stepsize);
	    counter += stepsize;
    }*/
    if (counter <= steps) {
        ostringstream oss;
        oss << "Reached focus homing step limit of " << steps;
        _log_callback(oss.str());
        focus_abs_location = -1;
        focus_init = true;
    } else {
        _log_callback("Found focus limit");
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
    ostringstream oss;
    oss << "gpiob: 0x" << hex << (int)res << dec;
    _log_callback(oss.str());
    return (res & FOCUS_LIMIT);
}

bool MotorDriver::focus(int steps)
{
   char mask;
   char out;
   char phase;
   for(int i = 0; i < abs(steps); ++i) {
        phase = 0;
        if (steps <0) {
                phase = 3- (i%4);
        }
        else {
                 phase = i%4;
        }
        out = 0;
        switch(phase){
                case 0:
                        out = FOCUS_APHASE;
                break;
                case 1:
                         out = FOCUS_APHASE+ FOCUS_BPHASE;
                break;
                case 2:
                         out = FOCUS_BPHASE;
                break;
                case 3:
                        out=0;
                break;
        }
   
        mask = exp1_gpiob & (~(FOCUS_APHASE + FOCUS_BPHASE));  //set before to the mask of phase a and b and the current output register
        exp1_gpiob = out + mask;
        DO_WRITE(addr1, REG_GPIOB, exp1_gpiob);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS));
   }

   focusLimit();
}

bool MotorDriver::enableIris() {

    exp0_gpioa |= IRIS_AENBL + IRIS_BENBL;
    DO_WRITE(addr0, REG_GPIOA, exp0_gpioa);
}

bool MotorDriver::disableIris()
{
    exp0_gpioa &= ~(IRIS_AENBL+IRIS_BENBL) ;
    DO_WRITE(addr0, REG_GPIOA, exp0_gpioa);
    return true;
}

bool MotorDriver::irisDir(int dir, int steps)
{
    if (0 == dir) {
        iris(0-steps);
    } else {
        iris(steps);
    }
    disableIris();
}

bool MotorDriver::irisUp(int steps)
{
    enableIris();
    iris(steps);
    iris_abs_location += steps;
    disableIris();
    return true;
}

bool MotorDriver::irisDown(int steps)
{
    enableIris();
    iris(0-steps);
    iris_abs_location -= steps;
    disableIris();
    return true;
}

bool MotorDriver::irisAbsolute(int loc)
{
    bool ret = false;
    if (false == iris_init) {
        _log_callback("Unable to adjust iris to absolute position, iris not homed");
        return false;
    } else if (loc < 0) {
        ostringstream oss;
        oss << "Invalid absolute iris location: " << loc;
        _log_callback(oss.str());
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
    if (steps == 0) {
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
    int stepsize;
    if (0 == Configuration::iris_home_direction()) {
        stepsize = 0-Configuration::iris_home_step_size();
    }
    else {
        stepsize = Configuration::iris_home_step_size();
    }
    int counter = 0;
    while (/*(!Configuration::iris_has_limit() || !irisLimit()) &&*/ (counter < steps))
    {
        iris(stepsize);
        ostringstream oss;
        oss << "finding iris home " << counter << " " << stepsize;
        _log_callback(oss.str());
        counter += stepsize;
    }
    if (counter <= steps) {
        ostringstream oss;
        oss << "Reached iris homing step limit of " << steps;
        _log_callback(oss.str());
        iris_abs_location = -1;
        iris_init = true;
    } else {
        _log_callback("Found iris limit");
        iris_abs_location = 0;
        ret = true;
    }
    disableIris();
    return ret;
}

/*bool MotorDriver::irisLimit()
{
    uint8_t res = 0x00;
    bool ret = readReg(addr2, REG_GPIOA, res);
    return (res & IRIS_LIMIT);
}*/
bool MotorDriver::iris(int steps)
{
   char mask;
   char out;
   char phase;
   for(int i = 0; i < abs(steps); ++i) {
        phase = 0;
        if (steps >0) {
                phase = 3- (i%4);
        }
        else {
                 phase = i%4;
        }
        out = 0;
        switch(phase){
                case 0:
                        out = IRIS_APHASE;
                break;
                case 1:
                         out = IRIS_APHASE+ IRIS_BPHASE;
                break;
                case 2:
                         out = IRIS_BPHASE;
                break;
                case 3:
                        out=0;
                break;
        }

        mask = exp0_gpioa & (~(IRIS_APHASE + IRIS_BPHASE));  //set before to the mask of phase a and b and the current output register
        exp0_gpioa = out + mask;
        DO_WRITE(addr0, REG_GPIOA, exp0_gpioa);
        std::this_thread::sleep_for(std::chrono::milliseconds(STEPPER_SLEEP_TIME_MS)*4);
   }
}
bool MotorDriver::ircutOn()
{
	exp0_gpiob |= IRCUT_A;
	exp0_gpiob &= ~IRCUT_B;
	DO_WRITE(addr0, REG_GPIOB, exp0_gpiob);
	return true;
	
}
bool MotorDriver::ircutOff()
{
        exp0_gpiob |= IRCUT_B;
        exp0_gpiob &= ~IRCUT_A;
        DO_WRITE(addr0, REG_GPIOB, exp0_gpiob);
	return true;

}


}
