#ifndef __PID_H__
#define __PID_H__

class PID
{
private:
  float _kP;
  float _kI;
  float _kD;
  float _dt;

  float _outputMin;
  float _outputMax;

  float _targetValue;

  float _prevError;
  float _prevProp;
  float _output;
  float _integral; // 積分値（アンチワインドアップ用）

public:
  PID(float kP, float kI, float kD, float dt, float outputMin, float outputMax, float targetValue);

  float kP() const;
  void kP(float k);

  float kI() const;
  void kI(float k);

  float kD() const;
  void kD(float k);

  float outputMin() const;
  void outputMin(float max);

  float outputMax() const;
  void outputMax(float max);

  void targetValue(float target);
  float targetValue() const;

  float output() const;

  void update(float currentValue);
  void reset();
};

#endif // __PID_H__
