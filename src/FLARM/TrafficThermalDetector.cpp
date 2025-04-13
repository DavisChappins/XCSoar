// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficThermalDetector.hpp"
#include "FLARM/Traffic.hpp" // Include the full definition
#include "Geo/GeoPoint.hpp"

#include "time/Cast.hxx"

#include <cmath> // For fabs, hypot
#include <numeric> // For std::accumulate (potential use)


// Private helper method implementations
// =====================================

void TrafficThermalDetector::UpdateCirclingState(TrafficThermalInfo& info, const FlarmTraffic& traffic, TimeStamp now) {
    // Basic checks: need location and turn rate
    if (!traffic.location_available || !traffic.turn_rate_received) {
        if (info.current_state == TrafficThermalInfo::State::CIRCLING) {
            info.current_state = TrafficThermalInfo::State::RECENTLY_STOPPED;
            info.last_update_time = now;
        } else if (info.current_state == TrafficThermalInfo::State::POTENTIALLY_CIRCLING) {
             info.current_state = TrafficThermalInfo::State::IDLE;
             info.ResetDetectionState();
        }
        return;
    }

    // Add current position to history (limited size)
    info.position_history.push_back(traffic.location);
    if (info.position_history.size() > POSITION_HISTORY_SIZE) {
        info.position_history.erase(info.position_history.begin());
    }

    bool meets_turn_criteria = std::fabs(traffic.turn_rate) >= MIN_TURN_RATE_DEG_S;
    bool meets_area_criteria = CheckAreaConstraint(info); // Check if staying within bounds

    // --- State Machine Logic ---

    switch (info.current_state) {
        case TrafficThermalInfo::State::IDLE:
            if (meets_turn_criteria) {
                info.current_state = TrafficThermalInfo::State::POTENTIALLY_CIRCLING;
                info.circling_start_time = now;
                info.last_update_time = now;
                info.ResetDetectionState(); // Start fresh for averaging etc.
                info.position_history.push_back(traffic.location); // Add first point
            }
            break;

        case TrafficThermalInfo::State::POTENTIALLY_CIRCLING:
            if (meets_turn_criteria && meets_area_criteria) {
                // Still potentially circling, check if duration met
                if ((now - info.circling_start_time) >= MIN_CIRCLING_DURATION) {
                    // Duration met, transition to CIRCLING
                    info.current_state = TrafficThermalInfo::State::CIRCLING;
                    info.last_update_time = now;
                    UpdateAverageClimb(info, traffic); // Start averaging
                    UpdateCenterLocation(info, traffic);
                } else {
                    // Duration not yet met, just update time
                    info.last_update_time = now;
                }
            } else {
                // Criteria failed, go back to IDLE
                info.current_state = TrafficThermalInfo::State::IDLE;
                info.ResetDetectionState();
            }
            break;

        case TrafficThermalInfo::State::CIRCLING:
            if (meets_turn_criteria && meets_area_criteria) {
                // Continue circling
                info.last_update_time = now;
                UpdateAverageClimb(info, traffic);
                UpdateCenterLocation(info, traffic);
            } else {
                // Stopped circling
                info.current_state = TrafficThermalInfo::State::RECENTLY_STOPPED;
                info.last_update_time = now;
                // Keep average climb rate and center location for display
            }
            break;

        case TrafficThermalInfo::State::RECENTLY_STOPPED:
            // Stay in this state until Cleanup removes it based on MAX_RECENTLY_STOPPED_AGE
            // If it starts circling again...
             if (meets_turn_criteria) {
                info.current_state = TrafficThermalInfo::State::POTENTIALLY_CIRCLING;
                info.circling_start_time = now;
                info.last_update_time = now;
                info.ResetDetectionState(); // Start fresh averaging
                info.position_history.push_back(traffic.location);
            }
            break;
    }
}

bool TrafficThermalDetector::CheckAreaConstraint(const TrafficThermalInfo& info) const {
    if (info.position_history.size() < 2) {
        return true; // Not enough data to check yet
    }

    GeoBoundingBox bbox;
    for(const auto& p : info.position_history) {
        bbox.Extend(p);
    }

    // Check if the diagonal distance is within the limit
    // This is a simplification; could also check max distance from center etc.
    return bbox.IsValid() && bbox.DiagonalDistance() <= MAX_DIAMETER_M;
}

void TrafficThermalDetector::UpdateAverageClimb(TrafficThermalInfo& info, const FlarmTraffic& traffic) {
    if (traffic.climb_rate_avg30s_available) {
        info.climb_rate_sum += traffic.climb_rate_avg30s;
        info.climb_rate_samples++;
        if (info.climb_rate_samples > 0) {
            info.average_climb_rate = info.climb_rate_sum / info.climb_rate_samples;
        }
    }
}

void TrafficThermalDetector::UpdateCenterLocation(TrafficThermalInfo& info, const FlarmTraffic& traffic) {
    // Simple approach: use the latest location as the center for now
    // Could be improved by averaging positions in history
    if (traffic.location_available) {
        info.center_location = traffic.location;
    }
}


// Public method implementations
// ============================

void TrafficThermalDetector::Update(const FlarmTraffic &traffic, const FlarmTraffic *last_traffic, TimeStamp now) {
    // Get or create the state entry for this traffic ID
    // operator[] conveniently creates a default TrafficThermalInfo if key doesn't exist
    TrafficThermalInfo& info = thermal_states[traffic.id];
    info.id = traffic.id; // Ensure ID is set if it's a new entry

    // Update the state machine
    UpdateCirclingState(info, traffic, last_traffic, now);
}

void TrafficThermalDetector::Cleanup(TimeStamp now) {
   for (auto it = thermal_states.begin(); it != thermal_states.end(); /* no increment here */) {
        const TrafficThermalInfo& info = it->second;
        bool remove = false;

        if (info.current_state == TrafficThermalInfo::State::IDLE) {
            remove = true; // Remove idle states immediately
        } else if (info.current_state == TrafficThermalInfo::State::RECENTLY_STOPPED) {
            if ((now - info.last_update_time) > MAX_RECENTLY_STOPPED_AGE) {
                remove = true; // Remove recently stopped if too old
            }
        } else if ((now - info.last_update_time) > (MAX_RECENTLY_STOPPED_AGE * 2)) {
             // Safety net: remove any state that hasn't been updated in a long time
             remove = true;
        }


        if (remove) {
            it = thermal_states.erase(it); // Erase and advance iterator
        } else {
            ++it; // Advance iterator only if not erased
        }
    }
}

const std::map<FlarmId, TrafficThermalInfo>& TrafficThermalDetector::GetDisplayableThermals() const {
    // Simply return the map. Filtering for display relevance can be done by the caller
    // using info.IsRelevantForDisplay() or checking the state.
    return thermal_states;
}