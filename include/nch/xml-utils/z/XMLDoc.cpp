#include "XMLDoc.h"
#include <libxml/parser.h>
#include <nch/cpp-utils/log.h>
using namespace nch;

XMLDoc::XMLDoc()
: doc(nullptr) {}
XMLDoc::XMLDoc(XMLDoc&& other)
: doc(other.doc)
{
    other.doc = nullptr;
}
XMLDoc& XMLDoc::operator=(XMLDoc&& other)
{
    if(this!=&other) {
        if(doc!=nullptr) xmlFreeDoc(doc);
        doc = other.doc;
        other.doc = nullptr;
    }
    return *this;
}
XMLDoc::~XMLDoc()
{
    if(doc!=nullptr) xmlFreeDoc(doc);
}

XMLDoc XMLDoc::loadFromFile(const std::string& path)
{
    xmlInitParser();

    XMLDoc ret;
    ret.doc = xmlReadFile(path.c_str(), nullptr, getParseOptions());
    if(ret.doc==nullptr) {
        Log::warn(__PRETTY_FUNCTION__, "Failed to parse XML file \"%s\"", path.c_str());
    }
    return ret;
}
XMLDoc XMLDoc::parse(const std::string& text, const std::string& context)
{
    xmlInitParser();

    XMLDoc ret;
    ret.doc = xmlReadMemory(text.c_str(), (int)text.size(), context.c_str(), nullptr, getParseOptions());
    if(ret.doc==nullptr) {
        Log::warn(__PRETTY_FUNCTION__, "Failed to parse XML from \"%s\"", context.c_str());
    }
    return ret;
}

bool XMLDoc::isLoaded() const { return doc!=nullptr; }
XMLElem XMLDoc::getRoot() const
{
    if(doc==nullptr) return XMLElem();
    return XMLElem(xmlDocGetRootElement(doc));
}
xmlDoc* XMLDoc::getRaw() const { return doc; }

int XMLDoc::getParseOptions()
{
    return XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS;
}
