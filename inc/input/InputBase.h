#pragma once

namespace Mana {

class InputBase;
extern InputBase* g_pInputEngine;

class InputBase {
 public:
  InputBase();
  virtual ~InputBase();

  virtual bool Init() = 0;
  virtual void Uninit() = 0;


};

}
