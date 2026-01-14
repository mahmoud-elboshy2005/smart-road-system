#include "fsm.h"

// Global current state variable
State currentState = STATE_IDLE;

// Transition table
static Transition transitionTable[MAX_TRANSITIONS];
static int transitionCount = 0;

// FreeRTOS event queue
static QueueHandle_t eventQueue = NULL;

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
    
    // Create FreeRTOS queue for events
    if (eventQueue == NULL) {
        eventQueue = xQueueCreate(MAX_EVENTS, sizeof(Event));
        if (eventQueue == NULL) {
            Serial.println("Error: Failed to create event queue!");
        }
    }
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
    if (eventQueue == NULL) {
        Serial.println("Error: Event queue not initialized!");
        return;
    }
    
    if (xQueueSend(eventQueue, &event, 0) != pdPASS) {
        Serial.println("Warning: Event queue full, event discarded!");
    }
}

// Process all events in the queue (main loop function)
void fsm_process_events(void) {
    if (eventQueue == NULL) {
        Serial.println("[FSM] ERROR: Event queue is NULL!");
        return;
    }
    
    Event event;
    while (xQueueReceive(eventQueue, &event, 0) == pdPASS) {
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
    if (eventQueue != NULL) {
        xQueueReset(eventQueue);
    }
    
    Serial.print("FSM Reset to state: ");
    Serial.println(initialState);
}

// Check if FSM is ready
bool fsm_is_ready(void) {
    return eventQueue != NULL;
}
