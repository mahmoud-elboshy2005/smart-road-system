#ifndef _FSM_H_
#define _FSM_H_

#include <Arduino.h>
#include <stdbool.h>

// Define maximum number of transitions
#define MAX_TRANSITIONS 32
#define MAX_EVENTS 16

// State enum - you can extend this with your specific states
typedef enum {
    STATE_IDLE,
    STATE_PROCESSING,
    STATE_WAITING,
    STATE_ERROR,
    STATE_COMPLETE,
    // Add more states as needed
} State;

// Event enum - you can extend this with your specific events
typedef enum {
    EVENT_NONE,
    EVENT_START,
    EVENT_STOP,
    EVENT_TIMEOUT,
    EVENT_SUCCESS,
    EVENT_FAILURE,
    // Add more events as needed
} Event;

// Action function pointer type
typedef void (*ActionFunction)(void);

// Transition structure
typedef struct {
    State currentState;
    State nextState;
    Event transitionEvent;
    ActionFunction action;  // Optional action to execute during transition
} Transition;

// Global current state variable
extern State currentState;

// Function declarations
void fsm_init(State initialState);
bool fsm_register_transition(State currentState, State nextState, Event event, ActionFunction action);
void fsm_push_event(Event event);
void fsm_process_events(void);
bool fsm_dispatch_event(Event event);
State fsm_get_current_state(void);
void fsm_set_current_state(State state);
void fsm_reset(State initialState);

#endif // _FSM_H_
