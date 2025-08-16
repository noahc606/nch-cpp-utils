#include "NoahAllocTable.h"
#include "file-utils.h"
#include "log.h"
#include "string-utils.h"
#include <sstream>

using namespace nch;

NoahAllocTable::NoahAllocTable(std::string pathWithoutExtension) { open(pathWithoutExtension); }
NoahAllocTable::~NoahAllocTable() { close(); }

bool NoahAllocTable::isHeaderLineValid(std::string hl) { return hl.size()!=0 && hl[0]!=' '; }
bool NoahAllocTable::isOpen() { return fpFilename!="???null???"; }

/*
    Opens the fpNat file (which will be kept open until close() is called).
    Reads the fpHead file (open, then close)
*/
void NoahAllocTable::open(std::string pathWithoutExtension)
{
    //Close last
    close();

    //Create file(s) if nonexistent
    fpFilename = pathWithoutExtension;
    std::string fpNatPath = fpFilename+".nat";
    std::string fpHeadPath = fpFilename+".nath";
    fpNat = std::fopen(fpNatPath.c_str(), "ab+");   std::fclose(fpNat);
    fpHead = std::fopen(fpHeadPath.c_str(), "a+");  std::fclose(fpHead);

    //Open NAT file for reading/writing
    fpNat = std::fopen(fpNatPath.c_str(), "r+b");

    //Go thru all lines of the 'fpHead' file and add them as header entries. Then, close fpHead.
    fpHead = std::fopen(fpHeadPath.c_str(), "r+");
    std::vector<std::string> fphLines = FileUtils::getFileLines(fpHead);
    for(int i = 0; i<fphLines.size(); i++) {
        if(isHeaderLineValid(fphLines[i])) {
            addHeaderEntFrom(fphLines[i]);
        }
    }
    std::fclose(fpHead);

    //Update unallocParts from gathered header data
    setUnallocPartsFromHeader();
}

/*
    Closes the fpNat file (which will be kept open until close() is called).
    Updates the fpHead file (open, add data from within headerLbls, then close)
*/
void NoahAllocTable::close()
{
    if(!isOpen()) return;

    //Close NAT file
    if(fpNat!=nullptr) {
        std::fclose(fpNat);
        fpNat = nullptr;
    }

    //Open header file, update its contents, then close it again.
    std::string fpHeadPath = fpFilename+".nath";
    fpHead = std::fopen(fpHeadPath.c_str(), "w");
    for(auto elem : headerLbls) {
        auto ent = headerEnts.find(elem.second);
        if(ent==headerEnts.end()) {
            nch::Log::warnv(__PRETTY_FUNCTION__, "canceling line write", "Out-of-sync headerLbls & headerEnts: couldn't find ptr %d within headerEnts", elem.second);
            continue;
        }

        std::stringstream ss; ss << elem.first << ":[" << elem.second << "," << ent->second << "]\n";
        nch::FileUtils::writeToFile(fpHead, ss.str());
    }
    std::fclose(fpHead);
    fpHead = nullptr;

    //Reset unallocatedParts and header caches
    fpFilename = "???null???";
    unallocParts.clear();
    headerEnts.clear();
    headerLbls.clear();
}

/*
    Returns a header entry (<dataPtr, dataSize>) based on its label.
*/
std::pair<int64_t, int64_t> NoahAllocTable::findLabeledData(std::string lbl)
{
    if(!isHeaderLineValid(lbl)) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "returning -1", "Label \"%s\" is invalid", lbl.c_str());
        return std::make_pair(-1, -1);
    }

    auto itr = headerLbls.find(lbl);
    if(itr==headerLbls.end()) {
        return std::make_pair(-1, -1);
    } else {
        return std::make_pair(itr->second, headerEnts.find(itr->second)->second);
    }
}

std::vector<std::string> NoahAllocTable::getLabelList()
{
    std::vector<std::string> res;
    for(auto elem : headerLbls) {
        res.push_back(elem.first);
    }
    return res;
}

int64_t NoahAllocTable::save(std::string label, unsigned char* data, int64_t size)
{
    auto ent = findLabeledData(label);

    //Data labeled 'label' does NOT exist yet: Simply alloc() new without freeing anything
    if(ent.first==-1) return alloc(label, data, size);

    //Size of this 'data' matches the already existing 'ent': Overwrite old data.
    if(size==ent.second) {
        std::fseek(fpNat, ent.first, SEEK_SET);
        std::fwrite(data, size, 1, fpNat);
        return ent.first;
    }        
    
    //Data to save has different size: Free and alloc() new.
    free(label);
    return alloc(label, data, size);
}

int64_t NoahAllocTable::save(std::string label, std::vector<unsigned char>& data)
{
    return save(label, &data[0], data.size());
}

int64_t NoahAllocTable::save(std::string label, std::string data)
{
    std::vector<unsigned char> charData;
    for(int i = 0; i<data.size(); i++) {
        charData.push_back(data[i]);
    }

    return save(label, charData);
}

