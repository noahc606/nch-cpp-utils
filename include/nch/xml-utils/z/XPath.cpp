#include "XPath.h"
#include "libxml/tree.h"
#include <libxml/parser.h>
#include <libxml/xpathInternals.h>
#include <sstream>
#include <vector>
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/string-utils.h"
using namespace nch;

std::vector<std::string> XPath::query(const std::string& siteHTML, const std::string& xpQuery)
{
    if(siteHTML=="" || xpQuery=="") { return {}; }

    /* Build HTML document, XPath context, and XPath object */
    htmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    bool builtTools = buildTools(siteHTML, xpQuery, doc, xpathCtx, xpathObj);
    if(!builtTools || !xpathObj->nodesetval) return {};

    /* Process result from 'buildTools' */
    //Result as list of strings
    std::vector<std::string> ret;
    //Process Query Results
    for(int i = 0; i<xpathObj->nodesetval->nodeNr; i++) {
        xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];

        xmlBufferPtr buf = xmlBufferCreate(); if(!buf) continue;
        xmlNodeDump(buf, doc, node, 0, 1);
        ret.push_back((char*)xmlBufferContent(buf));
    }
    //Cleanup
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    
    //Return
    return ret;
}
std::string XPath::queryFirst(const std::string& siteHTML, const std::string& xpQuery)
{
    if(siteHTML=="" || xpQuery=="") { return ""; }

    auto ret = query(siteHTML, xpQuery);
    if(ret.size()==0) return "";
    return ret[0];
}

std::vector<std::string> XPath::queryContent(const std::string& siteHTML, const std::string& xpQuery, const std::string& delim)
{
    if(siteHTML=="" || xpQuery=="") { return {}; }

    /* Build HTML document, XPath context, and XPath object */
    htmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    bool builtTools = buildTools(siteHTML, xpQuery, doc, xpathCtx, xpathObj);
    if(!builtTools || !xpathObj->nodesetval) return {};

    /* Process result from 'buildTools' */
    std::vector<std::string> ret;
    std::vector<xmlNodePtr> finalNodes;

    for(int i = 0; i<xpathObj->nodesetval->nodeNr; i++) {
        xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
        addSubNodesToVec(node, finalNodes);
    }
    for(int i = 0; i<finalNodes.size(); i++) {
        xmlNodePtr node = finalNodes[i];
        
        std::string cont = ""; {
            auto contXML = xmlNodeGetContent(node);
            if(contXML!=nullptr) {
                cont = (char*)contXML;
                xmlFree(contXML);
                cont = StringUtils::trimmed(StringUtils::removedNonASCII(cont))+delim;
            }
        }
        
        if(cont!="") {
            ret.push_back(cont);
        }
    }

    //Cleanup
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    //Return
    return ret;
}
std::string XPath::queryContentFirst(const std::string& siteHTML, const std::string& xpQuery, const std::string& delim)
{
    if(siteHTML=="" || xpQuery=="") { return ""; }

    std::stringstream ret;
    auto contentsList = queryContent(siteHTML, xpQuery);
    for(int i = 0; i<contentsList.size(); i++) {
        ret << contentsList[i] << delim;
    }

    return StringUtils::trimmed(ret.str());
}

bool XPath::buildTools(const std::string& siteHTML, const std::string& xpQuery, htmlDocPtr& doc, xmlXPathContextPtr& xpathCtx, xmlXPathObjectPtr& xpathObj)
{
    bool ret = true;

    doc = htmlReadMemory(siteHTML.c_str(), siteHTML.size(), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if(!doc) {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty vector", "Could not parse HTML");
        ret = false;
    }
    xpathCtx = xmlXPathNewContext(doc);
    if(!xpathCtx) {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty vector", "Failed to create XPath context");
        ret = false;
    }
    xpathObj = xmlXPathEvalExpression((xmlChar*)(xpQuery.c_str()), xpathCtx);
    if(!xpathObj) {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty vector", "Failed to evaluate XPath expression");
        ret = false;
    }

    return ret;
}
bool XPath::buildTools(const std::string& siteHTML, htmlDocPtr& doc)
{
    bool ret = true;
    doc = htmlReadMemory(siteHTML.c_str(), siteHTML.size(), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if(!doc) {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty vector", "Could not parse HTML");
        ret = false;
    }
    return ret;
}

void XPath::addSubNodesToVec(xmlNodePtr node, std::vector<xmlNodePtr>& vec)
{    
    int numChildren = 0;
    for(xmlNodePtr cur = node->children; cur != NULL; cur = cur->next) {
        addSubNodesToVec(cur, vec);
        numChildren++;
    }
    if(numChildren==0) {
        vec.push_back(node);
    }
}