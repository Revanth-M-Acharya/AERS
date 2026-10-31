/**
 * @file EmergencyValidator.cpp
 * @brief Implementation of emergency request validation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/EmergencyValidator.hpp"

namespace aegis::coordinator::control_center {

ValidationResult EmergencyValidator::Validate(const EmergencyRequest& req) const {
    ValidationResult res;

    // 1. Caller identification check
    if (req.callerId.empty() && req.callerPhone.empty()) {
        res.isValid = false;
        res.rejectionReason = "Missing caller identification or phone number";
        return res;
    }

    // 2. Location validation
    if (req.locationDescription.empty() && req.closestIntersectionId.empty() &&
        (req.latitude == 0.0 && req.longitude == 0.0)) {
        res.isValid = false;
        res.rejectionReason = "No location coordinates, intersection ID, or address provided";
        return res;
    }

    // Geographic boundary sanity check
    if (req.latitude != 0.0 || req.longitude != 0.0) {
        if (req.latitude < -90.0 || req.latitude > 90.0 ||
            req.longitude < -180.0 || req.longitude > 180.0) {
            res.isValid = false;
            res.rejectionReason = "GPS coordinates out of valid geographic range";
            return res;
        }
    }

    // 3. Casualty count check
    if (req.casualtyCount == 0) {
        res.isValid = false;
        res.rejectionReason = "Casualty count must be at least 1";
        return res;
    }
    if (req.casualtyCount > 500) {
        res.isValid = false;
        res.rejectionReason = "Casualty count exceeds plausible incident threshold";
        return res;
    }

    // 4. Patient age sanity check
    if (req.patientAgeYears > 125) {
        res.isValid = false;
        res.rejectionReason = "Patient age exceeds physiological limits";
        return res;
    }

    res.isValid = true;
    res.sanitizedLocation = !req.closestIntersectionId.empty() ?
        req.closestIntersectionId : req.locationDescription;
    return res;
}

} // namespace aegis::coordinator::control_center

