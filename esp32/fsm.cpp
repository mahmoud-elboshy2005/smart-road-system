#include "fsm.h"

// Global current state variable
State currentState = STATE_IDLE;

// Static transition table
static Transition transitionTable[MAX_TRANSITIONS];
static int transitionCount = 0;

// Static event queue (circular buffer)
static Event eventQueue[MAX_EVENTS];
static int queueHead = 0;
static int queueTail = 0;
static int queueSize = 0;

// Private helper function to find matching transition
static Transition* find_transition(State state, Event event) {
    for (int i = 0; i < transitionCount; i++) {
        if (transitionTable[i].currentState == state && 
            transitionTable[i].transitionEvent == event) {
            return &transitionTable[i];
        }
    }
    return NULL;
}

// Initialize FSM
void fsm_init(State initialState) {
    currentState = initialState;
    transitionCount = 0;
    queueHead = 0;
    queueTail = 0;
    queueSize = 0;
}

// Register a transition in the transition table
bool fsm_register_transition(State fromState, State toState, Event event, ActionFunction action) {
    if (transitionCount >= MAX_TRANSITIONS) {
        Serial.println("Error: Transition table full!");
        return false;
    }
    
    transitionTable[transitionCount].currentState = fromState;
    transitionTable[transitionCount].nextState = toState;
    transitionTable[transitionCount].transitionEvent = event;
    transitionTable[transitionCount].action = action;
    transitionCount++;
    
    return true;
}

// Add event to the queue
void fsm_push_event(Event event) {
    if (queueSize >= MAX_EVENTS) {
        Serial.println("Warning: Event queue full, event discarded!");
        return;
    }
    
    eventQueue[queueTail] = event;
    queueTail = (queueTail + 1) % MAX_EVENTS;
    queueSize++;
}

// Process all events in the queue (main loop function)
void fsm_process_events(void) {
    while (queueSize > 0) {
        Event event = eventQueue[queueHead];
        queueHead = (queueHead + 1) % MAX_EVENTS;
        queueSize--;
        
        if (!fsm_dispatch_event(event)) {
            Serial.print("Warning: No transition found for event ");
            Serial.print(event);
            Serial.print(" in state ");
            Serial.println(currentState);
        }
    }
}

// Event dispatcher - handles a single event and transitions state
bool fsm_dispatch_event(Event event) {
    // Find matching transition
    Transition* transition = find_transition(currentState, event);
    
    if (transition == NULL) {
        // No transition found for this state-event combination
        return false;
    }
    
    Serial.print("FSM Transition: ");
    Serial.print(currentState);
    Serial.print(" -> ");
    Serial.print(transition->nextState);
    Serial.print(" (Event: ");
    Serial.print(event);
    Serial.println(")");
    
    // Execute action if defined
    if (transition->action != NULL) {
        transition->action();
    }
    
    // Change state to next state
    currentState = transition->nextState;
    
    return true;
}

// Get current state
State fsm_get_current_state(void) {
    return currentState;
}

// Set current state directly (bypass transition logic)
void fsm_set_current_state(State state) {
    Serial.print("FSM State set directly to: ");
    Serial.println(state);
    currentState = state;
}

// Reset FSM to initial state
void fsm_reset(State initialState) {
    currentState = initialState;
    transitionCount = 0;
    
    // Clear event queue
    queueHead = 0;
    queueTail = 0;
    queueSize = 0;
    
    Serial.print("FSM Reset to state: ");
    Serial.println(initialState);
}
