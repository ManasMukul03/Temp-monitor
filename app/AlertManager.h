/*
 * AlertManager.h - decides Normal / Warning / Critical for a temperature.
 *   Normal   : T <  warning limit   (default 45 C)
 *   Warning  : warning <= T <= critical
 *   Critical : T >  critical limit  (default 60 C)
 */
#ifndef ALERTMANAGER_H
#define ALERTMANAGER_H

#include <mutex>
#include "Types.h"

class AlertManager {
public:
    AlertManager(double warning = 45.0, double critical = 60.0);

    AlertLevel classify(double celsius) const;
    bool setThresholds(double warning, double critical);   // false if invalid
    double warningLimit() const;
    double criticalLimit() const;

private:
    mutable std::mutex mutex_;   // thresholds can be changed by the menu while the thread classifies
    double warning_;
    double critical_;
};

#endif
