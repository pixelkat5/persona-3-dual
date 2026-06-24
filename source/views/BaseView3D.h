#pragma once
#include "core/globals.h"
#include "views/BaseView.h"
#include <nds.h>

class BaseView3D : public BaseView
{
  public:
    virtual void init() override;
    virtual void setupEnvironment() = 0;
    // update needs to be overridden
    // cleanup needs to be overridden
};
