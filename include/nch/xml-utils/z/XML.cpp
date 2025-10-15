#include "XML.h"
#include <stdexcept>
using namespace nch;

std::string XML::cleanHTML(const std::string& htmlContent, const std::vector<std::string>& tagNames, bool removeComments) {
    //Parse HTML string
    htmlDocPtr doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if(!doc) throw std::invalid_argument("Provided 'htmlContent' could not be read");

    //Remove <tagName> tags
    for(int i = 0; i<tagNames.size(); i++) {
        removeTagsFromNode(xmlDocGetRootElement(doc), tagNames[i]);
    }
    //Remove <!-- ... --> comments
    if(removeComments) {
        removeCommentsFromNode(xmlDocGetRootElement(doc));
    }
    
    //Save modified document to a memory buffer
    xmlBufferPtr buf = xmlBufferCreate();
    xmlSaveCtxtPtr saveCtx = xmlSaveToBuffer(buf, NULL, XML_SAVE_NO_DECL);
    xmlSaveDoc(saveCtx, doc);
    xmlSaveClose(saveCtx);

    //Clean up
    xmlFreeDoc(doc);

    std::string ret(reinterpret_cast<const char*>(buf->content));
    xmlBufferFree(buf);
    return ret;
}

std::string XML::getAttributeValue(const std::string& nodeString, const std::string& attributeName) {
    xmlDocPtr doc = xmlReadMemory(nodeString.c_str(), nodeString.size(), "noname.xml", nullptr, XML_PARSE_RECOVER);
    if(!doc) throw std::invalid_argument("Provided 'nodeString' could not be read as an XML node");

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == nullptr) {
        xmlFreeDoc(doc);
        return "";
    }

    xmlChar* attr = xmlGetProp(root, reinterpret_cast<const xmlChar*>(attributeName.c_str()));
    std::string value;
    if (attr) {
        value = reinterpret_cast<const char*>(attr);
        xmlFree(attr);
    }

    xmlFreeDoc(doc);
    return value;
}

void XML::removeTagsFromNode(xmlNodePtr node, const std::string& tagName) {
    xmlNodePtr cur = node;
    while(cur) {
        if(cur->type == XML_ELEMENT_NODE && xmlStrcasecmp(cur->name, BAD_CAST tagName.c_str()) == 0) {
            xmlNodePtr toRemove = cur;
            cur = cur->next;
            xmlUnlinkNode(toRemove);
            xmlFreeNode(toRemove);
        } else {
            removeTagsFromNode(cur->children, tagName);
            cur = cur->next;
        }
    }
}
void XML::removeCommentsFromNode(xmlNodePtr node) {
    xmlNodePtr cur = node;
    while(cur) {
        if(cur->type==XML_COMMENT_NODE) {
            xmlNodePtr toRemove = cur;
            cur = cur->next;
            xmlUnlinkNode(toRemove);
            xmlFreeNode(toRemove);
        } else {
            removeCommentsFromNode(cur->children);
            cur = cur->next;
        }
    }
}