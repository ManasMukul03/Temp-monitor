/*
 * ReportExporter.h - saves readings to a CSV file (opens in Excel/LibreOffice).
 */
#ifndef REPORTEXPORTER_H
#define REPORTEXPORTER_H

#include <string>
#include <vector>
#include "Types.h"

class ReportExporter {
public:
    // Returns the number of rows written, or -1 if the file could not be created.
    static long exportCsv(const std::vector<Reading>& readings, const std::string& path);
};

#endif
