/*
 *  This file is part of WinSparkle (http://winsparkle.org)
 *
 *  Copyright (C) 2009-2013 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#include "appcast.h"
#include "error.h"

#include <expat.h>
#include <vector>
#include <algorithm>
#include <cstring>

namespace winsparkle
{

/*--------------------------------------------------------------------------*
                                XML parsing
 *--------------------------------------------------------------------------*/

namespace
{

#define MVAL(x) x
#define CONCAT3(a,b,c) MVAL(a)##MVAL(b)##MVAL(c)

#define NS_SPARKLE      "http://www.andymatuschak.org/xml-namespaces/sparkle"
#define NS_SEP          '#'
#define NS_SPARKLE_NAME(name) NS_SPARKLE "#" name

#define NODE_CHANNEL    "channel"
#define NODE_ITEM       "item"
#define NODE_RELNOTES   NS_SPARKLE_NAME("releaseNotesLink")
#define NODE_TITLE "title"
#define NODE_DESCRIPTION "description"
#define NODE_ENCLOSURE  "enclosure"
#define ATTR_URL        "url"
#define ATTR_VERSION    NS_SPARKLE_NAME("version")
#define ATTR_SHORTVERSION NS_SPARKLE_NAME("shortVersionString")
#define ATTR_OS         NS_SPARKLE_NAME("os")
#define OS_MARKER       "windows"

// context data for the parser
struct ContextData
{
    ContextData(XML_Parser& p)
        : parser(p),
          in_channel(0), in_item(0), in_relnotes(0), in_title(0), in_description(0)
    {}

    // the parser we're using
    XML_Parser& parser;

    // is inside <channel>, <item> or <sparkle:releaseNotesLink>, <title>, or <description> respectively?
    int in_channel, in_item, in_relnotes, in_title, in_description;

    // parsed <item>s
    std::vector<Appcast> items;
};


void XMLCALL OnStartElement(void *data, const char *name, const char **attrs)
{
    ContextData& ctxt = *static_cast<ContextData*>(data);

    if ( strcmp(name, NODE_CHANNEL) == 0 )
    {
        ctxt.in_channel++;
    }
    else if ( ctxt.in_channel && strcmp(name, NODE_ITEM) == 0 )
    {
        ctxt.in_item++;
        Appcast item;
        ctxt.items.push_back(item);
    }
    else if ( ctxt.in_item )
    {
        if ( strcmp(name, NODE_RELNOTES) == 0 )
        {
            ctxt.in_relnotes++;
        }
        else if ( strcmp(name, NODE_TITLE) == 0 )
        {
            ctxt.in_title++;
        }
        else if ( strcmp(name, NODE_DESCRIPTION) == 0 )
        {
            ctxt.in_description++;
        }
        else if (strcmp(name, NODE_ENCLOSURE) == 0 )
        {
            const int size = ctxt.items.size();
            for ( int i = 0; attrs[i]; i += 2 )
            {
                const char *name = attrs[i];
                const char *value = attrs[i+1];

                if ( strcmp(name, ATTR_URL) == 0 )
                    ctxt.items[size-1].DownloadURL = value;
                else if ( strcmp(name, ATTR_VERSION) == 0 )
                    ctxt.items[size-1].Version = value;
                else if ( strcmp(name, ATTR_SHORTVERSION) == 0 )
                    ctxt.items[size-1].ShortVersionString = value;
                else if ( strcmp(name, ATTR_OS) == 0 )
                    ctxt.items[size-1].Os = value;
            }
        }
    }
}


void XMLCALL OnEndElement(void *data, const char *name)
{
    ContextData& ctxt = *static_cast<ContextData*>(data);

    if ( ctxt.in_item && strcmp(name, NODE_RELNOTES) == 0 )
    {
        ctxt.in_relnotes--;
    }
    else if ( ctxt.in_item && strcmp(name, NODE_TITLE) == 0 )
    {
        ctxt.in_title--;
    }
    else if ( ctxt.in_item && strcmp(name, NODE_DESCRIPTION) == 0 )
    {
        ctxt.in_description--;
    }
    else if ( ctxt.in_channel && strcmp(name, NODE_ITEM) == 0 )
    {
        ctxt.in_item--;
        if (ctxt.items[ctxt.items.size()-1].Os == OS_MARKER) {
            // we've found os-specific <item>, no need to continue parsing
            XML_StopParser(ctxt.parser, XML_TRUE);
        }
    }
    else if ( strcmp(name, NODE_CHANNEL) == 0 )
    {
        ctxt.in_channel--;
        // we've reached the end of <channel> element,
        // so we stop parsing
        XML_StopParser(ctxt.parser, XML_TRUE);
    }
}


void XMLCALL OnText(void *data, const char *s, int len)
{
    ContextData& ctxt = *static_cast<ContextData*>(data);
    const int size = ctxt.items.size();

    if ( ctxt.in_relnotes )
        ctxt.items[size-1].ReleaseNotesURL.append(s, len);
    else if ( ctxt.in_title )
        ctxt.items[size-1].Title.append(s, len);
    else if ( ctxt.in_description )
        ctxt.items[size-1].Description.append(s, len);
}

bool is_windows_item(const Appcast &item)
{
    return item.Os == OS_MARKER;
}

} // anonymous namespace


/*--------------------------------------------------------------------------*
                               Appcast class
 *--------------------------------------------------------------------------*/

void Appcast::Load(const std::string& xml)
{
    XML_Parser p = XML_ParserCreateNS(NULL, NS_SEP);
    if ( !p )
        throw std::runtime_error("Failed to create XML parser.");

    ContextData ctxt(p);

    XML_SetUserData(p, &ctxt);
    XML_SetElementHandler(p, OnStartElement, OnEndElement);
    XML_SetCharacterDataHandler(p, OnText);

    XML_Status st = XML_Parse(p, xml.c_str(), xml.size(), XML_TRUE);

    if ( st == XML_STATUS_ERROR )
    {
        std::string msg("XML parser error: ");
        msg.append(XML_ErrorString(XML_GetErrorCode(p)));
        XML_ParserFree(p);
        throw std::runtime_error(msg);
    }

    XML_ParserFree(p);

    // Search for first <item> which has "sparkle:os=windows" attribute.
    // If none, use the first item.
    std::vector<Appcast>::iterator it = std::find_if(ctxt.items.begin(), ctxt.items.end(), is_windows_item);
    if (it == ctxt.items.end())
        it = ctxt.items.begin();
    *this = *it;
}

} // namespace winsparkle
