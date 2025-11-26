#pragma once

#include <iostream>
#include <string>

// Boost MSM headers
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace astra::fsm {

namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msmf = boost::msm::front;

// Events
struct EventStart {};
struct EventStop {};
struct EventPause {};
struct EventResume {};

// State Machine Definition
struct LifecycleStateMachine_ : public msmf::state_machine_def<LifecycleStateMachine_> {
    
    // States
    struct Initial : public msmf::state<> {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { std::cout << "Entering: Initial" << std::endl; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { std::cout << "Exiting: Initial" << std::endl; }
    };

    struct Running : public msmf::state<> {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { std::cout << "Entering: Running" << std::endl; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { std::cout << "Exiting: Running" << std::endl; }
    };

    struct Paused : public msmf::state<> {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { std::cout << "Entering: Paused" << std::endl; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { std::cout << "Exiting: Paused" << std::endl; }
    };

    struct Stopped : public msmf::state<> {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { std::cout << "Entering: Stopped" << std::endl; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { std::cout << "Exiting: Stopped" << std::endl; }
    };

    // Initial State
    using initial_state = Initial;

    // Actions
    struct LogTransition {
        template <class EVT, class FSM, class Source, class Target>
        void operator()(EVT const&, FSM&, Source&, Target&) {
            std::cout << "Transitioning..." << std::endl;
        }
    };

    // Transition Table
    struct transition_table : mpl::vector<
        //    Start     Event         Target      Action         Guard
        //   +---------+-------------+-----------+--------------+-------+
        msmf::Row < Initial,  EventStart,   Running,    LogTransition, msmf::none >,
        msmf::Row < Running,  EventStop,    Stopped,    LogTransition, msmf::none >,
        msmf::Row < Running,  EventPause,   Paused,     LogTransition, msmf::none >,
        msmf::Row < Paused,   EventResume,  Running,    LogTransition, msmf::none >,
        msmf::Row < Paused,   EventStop,    Stopped,    LogTransition, msmf::none >,
        msmf::Row < Stopped,  EventStart,   Running,    LogTransition, msmf::none >
        //   +---------+-------------+-----------+--------------+-------+
    > {};

    // Replaces the default no-transition handler
    template <class FSM, class Event>
    void no_transition(Event const& e, FSM&, int state) {
        std::cout << "No transition from state " << state << " on event" << std::endl;
    }
};

// Back-end
using LifecycleStateMachine = msm::back::state_machine<LifecycleStateMachine_>;

} // namespace astra::fsm
