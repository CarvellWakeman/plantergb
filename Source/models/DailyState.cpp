#include <iterator>
#include "../includes/DailyState.hpp"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Constructor
DailyState::DailyState() {}
DailyState::DailyState(const DailyState& d)
{
    copy(d);
}


// Copy
void DailyState::copy(const DailyState& d)
{
    std::unordered_map<unsigned int, LEDState*> states = d.get_time_state_map();
    if (!states.empty()) {
        std::unordered_map<unsigned int, LEDState*>::iterator iter = states.begin();

        // Iterate through all map pairs
        while(iter != states.end()) {
            add_state(iter->first, iter->second);
        }
    }
}


// Accessors
unsigned int DailyState::get_id() const
{
    return id;
}

std::unordered_map<unsigned int, LEDState*> DailyState::get_time_state_map() const
{
    return timeStatePairs;
}


// Mutators
void DailyState::set_id(unsigned int val)
{
    id = val;
}


// CRUD
bool DailyState::add_state(unsigned int time, LEDState* state)
{
    // Check if key already exists, update if it does, insert new key if not
    if (timeStatePairs.count(time) > 0) {
        return (timeStatePairs[time] = state);
    } else {
        return timeStatePairs.insert(std::make_pair(time, state)).second;
    }
}

std::unordered_map<unsigned int, LEDState*> DailyState::get_led_states() const
{
    return timeStatePairs;
}

LEDState* DailyState::get_led_state(unsigned int time)
{
    LEDState* nearest_state = 0;
    unsigned int nearest_time = 0;

    // First check if no time<->state pairs exist
    if (timeStatePairs.count(time) == 0) {
        std::unordered_map<unsigned int, LEDState*>::iterator iter = timeStatePairs.begin();

        // Iterate through all map pairs
        while(iter != timeStatePairs.end()) {
            unsigned int t = iter->first;

            // Only check times greater than or equal to the given time
            if (time >= t) {
                // Find the greatest of the times listed for this day
                nearest_time = MAX(t, nearest_time);
            }

            iter++;
        }        
    } else {
        nearest_time = time;
    }

    nearest_state = timeStatePairs[nearest_time];
    return nearest_state;
}

#include <iostream>

bool DailyState::delete_state(unsigned int time)
{
    timeStatePairs.erase(time);
    return timeStatePairs.count(time) == 0;
}

int DailyState::get_time_state_count() const
{
    return timeStatePairs.size();
}