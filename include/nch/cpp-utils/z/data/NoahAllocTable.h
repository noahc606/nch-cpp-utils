#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

/*
    Class for working with .nat files
*/

namespace nch { class NoahAllocTable {
public:
    NoahAllocTable(std::string pathWithoutExtension);
    ~NoahAllocTable();

    static bool isHeaderLineValid(std::string hd);
    bool isOpen();
    void open(std::string pathWithoutExtension);
    void close();

    std::pair<int64_t, int64_t> findLabeledData(std::string lbl);
    std::vector<std::string> getLabelList();

    int64_t save(std::string label, unsigned char* data, int64_t size);
    int64_t save(std::string label, std::vector<unsigned char>& data);
    int64_t save(std::string label, std::string data);
    std::vector<unsigned char> load(std::string label);
    void putInfo(std::stringstream& ss);
private:
    std::string fpFilename = "???null???";
    FILE* fpNat = nullptr;
    FILE* fpHead = nullptr;

    int64_t natSize = 0;
    /* Unallocated parts multimap */
    std::multimap<int64_t, int64_t> unallocParts;   // UnallocatedParts <dataSize, dataPtr>
    /* Header entries and header labels */
    std::map<int64_t, int64_t> headerEnts;          // Header entries: <dataPtr, dataSize>
    std::map<std::string, int64_t> headerLbls;      // Header labels: <label, dataPtr>

    int64_t alloc(std::string label, unsigned char* data, int64_t size);
    void free(std::string label);
    void addHeaderEntFrom(std::string headerFileLine);
    void setUnallocPartsFromHeader();
    void removeUnallocArea(int64_t newDataPos, int64_t newDataSize);
    void increaseNatSize(int64_t by);

}; }