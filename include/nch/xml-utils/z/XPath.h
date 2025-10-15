#pragma once
#include "libxml/tree.h"
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <string>
#include <vector>

namespace nch { class XPath {
public:
    /// @brief Runs an XPath query on the specified 'siteHTML'.
    /// @param siteHTML XML to run an XPath query on.
    /// @param xpQuery The XPath query.
    /// @return a vector of stringed XML nodes equal to the XPath query result.
    static std::vector<std::string> query(const std::string& siteHTML, const std::string& xpQuery);
    /// @brief Same as query() but return only the first result.
    static std::string queryFirst(const std::string& siteHTML, const std::string& xpQuery);
    /// @brief Same as query() but each element is shrunk down to its bare textContent (just words - no tags, attributes, etc.).
    /// @param delim Separator between inner tags (defaults to ' ').
    /// @return Result of the XPath query on 'siteHTML' (a vector) where each element's contents are separated by 'delim'.
    static std::vector<std::string> queryContent(const std::string& siteHTML, const std::string& xpQuery, const std::string& delim = " ");
    /// @brief Same as queryContent() but return only the first result.
    static std::string queryContentFirst(const std::string& siteHTML, const std::string& xpQuery, const std::string& delim = " ");
private:
    static bool buildTools(const std::string& siteHTML, const std::string& xpQuery, htmlDocPtr& doc, xmlXPathContextPtr& xpathCtx, xmlXPathObjectPtr& xpathObj);
    static bool buildTools(const std::string& siteHTML, htmlDocPtr& doc);
    static void addSubNodesToVec(xmlNodePtr node, std::vector<xmlNodePtr>& vec);
}; }