std::vector<unsigned char> NoahAllocTable::load(std::string label)
{
    std::vector<unsigned char> res;     //'res'ult to be built
    auto ent = findLabeledData(label);  //Get header entry related to label.

    //If 'ent' invalid or not found, return empty 'res'.
    if(ent.first==-1) { return res; }

    //Build 'res' by reading the file 1 byte (unsigned char) at a time
    for(int i = 0; i<ent.second; i++) {
        unsigned char buffer;
        std::fseek(fpNat, ent.first+i, SEEK_SET);
        std::fread(&buffer, 1, 1, fpNat);
        res.push_back(buffer);
    }

    //Return
    return res;
}

void NoahAllocTable::putInfo(std::stringstream& ss)
{
    ss << "unallocParts: [ ";
    for(auto it = unallocParts.begin(); it!=unallocParts.end(); it++)
        ss << "(" << it->first << "," << it->second << ") ";
    ss << "];\n";

    ss << "headerEnts: [ ";
    for(auto it = headerEnts.begin(); it!=headerEnts.end(); it++)
        ss << "(" << it->first << "," << it->second << ") ";
    ss << "];\n";

    ss << "headerLbls: [ ";
    for(auto it = headerLbls.begin(); it!=headerLbls.end(); it++)
        ss << "(\"" << it->first << "\"->" << it->second << ") ";
    ss << "];\n";
}


/*
    Allocate data within the .nat file and store its reference within 'headerEnts.

    Returns: Location of newly allocated data in bytes (will be written from 'headerEnts' to file upon close()).
*/
int64_t NoahAllocTable::alloc(std::string label, unsigned char* data, int64_t size)
{
    /* Prelim. checks */
    if(!isOpen()) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "returning -1", "NAT is not open");
        return -1;
    }
    if(!isHeaderLineValid(label)) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "returning -1", "Label \"%s\" is invalid", label.c_str());
        return -1;
    }
    if(findLabeledData(label).first!=-1) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "returning -1", "Tried to allocate memory for \"%s\" which already exists in header (try using save() instead?)", label.c_str());
        return -1;
    }

    
    /* Find space in the file for writing */
    //Try to find an area that is (ideally) just big enough to hold the data.
    auto area = unallocParts.lower_bound(size);

    /* Allocate more space or use existing space within the file */
    int64_t dataSize;
    int64_t dataPos;
    if(area==unallocParts.end()) {  /* Should allocate more space in file */
        //Find unallocated space at end (if it exists)
        int64_t freeSpaceEnd = 0;
        for(auto elem : unallocParts) {
            if(elem.first+elem.second==natSize) {
                freeSpaceEnd = elem.first;
                break;
            }
        }

        dataSize = size;
        dataPos = natSize-freeSpaceEnd;

        //Increase .nat file size
        increaseNatSize(dataSize-freeSpaceEnd);
    } else {                        /* Should use existing empty space within file */
        dataSize = size;
        dataPos = area->second;
    }

    /* Store new data */
    //Write stuff to NAT file
    std::fseek(fpNat, dataPos, SEEK_SET);
    std::fwrite(data, size, 1, fpNat);
    //Save changes
    fpNat = std::freopen((fpFilename+".nat").c_str(), "r+b", fpNat);
    if(fpNat==NULL) {
        nch::Log::error(__PRETTY_FUNCTION__, "Failed to save changes to file \"%s\"", (fpFilename+".nat").c_str());
    }
    
    /* Change map structures */
    removeUnallocArea(dataPos, dataSize);
    if(headerEnts.find(dataPos)==headerEnts.end()) {
        headerEnts.insert(std::make_pair(dataPos, dataSize));
    } else {
        headerEnts.find(dataPos)->second = dataSize;
    }
    if(headerLbls.find(label)==headerLbls.end()) {
        headerLbls.insert(std::make_pair(label, dataPos));
    } else {
        headerLbls.find(label)->second = dataPos;
    }

    return dataPos;
}

/*
    Given a pointer to (location of, in bytes) some data, zero-out that data.
*/
void NoahAllocTable::free(std::string label)
{
    /* Prelim. checks */
    if(!isOpen()) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "canceling", "NAT is not open");
        return;
    }
    if(!isHeaderLineValid(label)) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "returning -1", "Label \"%s\" is invalid", label.c_str());
        return;
    }

    /* Find data */
    //Find the header entry holding 'dataPos' and 'dataSize'.
    //If we can't find it, stop and return error.
    auto ent = findLabeledData(label);
    if(ent.first==-1) {
        nch::Log::warnv(__PRETTY_FUNCTION__, "canceling", "Couldn't find valid data labeled by \"%s\"", label.c_str());
        return;
    }
    int64_t dataSize = ent.second;

    /* Remove data pointer and zero-out data */
    //Remove the header entry
    headerEnts.erase(ent.first);
    headerLbls.erase(label);
    //Zero-out NAT data
    fseek(fpNat, ent.first, SEEK_SET);
    std::vector<unsigned char> bytes; for(int i = 0; i<dataSize; i++) bytes.push_back(0b00000000);
    fwrite(bytes.data(), 1, dataSize, fpNat);
    //Save changes
    fpNat = std::freopen((fpFilename+".nat").c_str(), "r+b", fpNat);
    if(fpNat==NULL) {
        nch::Log::error(__PRETTY_FUNCTION__, "Failed to save changes to file \"%s\"", (fpFilename+".nat").c_str());
    }

    /* Update unallocParts */
    setUnallocPartsFromHeader();
}

