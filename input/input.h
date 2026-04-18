#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <CircularBuffer.h>

enum ButtonName
{
  UP_BUTTON,
  DOWN_BUTTON,
  LEFT_BUTTON,
  RIGHT_BUTTON,
  MIDDLE_BUTTON,
  EXT_BUTTON
};

class Input
{
private:
    CircularBuffer<ButtonName, 10> buttonQueue;
    int upButtonLastClickedTime = 0;
    int downButtonLastClickedTime = 0;
    int leftButtonLastClickedTime = 0;
    int rightButtonLastClickedTime = 0;
    int middleButtonLastClickedTime = 0;
    int extButtonLastClickedTime = 0;
    bool upButtonLastState = 0;
    bool downButtonLastState = 0;
    bool leftButtonLastState = 0;
    bool rightButtonLastState = 0;
    bool middleButtonLastState = 0;
    bool extButtonLastState = 0;

public:
    Input();
    ~Input();
    void loop();
};



#endif