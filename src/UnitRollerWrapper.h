#ifndef UNITROLLERWRAPPER_H_
#define UNITROLLERWRAPPER_H_

#include <unit_rolleri2c.hpp>

class UnitRollerWrapper
{
public:
    bool begin(TwoWire *wire = &Wire, uint8_t addr = I2C_ADDR, uint8_t sda = 21, uint8_t scl = 22,
               uint32_t speed = 4000000L);
    void setMode(roller_mode_t mode);
    void setOutput(uint8_t en);

    // 読み取り値の取得
    float getVinV();
    float getCurrentA();
    float getSpeedRpm();
    float getPosRev();

    // 電流制御モード
    void setTargetCurrentA(float currentA);
    float getTargetCurrentA();

    // 速度制御モード
    void setTargetSpeedRpm(float speedRpm);
    float getTargetSpeedRpm();
    void setSpeedPID(float kp, float ki, float kd);
    void getSpeedPID(float &kp, float &ki, float &kd);
    float getSpeedMaxCurrentA();
    void setSpeedMaxCurrentA(float speedMaxCurrentA);

    // 位置制御モード
    void setTargetPosRev(float posRev);
    float getTargetPosRev();
    void setPosPID(float kp, float ki, float kd);
    void getPosPID(float &kp, float &ki, float &kd);
    float getPosMaxCurrentA();
    void setPosMaxCurrentA(float posMaxCurrentA);

private:
    UnitRollerI2C _roller;
    bool _initialized = false;
};


#endif // UNITROLLERWRAPPER_H_
