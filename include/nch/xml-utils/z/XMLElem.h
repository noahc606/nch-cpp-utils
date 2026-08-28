#pragma once
#include <cstdint>
#include <libxml/tree.h>
#include <string>
#include <vector>
#include <nch/cpp-utils/color.h>
#include <nch/cpp-utils/log.h>
#include <nch/math-utils/vec2.h>
#include <nch/math-utils/vec3.h>

namespace nch {

/* One XML text value -> a T, the way nlohmann's from_json reads one JSON value. Every reader on XMLElem
   (getOpt, getAttrOpt, ...) goes through these, so a type spelled here needs no separate parsing step at
   the call site. Each returns whether the text was readable; 'out' keeps its old value when it wasn't. */
bool fromXMLText(const std::string& s, std::string& out);
//"true"/"false", "yes"/"no", "1"/"0", case-insensitive.
bool fromXMLText(const std::string& s, bool& out);
bool fromXMLText(const std::string& s, double& out);
bool fromXMLText(const std::string& s, long double& out);
bool fromXMLText(const std::string& s, float& out);
bool fromXMLText(const std::string& s, int& out);
bool fromXMLText(const std::string& s, int64_t& out);
//"rgb(r,g,b)", "rgba(r,g,b,a)", or a "#rrggbb"/"#rrggbbaa" hex string ('#' optional).
bool fromXMLText(const std::string& s, Color& out);

/**
 * @brief A borrowed handle to one element of a parsed document (XMLDoc owns the tree it points into).
 *
 * Null-safe: every accessor on a handle that doesn't exist() yields the empty value, so
 * getChild("a").getChild("b").getAttr("c") needs no checks in between.
 */
class XMLElem {
public:
    XMLElem();
    XMLElem(xmlNode* node);

    //False for a handle onto a missing element, and for anything in the tree that isn't an element.
    bool exists() const;
    xmlNode* getRaw() const;
    std::string getName() const;
    bool isNamed(const char* name) const;

    /* Traversal. Text, comment and other non-element nodes are never visited. */
    bool hasChild(const char* name) const;
    XMLElem getChild(const char* name) const;
    std::vector<XMLElem> getChildren() const;
    std::vector<XMLElem> getChildren(const char* name) const;

    /* Raw text */
    //Text content of this element and its descendants, trimmed.
    std::string getText() const;
    std::string getChildText(const char* name) const;
    bool hasAttr(const char* name) const;
    //"" when the attribute is absent, which is also what an empty attribute reads as.
    std::string getAttr(const char* name) const;

    /* Typed reads. An absent value keeps the fallback; a present but unreadable one logs and keeps it. */
    template<typename T> T getOpt(const char* childName, const T& fallback) const
    {
        XMLElem ch = getChild(childName);
        if(!ch.exists()) return fallback;
        return readOr(ch.getText(), fallback, childName);
    }
    template<typename T> T getAttrOpt(const char* attrName, const T& fallback) const
    {
        if(!hasAttr(attrName)) return fallback;
        return readOr(getAttr(attrName), fallback, attrName);
    }
    //Overloads so a string literal fallback yields a std::string rather than deducing a char array.
    std::string getOpt(const char* childName, const char* fallback) const;
    std::string getAttrOpt(const char* attrName, const char* fallback) const;

    /**
     * @brief Read the numbers out of a multi-component value ("x y z", "1, 2, 3") - the form every XML
     *        value with more than one part is written in, commas and whitespace being interchangeable.
     * @param maxCount Stop after this many numbers, or -1 for all of them.
     * @return The numbers read, which may be fewer than the text holds parts if one of them is malformed.
     */
    static std::vector<double> readNumbers(const std::string& s, int maxCount = -1);
    static std::string toStdString(const xmlChar* s);
    static const xmlChar* toXmlChar(const char* s);
private:
    template<typename T> T readOr(const std::string& text, const T& fallback, const char* what) const
    {
        T ret = fallback;
        if(fromXMLText(text, ret)) return ret;
        Log::warnv(__PRETTY_FUNCTION__, "using the fallback", "Couldn't read \"%s\" of <%s> from \"%s\"", what, getName().c_str(), text.c_str());
        return fallback;
    }

    xmlNode* node;
};

/* Vectors are a plain component list, so a partial one ("0 1") keeps 'out's remaining components. */
template<typename T> bool fromXMLText(const std::string& s, Vec2<T>& out)
{
    std::vector<double> v = XMLElem::readNumbers(s, 2);
    if(v.size()>0) out.x = (T)v[0];
    if(v.size()>1) out.y = (T)v[1];
    return !v.empty();
}
template<typename T> bool fromXMLText(const std::string& s, Vec3<T>& out)
{
    std::vector<double> v = XMLElem::readNumbers(s, 3);
    if(v.size()>0) out.x = (T)v[0];
    if(v.size()>1) out.y = (T)v[1];
    if(v.size()>2) out.z = (T)v[2];
    return !v.empty();
}

}
