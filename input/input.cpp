#include "input.h"

Input::Input()
{
}

Input::~Input()
{
}

void Input::loop() {
    int upAndDown = analogRead(35);
    if (upAndDown > 2048)
    {
        if (upButtonLastState == 0)
        {
            upButtonLastState = 1;
            Debug.Debug("Up Click");
            buttonQueue.push(ButtonName::UP_BUTTON);
        }
    }
    else if (upAndDown > 32 && upAndDown < 2048)
    {
        if (downButtonLastState == 0)
        {
            downButtonLastState = 1;
            Debug.Debug("Down Click");
            buttonQueue.push(ButtonName::DOWN_BUTTON);
        }
    }
    else
    {
        upButtonLastState = 0;
        downButtonLastState = 0;
    }
    int leftAndRight = analogRead(32);
    if (leftAndRight > 2048)
    {
        if (leftButtonLastState == 0)
        {
            leftButtonLastState = 1;
            Debug.Debug("Left Click");
            buttonQueue.push(ButtonName::LEFT_BUTTON);
        }
    }
    else if (leftAndRight > 32 && leftAndRight < 2048)
    {
        if (rightButtonLastState == 0)
        {
            rightButtonLastState = 1;
            Debug.Debug("Right Click");
            buttonQueue.push(ButtonName::RIGHT_BUTTON);
        }
    }
    else
    {
        leftButtonLastState = 0;
        rightButtonLastState = 0;
    }

    int midAndExt = analogRead(34);
    if (midAndExt > 2048)
    {
        if (middleButtonLastState == 0)
        {
            middleButtonLastState = 1;
            Debug.Debug("Middle Click");
            buttonQueue.push(ButtonName::MIDDLE_BUTTON);
        }
    }
    else if (midAndExt > 32 && midAndExt < 2048)
    {
        if (extButtonLastState == 0)
        {
            extButtonLastState = 1;
            Debug.Debug("Ext Click");
            buttonQueue.push(ButtonName::EXT_BUTTON);
        }
    }
    else
    {
        middleButtonLastState = 0;
        extButtonLastState = 0;
    }
}
