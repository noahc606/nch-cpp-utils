#include "XMLElem.h"
#include <sstream>
#include <nch/cpp-utils/string-utils.h>
using namespace nch;

static const std::string xmlWhitespace = " \t\n\r";

bool nch::fromXMLText(const std::string& s, std::string& out)
{
    out = s;
    return true;
}
bool nch::fromXMLText(const std::string& s, bool& out)
{
    std::string t = StringUtils::lowercased(StringUtils::trimmed(s, xmlWhitespace));
    if(t=="true" || t=="1" || t=="yes") { out = true; return true; }
    if(t=="false"|| t=="0" || t=="no")  { out = false; return true; }
    return false;
}
bool nch::fromXMLText(const std::string& s, double& out)
{
    try {
        out = std::stod(StringUtils::trimmed(s, xmlWhitespace));
        return true;
    } catch(...) {}
    return false;
}
bool nch::fromXMLText(const std::string& s, long double& out)
{
    double d = 0.0;
    if(!fromXMLText(s, d)) return false;
    out = (long double)d;
    return true;
}
bool nch::fromXMLText(const std::string& s, float& out)
{
    double d = 0.0;
    if(!fromXMLText(s, d)) return false;
    out = (float)d;
    return true;
}
bool nch::fromXMLText(const std::string& s, int& out)
{
    int64_t i = 0;
    if(!fromXMLText(s, i)) return false;
    out = (int)i;
    return true;
}
bool nch::fromXMLText(const std::string& s, int64_t& out)
{
    try {
        out = std::stoll(StringUtils::trimmed(s, xmlWhitespace));
        return true;
    } catch(...) {}
    return false;
}
bool nch::fromXMLText(const std::string& s, Color& out)
{
    std::string t = StringUtils::trimmed(s, xmlWhitespace);
    if(t.empty()) return false;

    size_t lp = t.find('(');
    if(lp==std::string::npos) {
        //Hex form. Alpha is optional, so both "#rrggbb" and "#rrggbbaa" are valid.
        if(t[0]=='#') t = t.substr(1);
        if(t.size()!=6 && t.size()!=8) return false;
        if(t.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos) return false;
        out = Color::fromStringB16(t);
        return true;
    }

    size_t rp = t.rfind(')');
    if(rp==std::string::npos || rp<=lp) return false;
    std::vector<double> v = XMLElem::readNumbers(t.substr(lp+1, rp-(lp+1)), 4);
    if(v.size()<3) return false;
    out = Color::fromDoubles255(v[0], v[1], v[2], v.size()>3 ? v[3] : 255.0);
    return true;
}

XMLElem::XMLElem()
: node(nullptr) {}
XMLElem::XMLElem(xmlNode* node)
: node(node) {}

bool XMLElem::exists() const { return node!=nullptr && node->type==XML_ELEMENT_NODE; }
xmlNode* XMLElem::getRaw() const { return node; }
std::string XMLElem::getName() const
{
    if(!exists()) return "";
    return toStdString(node->name);
}
bool XMLElem::isNamed(const char* name) const
{
    return exists() && xmlStrcmp(node->name, toXmlChar(name))==0;
}

bool XMLElem::hasChild(const char* name) const { return getChild(name).exists(); }
XMLElem XMLElem::getChild(const char* name) const
{
    if(!exists()) return XMLElem();
    for(xmlNode* c = node->children; c!=nullptr; c = c->next) {
        XMLElem ce(c);
        if(ce.isNamed(name)) return ce;
    }
    return XMLElem();
}
std::vector<XMLElem> XMLElem::getChildren() const
{
    std::vector<XMLElem> ret;
    if(!exists()) return ret;
    for(xmlNode* c = node->children; c!=nullptr; c = c->next) {
        XMLElem ce(c);
        if(ce.exists()) ret.push_back(ce);
    }
    return ret;
}
std::vector<XMLElem> XMLElem::getChildren(const char* name) const
{
    std::vector<XMLElem> ret;
    if(!exists()) return ret;
    for(xmlNode* c = node->children; c!=nullptr; c = c->next) {
        XMLElem ce(c);
        if(ce.isNamed(name)) ret.push_back(ce);
    }
    return ret;
}

std::string XMLElem::getText() const
{
    if(!exists()) return "";
    xmlChar* c = xmlNodeGetContent(node);
    std::string ret = toStdString(c);
    if(c!=nullptr) xmlFree(c);
    return StringUtils::trimmed(ret, xmlWhitespace);
}
std::string XMLElem::getChildText(const char* name) const
{
    return getChild(name).getText();
}
bool XMLElem::hasAttr(const char* name) const
{
    return exists() && xmlHasProp(node, toXmlChar(name))!=nullptr;
}
std::string XMLElem::getAttr(const char* name) const
{
    if(!exists()) return "";
    xmlChar* p = xmlGetProp(node, toXmlChar(name));
    std::string ret = toStdString(p);
    if(p!=nullptr) xmlFree(p);
    return ret;
}

std::string XMLElem::getOpt(const char* childName, const char* fallback) const
{
    return getOpt<std::string>(childName, std::string(fallback));
}
std::string XMLElem::getAttrOpt(const char* attrName, const char* fallback) const
{
    return getAttrOpt<std::string>(attrName, std::string(fallback));
}

std::vector<double> XMLElem::readNumbers(const std::string& s, int maxCount)
{
    std::string t = s;
    for(char& c : t) if(c==',') c = ' ';

    std::vector<double> ret;
    std::stringstream ss(t);
    double d = 0.0;
    while((maxCount<0 || (int)ret.size()<maxCount) && (ss >> d)) {
        ret.push_back(d);
    }
    return ret;
}
std::string XMLElem::toStdString(const xmlChar* s)
{
    if(s==nullptr) return "";
    return std::string(reinterpret_cast<const char*>(s));
}
const xmlChar* XMLElem::toXmlChar(const char* s)
{
    return reinterpret_cast<const xmlChar*>(s);
}