void NoahAllocTable::addHeaderEntFrom(std::string headerFileLine)
{
    //On this specific line, find 'elem1' and 'elem2'
    std::stringstream elem1;    //String left of ":"
    std::stringstream elem2;    //String right of ":"
    bool foundSep = false;      //Found separator (":")
    for(int j = 0; j<headerFileLine.size(); j++) {    
        if(headerFileLine[j]==':') {
            foundSep = true;
            continue;
        }
        
        if(!foundSep)   { elem1 << headerFileLine[j]; }
        else            { elem2 << headerFileLine[j]; }
    }

    //Track elem1 and elem2.
    std::string label = elem1.str();
    std::string ptr = elem2.str();
    std::vector<int64_t> vi64 = nch::StringUtils::parseI64Array(ptr);

    if(vi64.size()==2) {
        headerLbls.insert(std::make_pair(label, vi64[0]));
        headerEnts.insert(std::make_pair(vi64[0], vi64[1]));
    } else {
        Log::warnv(__PRETTY_FUNCTION__, "adding (-1, -1) as header entry", "Couldn't parse I64 array from elem2==\"%s\"", elem2.str().c_str());
    }
}

void NoahAllocTable::setUnallocPartsFromHeader()
{
    //Get file size of NAT file
    std::fseek(fpNat, 0, SEEK_END);
    natSize = std::ftell(fpNat);

    //Check if size of NAT makes sense - if not, add empty bytes to NAT (this is abnormal).
    if(headerEnts.size()>0) {
        auto ent = std::prev(headerEnts.end());
        int64_t minimumSize = ent->first+ent->second;
        if(natSize<minimumSize) {
            nch::Log::warnv(__PRETTY_FUNCTION__, "adding empty bytes", "Implied minimum size '%d' within header > actual size '%d' of NAT", minimumSize, natSize);
            int64_t numNewBtyes = minimumSize-natSize;

            unsigned char* bytes = new unsigned char[numNewBtyes];
            fwrite(bytes, 1, numNewBtyes, fpNat);
            delete[] bytes;

            natSize = minimumSize;
        }
    }


    //Update 'unallocParts'
    unallocParts.clear();
    unallocParts.insert(std::make_pair(natSize, 0));
    for(auto& ent : headerEnts) {
        removeUnallocArea(ent.first, ent.second);
    }
}

void NoahAllocTable::removeUnallocArea(int64_t newDataPos, int64_t newDataSize)
{
    int64_t ndp1 = newDataPos;
    int64_t ndp2 = newDataSize+newDataPos;

    std::vector<std::pair<int64_t, int64_t>> toRemove;
    std::vector<std::pair<int64_t, int64_t>> toAdd;
    for(auto& part : unallocParts) {
        int64_t p1 = part.second;
        int64_t p2 = part.second+part.first;
        
        //If an intersecting area is found...
        if( ndp1<=p2 && p1<=ndp2 ) {
            //If this part's 'end' touches new data area
            if(ndp1>p1) {
                //Cut off a right piece of this 'part'
                std::pair<int64_t, int64_t> newPart1 = part;
                newPart1.first -= (p2-ndp1); //Changing size
                toRemove.push_back(part);
                toAdd.push_back(newPart1);
            }
            //If this part's 'beginning' touches new data area
            if(ndp2<p2) {
                //Cut off a left piece of this part
                std::pair<int64_t, int64_t> newPart2 = part;
                newPart2.first -= (ndp2-p1); //Changing size
                newPart2.second += (ndp2-p1); //Changing pos
                toRemove.push_back(part);
                toAdd.push_back(newPart2);
            }
            //If this part is completely contained by the new data area
            if(ndp1<=p1 && ndp2>=p2) {
                //Remove entire part
                toRemove.push_back(part);
            }
        }
    }

    for(auto& rem : toRemove) {
        for(auto itr = unallocParts.find(rem.first); itr!=unallocParts.end(); itr++) {
            if(itr->second==rem.second) {
                unallocParts.erase(itr);
                break;
            }
        }
    }

    for(auto& add : toAdd) {
        unallocParts.insert(add);
    }
}

void NoahAllocTable::increaseNatSize(int64_t by)
{
    unallocParts.insert(std::make_pair(by, natSize));
    natSize += by;
}