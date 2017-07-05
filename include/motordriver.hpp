#include <stdint.h>

class MotorDriver {
public:
    MotorDriver();
    ~MotorDriver();

    bool init();
    bool initExpanders();
    bool writeReg(uint8_t addr, uint8_t regaddr, char data);
    bool enablePowerLines();

    bool enableZoom();
    bool disableZoom();
    bool zoomIn(int steps);
    bool zoomOut(int steps);

    bool enableFocus();
    bool disableFocus();
    bool focusIn(int steps);
    bool focusOut(int steps);

    bool enableIris();
    bool disableIris();
    bool irisIn(int steps);
    bool irisOut(int steps);
private:
    bool zoom(int steps);
    bool focus(int steps);
    bool iris(int steps);

    int fd_;
    static int addr1, addr2;
    char exp1_gpioa, exp1_gpiob, exp2_gpioa, exp2_gpiob;
};
