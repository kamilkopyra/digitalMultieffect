#pragma once
#include "Effect.h"


class ModulationEffect : public Effect {
public:


    bool isModulation() override { return true; }
    virtual void TapToParam(float ms) = 0;
};

