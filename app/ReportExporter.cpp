#include "ReportExporter.h"

#include <fstream>

long ReportExporter::exportCsv(const std::vector<Reading>& readings, const std::string& path) {
    std::ofstream out(path);
    if (!out) return -1;
    out << "timestamp,temperature_c,level\n";
    for (const Reading& r : readings)
        out << formatTime(r.time) << "," << formatTemp(r.celsius) << "," << levelName(r.level) << "\n";
    return static_cast<long>(readings.size());
}
