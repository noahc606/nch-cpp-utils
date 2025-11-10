#pragma once
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <string>
#include <vector>

namespace nch { class XML {
public:
    static std::string cleanHTML(const std::string& htmlContent, const std::vector<std::string>& tagNames, bool removeComments = true);
    static std::string cleanHTML(const std::string& htmlContent, bool removeComments = true);
    static std::string getAttributeValue(const std::string& xmlNode, const std::string& attributeName);
private:
    static void removeTagsFromNode(xmlNodePtr node, const std::string& tagName);
    static void removeCommentsFromNode(xmlNodePtr node);
}; }