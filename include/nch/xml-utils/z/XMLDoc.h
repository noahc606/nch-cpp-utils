#pragma once
#include <libxml/tree.h>
#include <string>
#include "XMLElem.h"

namespace nch {

/**
 * @brief A parsed XML document, owning its tree: it frees on destruction, so a load path can return or
 *        throw partway through without leaking one. Move-only, since the tree has exactly one owner.
 *
 * Parsing never throws - an unreadable or malformed file logs and yields a document with no root - so a
 * caller decides for itself whether a missing document is fatal.
 */
class XMLDoc {
public:
    XMLDoc();
    XMLDoc(XMLDoc&& other);
    XMLDoc& operator=(XMLDoc&& other);
    ~XMLDoc();
    XMLDoc(const XMLDoc&) = delete;
    XMLDoc& operator=(const XMLDoc&) = delete;

    static XMLDoc loadFromFile(const std::string& path);
    /**
     * @brief Parse raw text as a document.
     * @param context Named in the log message when the text doesn't parse (a network response, a row, ...).
     */
    static XMLDoc parse(const std::string& text, const std::string& context = "");

    bool isLoaded() const;
    //A handle onto the document's single root element, which doesn't exist() when nothing was parsed.
    XMLElem getRoot() const;
    xmlDoc* getRaw() const;
private:
    //Blank text nodes are dropped and libxml's own stderr reporting is off: a document that fails is
    //reported once, by the caller, naming what it was trying to load.
    static int getParseOptions();

    xmlDoc* doc;
};

}
