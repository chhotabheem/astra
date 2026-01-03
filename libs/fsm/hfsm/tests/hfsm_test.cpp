#include "Machine.h"

#include <cassert>
#include <iostream>

// Define state machine structure
struct Context {};

using M = hfsm2::MachineT<hfsm2::Config::ContextT<Context>>;

struct On;
struct Off;

using FSM = M::PeerRoot<On, Off>;

struct On : FSM::State {
  void enter(PlanControl &control) {
    std::cout << "Entering On" << std::endl;
  }
  void exit(PlanControl &control) {
    std::cout << "Exiting On" << std::endl;
  }
};

struct Off : FSM::State {
  void enter(PlanControl &control) {
    std::cout << "Entering Off" << std::endl;
  }
  void exit(PlanControl &control) {
    std::cout << "Exiting Off" << std::endl;
  }
};

int main() {
  Context context;
  FSM::Instance machine(context);

  assert(machine.isActive<On>());

  machine.changeTo<Off>();
  machine.update();
  assert(machine.isActive<Off>());

  machine.changeTo<On>();
  machine.update();
  assert(machine.isActive<On>());

  std::cout << "HFSM unit test passed" << std::endl;
  return 0;
}
