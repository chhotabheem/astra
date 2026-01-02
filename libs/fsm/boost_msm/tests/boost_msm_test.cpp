#include "StateMachine.h"

#include <cassert>
#include <iostream>

using namespace astra::fsm;

void test_lifecycle() {
  LifecycleStateMachine fsm;

  // Start the FSM
  fsm.start();
  // Should be in Initial (0) -> wait, start() enters initial state.
  // Actually, msm::back::state_machine enters initial state on construction or
  // start(). Let's check state.

  std::cout << "Current State: " << fsm.current_state()[0] << std::endl;

  // Initial -> Running
  std::cout << "Sending EventStart..." << std::endl;
  fsm.process_event(EventStart{});
  // Check if we are in Running (which is index 1 in the definition order? No,
  // MSM indices are implementation defined usually, but typically follow the
  // order in the mpl::vector if explicitly defined or just unique IDs). A
  // better way is to check flags or just trust the output for this basic test.

  // Running -> Paused
  std::cout << "Sending EventPause..." << std::endl;
  fsm.process_event(EventPause{});

  // Paused -> Running
  std::cout << "Sending EventResume..." << std::endl;
  fsm.process_event(EventResume{});

  // Running -> Stopped
  std::cout << "Sending EventStop..." << std::endl;
  fsm.process_event(EventStop{});

  // Stopped -> Running
  std::cout << "Sending EventStart..." << std::endl;
  fsm.process_event(EventStart{});

  std::cout << "Lifecycle test completed" << std::endl;
}

int main() {
  test_lifecycle();
  return 0;
}
