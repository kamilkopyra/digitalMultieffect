#pragma once
#include "effect.h"


class ModulationEffect : public Effect {

public:
    virtual int msToParam(float ms) = 0;
};
