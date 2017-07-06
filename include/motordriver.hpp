#include <stdint.h>

class MotorDriver {
public:
    MotorDriver();
    ~MotorDriver();

    bool init();
    bool initExpanders();
    bool enablePowerLines();

    bool zoomIn(int steps);
    bool zoomOut(int steps);
    bool zoomDir(int dir, int steps);
    bool zoomHome(int dir, int steps);

    bool focusIn(int steps);
    bool focusOut(int steps);
    bool focusDir(int dir, int steps);
    bool focusHome(int dir, int steps);

    bool irisIn(int steps);
    bool irisOut(int steps);
    bool irisDir(int dir, int steps);
    bool printZoom();
    bool printFocus();
    bool printIris();
    bool printPower();

private:
    bool writeReg(uint8_t addr, uint8_t regaddr, uint8_t data);
    bool readReg(uint8_t addr, uint8_t regaddr, uint8_t& res);
    bool printReg(uint8_t addr, uint8_t regaddr);

    bool enableZoom();
    bool disableZoom();
    bool zoom(int steps);
    bool zoomLimit();

    bool enableFocus();
    bool disableFocus();
    bool focus(int steps);
    bool focusLimit();

    bool enableIris();
    bool disableIris();
    bool iris(int steps);

    int fd_;
    static int addr1, addr2;
    char exp1_gpioa, exp1_gpiob, exp2_gpioa, exp2_gpiob;
};
