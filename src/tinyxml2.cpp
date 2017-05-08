/*
 TINYXML2.CPP

 Original code by Lee Thomason (www.grinninglizard.com)
 - Modified by Johan Barthelemy

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any
 damages arising from the use of this software.

 Permission is granted to anyone to use this software for any
 purpose, including commercial applications, and to alter it and
 redistribute it freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must
 not claim that you wrote the original software. If you use this
 software in a product, an acknowledgment in the product documentation
 would be appreciated but is not required.

 2. Altered source versions must be plainly marked as such, and
 must not be misrepresented as being the original software.

 3. This notice may not be removed or altered from any source
 distribution.
 */

#include "../include/tinyxml2.hpp"

#include <iostream>

#include <cstdio>
#include <cstdlib>
#include <new>
#include <cstddef>

using namespace tinyxml2;

static const char LINE_FEED = (char) 0x0a;// all line endings are normalized to LF
static const char LF = LINE_FEED;
static const char CARRIAGE_RETURN = (char) 0x0d;			// CR gets filtered out
static const char CR = CARRIAGE_RETURN;
static const char SINGLE_QUOTE = '\'';
static const char DOUBLE_QUOTE = '\"';

// Bunch of unicode info at:
//		http://www.unicode.org/faq/utf_bom.html
//	ef bb bf (Microsoft "lead bytes") - designates UTF-8

static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

#define DELETE_NODE( node )	{			\
	if ( node ) {						\
		MemPool* pool = node->memPool;	\
		node->~XMLNode();				\
		pool->Free( node );				\
	}									\
}
#define DELETE_ATTRIBUTE( attrib ) {		\
	if ( attrib ) {							\
		MemPool* pool = attrib->memPool;	\
		attrib->~XMLAttribute();			\
		pool->Free( attrib );				\
	}										\
}

struct Entity {
  const char* pattern;
  int length;
  char value;
};

static const int NUM_ENTITIES = 5;
static const Entity entities[NUM_ENTITIES] = { { "quot", 4, DOUBLE_QUOTE }, {
    "amp", 3, '&' }, { "apos", 4, SINGLE_QUOTE }, { "lt", 2, '<' }, { "gt", 2,
    '>' } };

StrPair::~StrPair() {
  reset();
}

void StrPair::reset() {
  if (flags & NEEDS_DELETE) {
    delete[] start;
  }
  flags = 0;
  start = 0;
  end = 0;
}

void StrPair::setStr(const char* str, int flags) {
  reset();
  size_t len = strlen(str);
  start = new char[len + 1];
  memcpy(start, str, len + 1);
  end = start + len;
  this->flags = flags | NEEDS_DELETE;
}

char* StrPair::parseText(char* p, const char* endTag, int strFlags) {
  TIXMLASSERT( endTag && *endTag);

  char* start = p;
  char endChar = *endTag;
  size_t length = strlen(endTag);

  // Inner loop of text parsing.
  while (*p) {
    if (*p == endChar && strncmp(p, endTag, length) == 0) {
      Set(start, p, strFlags);
      return p + length;
    }
    ++p;
  }
  return 0;
}

char* StrPair::parseName(char* p) {
  char* start = p;

  if (!start || !(*start)) {
    return 0;
  }

  if (!XMLUtil::IsAlpha(*p)) {
    return 0;
  }

  while (*p
      && (XMLUtil::IsAlphaNum((unsigned char) *p) || *p == '_' || *p == '-'
          || *p == '.' || *p == ':')) {
    ++p;
  }

  if (p > start) {
    Set(start, p, 0);
    return p;
  }
  return 0;
}

const char* StrPair::getStr() {
  if (flags & NEEDS_FLUSH) {
    *end = 0;
    flags ^= NEEDS_FLUSH;

    if (flags) {
      char* p = start;	// the read pointer
      char* q = start;	// the write pointer

      while (p < end) {
        if ((flags & NEEDS_NEWLINE_NORMALIZATION) && *p == CR) {
          // CR-LF pair becomes LF
          // CR alone becomes LF
          // LF-CR becomes LF
          if (*(p + 1) == LF) {
            p += 2;
          } else {
            ++p;
          }
          *q++ = LF;
        } else if ((flags & NEEDS_NEWLINE_NORMALIZATION) && *p == LF) {
          if (*(p + 1) == CR) {
            p += 2;
          } else {
            ++p;
          }
          *q++ = LF;
        } else if ((flags & NEEDS_ENTITY_PROCESSING) && *p == '&') {
          // Entities handled by tinyXML2:
          // - special entities in the entity table [in/out]
          // - numeric character reference [in]
          //   &#20013; or &#x4e2d;

          if (*(p + 1) == '#') {
            char buf[10] = { 0 };
            int len;
            p = const_cast<char*>(XMLUtil::getCharacterRef(p, buf, &len));
            for (int i = 0; i < len; ++i) {
              *q++ = buf[i];
            }
            TIXMLASSERT( q <= p);
          } else {
            int i = 0;
            for (; i < NUM_ENTITIES; ++i) {
              if (strncmp(p + 1, entities[i].pattern, entities[i].length) == 0
                  && *(p + entities[i].length + 1) == ';') {
                // Found an entity convert;
                *q = entities[i].value;
                ++q;
                p += entities[i].length + 2;
                break;
              }
            }
            if (i == NUM_ENTITIES) {
              ++p;
              ++q;
            }
          }
        } else {
          *q = *p;
          ++p;
          ++q;
        }
      }
      *q = 0;
    }
    flags = (flags & NEEDS_DELETE);
  }
  return start;
}

// --------- XMLUtil ----------- //

const char* XMLUtil::readBOM(const char* p, bool* bom) {
  *bom = false;
  const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
  // Check for BOM:
  if (*(pu + 0) == TIXML_UTF_LEAD_0 && *(pu + 1) == TIXML_UTF_LEAD_1
      && *(pu + 2) == TIXML_UTF_LEAD_2) {
    *bom = true;
    p += 3;
  }
  return p;
}

void XMLUtil::convertUTF32ToUTF8(unsigned long input, char* output,
    int* length) {
  const unsigned long BYTE_MASK = 0xBF;
  const unsigned long BYTE_MARK = 0x80;
  const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8,
      0xFC };

  if (input < 0x80)
    *length = 1;
  else if (input < 0x800)
    *length = 2;
  else if (input < 0x10000)
    *length = 3;
  else if (input < 0x200000)
    *length = 4;
  else {
    *length = 0;
    return;
  }	// This code won't covert this correctly anyway.

  output += *length;

  // Scary scary fall throughs.
  switch (*length) {
    case 4:

      --output;
      *output = (char) ((input | BYTE_MARK) & BYTE_MASK);
      input >>= 6;

      // length = 3
      --output;
      *output = (char) ((input | BYTE_MARK) & BYTE_MASK);
      input >>= 6;

      // length = 2
      --output;
      *output = (char) ((input | BYTE_MARK) & BYTE_MASK);
      input >>= 6;

      // length = 1
      --output;
      *output = (char) (input | FIRST_BYTE_MARK[*length]);
      break;

    case 3:

      --output;
      *output = (char) ((input | BYTE_MARK) & BYTE_MASK);
      input >>= 6;

      // length = 2
      --output;
      *output = (char) ((input | BYTE_MARK) & BYTE_MASK);
      input >>= 6;

      // length = 1
      --output;
      *output = (char) (input | FIRST_BYTE_MARK[*length]);
      break;

    case 2:

      --output;
      *output = (char) ((input | BYTE_MARK) & BYTE_MASK);
      input >>= 6;

      // length = 1
      --output;
      *output = (char) (input | FIRST_BYTE_MARK[*length]);
      break;

    case 1:

      --output;
      *output = (char) (input | FIRST_BYTE_MARK[*length]);
      break;

  }

}

const char* XMLUtil::getCharacterRef(const char* p, char* value, int* length) {
  // Presume an entity, and pull it out.
  *length = 0;

  if (*(p + 1) == '#' && *(p + 2)) {
    unsigned long ucs = 0;
    ptrdiff_t delta = 0;
    unsigned mult = 1;

    if (*(p + 2) == 'x') {
      // Hexadecimal.
      if (!*(p + 3))
        return 0;

      const char* q = p + 3;
      q = strchr(q, ';');

      if (!q || !*q)
        return 0;

      delta = q - p;
      --q;

      while (*q != 'x') {
        if (*q >= '0' && *q <= '9')
          ucs += mult * (*q - '0');
        else if (*q >= 'a' && *q <= 'f')
          ucs += mult * (*q - 'a' + 10);
        else if (*q >= 'A' && *q <= 'F')
          ucs += mult * (*q - 'A' + 10);
        else
          return 0;
        mult *= 16;
        --q;
      }
    } else {
      // Decimal.
      if (!*(p + 2))
        return 0;

      const char* q = p + 2;
      q = strchr(q, ';');

      if (!q || !*q)
        return 0;

      delta = q - p;
      --q;

      while (*q != '#') {
        if (*q >= '0' && *q <= '9')
          ucs += mult * (*q - '0');
        else
          return 0;
        mult *= 10;
        --q;
      }
    }
    // convert the UCS to UTF-8
    convertUTF32ToUTF8(ucs, value, length);
    return p + delta + 1;
  }
  return p + 1;
}

void XMLUtil::toStr(int v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%d", v);
}

void XMLUtil::toStr(long v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%ld", v);
}

void XMLUtil::toStr(char v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%c", v);
}

void XMLUtil::toStr(unsigned v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%u", v);
}

void XMLUtil::toStr(bool v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%d", v ? 1 : 0);
}

void XMLUtil::toStr(float v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%f", v);
}

void XMLUtil::toStr(double v, char* buffer, int bufferSize) {
  TIXML_SNPRINTF(buffer, bufferSize, "%lf", v);
}

bool XMLUtil::toInt(const char* str, int* value) {
  if (TIXML_SSCANF(str, "%d", value) == 1)
    return true;
  return false;
}

bool XMLUtil::toUnsigned(const char* str, unsigned *value) {
  if (TIXML_SSCANF(str, "%u", value) == 1)
    return true;
  return false;
}

bool XMLUtil::toLong(const char* str, long *value) {
  if (TIXML_SSCANF(str, "%ld", value) == 1)
    return true;
  return false;
}

bool XMLUtil::toBool(const char* str, bool* value) {
  int ival = 0;
  if (toInt(str, &ival)) {
    *value = (ival == 0) ? false : true;
    return true;
  }
  if (StringEqual(str, "true")) {
    *value = true;
    return true;
  } else if (StringEqual(str, "false")) {
    *value = false;
    return true;
  }
  return false;
}

bool XMLUtil::toFloat(const char* str, float* value) {
  if (TIXML_SSCANF(str, "%f", value) == 1) {
    return true;
  }
  return false;
}

bool XMLUtil::toDouble(const char* str, double* value) {
  if (TIXML_SSCANF(str, "%lf", value) == 1) {
    return true;
  }
  return false;
}

bool XMLUtil::toChar(const char* str, char* value) {
  if (TIXML_SSCANF(str, "%c", value) == 1) {
    return true;
  }
  return false;
}

bool XMLUtil::toString(const char* str, std::string & value) {

	value = str;
	return true;

}

char* XMLDocument::identify(char* p, XMLNode** node) {
  XMLNode* returnNode = 0;
  char* start = p;
  p = XMLUtil::SkipWhiteSpace(p);
  if (!p || !*p) {
    return p;
  }

  // What is this thing? 
  // - Elements start with a letter or underscore, but xml is reserved.
  // - Comments: <!--
  // - Decleration: <?
  // - Everthing else is unknown to tinyxml.
  //

  static const char* xmlHeader = { "<?" };
  static const char* commentHeader = { "<!--" };
  static const char* dtdHeader = { "<!" };
  static const char* cdataHeader = { "<![CDATA[" };
  static const char* elementHeader = { "<" };	// and a header for everything else; check last.

  static const int xmlHeaderLen = 2;
  static const int commentHeaderLen = 4;
  static const int dtdHeaderLen = 2;
  static const int cdataHeaderLen = 9;
  static const int elementHeaderLen = 1;

#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4127 )
#endif
  TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLUnknown ));
  // use same memory pool
  TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLDeclaration ));
  // use same memory pool
#if defined(_MSC_VER)
#pragma warning (pop)
#endif
  if (XMLUtil::StringEqual(p, xmlHeader, xmlHeaderLen)) {
    returnNode = new (commentPool.Alloc()) XMLDeclaration(this);
    returnNode->memPool = &commentPool;
    p += xmlHeaderLen;
  } else if (XMLUtil::StringEqual(p, commentHeader, commentHeaderLen)) {
    returnNode = new (commentPool.Alloc()) XMLComment(this);
    returnNode->memPool = &commentPool;
    p += commentHeaderLen;
  } else if (XMLUtil::StringEqual(p, cdataHeader, cdataHeaderLen)) {
    XMLText* text = new (textPool.Alloc()) XMLText(this);
    returnNode = text;
    returnNode->memPool = &textPool;
    p += cdataHeaderLen;
    text->SetCData(true);
  } else if (XMLUtil::StringEqual(p, dtdHeader, dtdHeaderLen)) {
    returnNode = new (commentPool.Alloc()) XMLUnknown(this);
    returnNode->memPool = &commentPool;
    p += dtdHeaderLen;
  } else if (XMLUtil::StringEqual(p, elementHeader, elementHeaderLen)) {
    returnNode = new (elementPool.Alloc()) XMLElement(this);
    returnNode->memPool = &elementPool;
    p += elementHeaderLen;
  } else {
    returnNode = new (textPool.Alloc()) XMLText(this);
    returnNode->memPool = &textPool;
    p = start;	// Back it up, all the text counts.
  }

  *node = returnNode;
  return p;
}

bool XMLDocument::accept(XMLVisitor* visitor) const {
  if (visitor->visitEnter(*this)) {
    for (const XMLNode* node = FirstChild(); node; node = node->NextSibling()) {
      if (!node->accept(visitor))
        break;
    }
  }
  return visitor->VisitExit(*this);
}

// --------- XMLNode ----------- //

XMLNode::XMLNode(XMLDocument* doc) :
    document(doc), parent(0), firstChild(0), lastChild(0), prev(0), next(0), memPool() {
}

XMLNode::~XMLNode() {
  deleteChildren();
  if (parent) {
    parent->unlink(this);
  }
}

void XMLNode::setValue(const char* str, bool staticMem) {
  if (staticMem)
    value.SetInternedStr(str);
  else
    value.setStr(str);
}

void XMLNode::deleteChildren() {
  while (firstChild) {
    XMLNode* node = firstChild;
    unlink(node);

    DELETE_NODE( node);
  }
  firstChild = lastChild = 0;
}

void XMLNode::unlink(XMLNode* child) {
  TIXMLASSERT( child->parent == this);
  if (child == firstChild)
    firstChild = firstChild->next;
  if (child == lastChild)
    lastChild = lastChild->prev;

  if (child->prev) {
    child->prev->next = child->next;
  }
  if (child->next) {
    child->next->prev = child->prev;
  }
  child->parent = 0;
}

void XMLNode::deleteChild(XMLNode* node) {
  TIXMLASSERT( node->parent == this);
  DELETE_NODE( node);
}

XMLNode* XMLNode::insertEndChild(XMLNode* addThis) {
  if (lastChild) {
    TIXMLASSERT( firstChild);
    TIXMLASSERT( lastChild->next == 0);
    lastChild->next = addThis;
    addThis->prev = lastChild;
    lastChild = addThis;

    addThis->next = 0;
  } else {
    TIXMLASSERT( firstChild == 0);
    firstChild = lastChild = addThis;

    addThis->prev = 0;
    addThis->next = 0;
  }
  addThis->parent = this;
  return addThis;
}

XMLNode* XMLNode::insertFirstChild(XMLNode* addThis) {
  if (firstChild) {
    TIXMLASSERT( lastChild);
    TIXMLASSERT( firstChild->prev == 0);

    firstChild->prev = addThis;
    addThis->next = firstChild;
    firstChild = addThis;

    addThis->prev = 0;
  } else {
    TIXMLASSERT( lastChild == 0);
    firstChild = lastChild = addThis;

    addThis->prev = 0;
    addThis->next = 0;
  }
  addThis->parent = this;
  return addThis;
}

XMLNode* XMLNode::insertAfterChild(XMLNode* afterThis, XMLNode* addThis) {
  TIXMLASSERT( afterThis->parent == this);
  if (afterThis->parent != this)
    return 0;

  if (afterThis->next == 0) {
    // The last node or the only node.
    return insertEndChild(addThis);
  }
  addThis->prev = afterThis;
  addThis->next = afterThis->next;
  afterThis->next->prev = addThis;
  afterThis->next = addThis;
  addThis->parent = this;
  return addThis;
}

const XMLElement* XMLNode::firstChildElement(const char* value) const {
  for (XMLNode* node = firstChild; node; node = node->next) {
    XMLElement* element = node->ToElement();
    if (element) {
      if (!value || XMLUtil::StringEqual(element->Name(), value)) {
        return element;
      }
    }
  }
  return 0;
}

const XMLElement* XMLNode::lastChildElement(const char* value) const {
  for (XMLNode* node = lastChild; node; node = node->prev) {
    XMLElement* element = node->ToElement();
    if (element) {
      if (!value || XMLUtil::StringEqual(element->Name(), value)) {
        return element;
      }
    }
  }
  return 0;
}

const XMLElement* XMLNode::nextSiblingElement(const char* value) const {
  for (XMLNode* element = this->next; element; element = element->next) {
    if (element->ToElement()
        && (!value || XMLUtil::StringEqual(value, element->Value()))) {
      return element->ToElement();
    }
  }
  return 0;
}

const XMLElement* XMLNode::previousSiblingElement(const char* value) const {
  for (XMLNode* element = this->prev; element; element = element->prev) {
    if (element->ToElement()
        && (!value || XMLUtil::StringEqual(value, element->Value()))) {
      return element->ToElement();
    }
  }
  return 0;
}

char* XMLNode::parseDeep(char* p, StrPair* parentEnd) {
  // This is a recursive method, but thinking about it "at the current level"
  // it is a pretty simple flat list:
  //		<foo/>
  //		<!-- comment -->
  //
  // With a special case:
  //		<foo>
  //		</foo>
  //		<!-- comment -->
  //		
  // Where the closing element (/foo) *must* be the next thing after the opening
  // element, and the names must match. BUT the tricky bit is that the closing
  // element will be read by the child.
  // 
  // 'endTag' is the end tag for this node, it is returned by a call to a child.
  // 'parentEnd' is the end tag for the parent, which is filled in and returned.

  while (p && *p) {
    XMLNode* node = 0;

    p = document->identify(p, &node);
    if (p == 0 || node == 0) {
      break;
    }

    StrPair endTag;
    p = node->parseDeep(p, &endTag);
    if (!p) {
      DELETE_NODE( node);
      node = 0;
      if (!document->Error()) {
        document->setError(XML_ERROR_PARSING, 0, 0);
      }
      break;
    }

    // We read the end tag. Return it to the parent.
    if (node->ToElement()
        && node->ToElement()->ClosingType() == XMLElement::CLOSING) {
      if (parentEnd) {
        *parentEnd = static_cast<XMLElement*>(node)->value;
      }
      DELETE_NODE( node);
      return p;
    }

    // Handle an end tag returned to this level.
    // And handle a bunch of annoying errors.
    XMLElement* ele = node->ToElement();
    if (ele) {
      if (endTag.Empty() && ele->ClosingType() == XMLElement::OPEN) {
        document->setError(XML_ERROR_MISMATCHED_ELEMENT, node->Value(), 0);
        p = 0;
      } else if (!endTag.Empty() && ele->ClosingType() != XMLElement::OPEN) {
        document->setError(XML_ERROR_MISMATCHED_ELEMENT, node->Value(), 0);
        p = 0;
      } else if (!endTag.Empty()) {
        if (!XMLUtil::StringEqual(endTag.getStr(), node->Value())) {
          document->setError(XML_ERROR_MISMATCHED_ELEMENT, node->Value(), 0);
          p = 0;
        }
      }
    }
    if (p == 0) {
      DELETE_NODE( node);
      node = 0;
    }
    if (node) {
      this->insertEndChild(node);
    }
  }
  return 0;
}

// --------- XMLText ---------- //
char* XMLText::parseDeep(char* p, StrPair*) {
  const char* start = p;
  if (this->CData()) {
    p = value.parseText(p, "]]>", StrPair::NEEDS_NEWLINE_NORMALIZATION);
    if (!p) {
      document->setError(XML_ERROR_PARSING_CDATA, start, 0);
    }
    return p;
  } else {
    p = value.parseText(p, "<",
        document->ProcessEntities() ?
            StrPair::TEXT_ELEMENT : StrPair::TEXT_ELEMENT_LEAVE_ENTITIES);
    if (!p) {
      document->setError(XML_ERROR_PARSING_TEXT, start, 0);
    }
    if (p && *p) {
      return p - 1;
    }
  }
  return 0;
}

XMLNode* XMLText::shallowClone(XMLDocument* doc) const {
  if (!doc) {
    doc = document;
  }
  XMLText* text = doc->newText(Value());
  text->SetCData(this->CData());
  return text;
}

bool XMLText::shallowEqual(const XMLNode* compare) const {
  return (compare->ToText()
      && XMLUtil::StringEqual(compare->ToText()->Value(), Value()));
}

bool XMLText::accept(XMLVisitor* visitor) const {
  return visitor->visit(*this);
}

// --------- XMLComment ---------- //

XMLComment::XMLComment(XMLDocument* doc) :
    XMLNode(doc) {
}

XMLComment::~XMLComment() {
  //printf( "~XMLComment\n" );
}

char* XMLComment::parseDeep(char* p, StrPair*) {
  // Comment parses as text.
  const char* start = p;
  p = value.parseText(p, "-->", StrPair::COMMENT);
  if (p == 0) {
    document->setError(XML_ERROR_PARSING_COMMENT, start, 0);
  }
  return p;
}

XMLNode* XMLComment::shallowClone(XMLDocument* doc) const {
  if (!doc) {
    doc = document;
  }
  XMLComment* comment = doc->newComment(Value());
  return comment;
}

bool XMLComment::shallowEqual(const XMLNode* compare) const {
  return (compare->ToComment()
      && XMLUtil::StringEqual(compare->ToComment()->Value(), Value()));
}

bool XMLComment::accept(XMLVisitor* visitor) const {
  return visitor->visit(*this);
}

// --------- XMLDeclaration ---------- //

XMLDeclaration::XMLDeclaration(XMLDocument* doc) :
    XMLNode(doc) {
}

XMLDeclaration::~XMLDeclaration() {
  //printf( "~XMLDeclaration\n" );
}

char* XMLDeclaration::parseDeep(char* p, StrPair*) {
  // Declaration parses as text.
  const char* start = p;
  p = value.parseText(p, "?>", StrPair::NEEDS_NEWLINE_NORMALIZATION);
  if (p == 0) {
    document->setError(XML_ERROR_PARSING_DECLARATION, start, 0);
  }
  return p;
}

XMLNode* XMLDeclaration::shallowClone(XMLDocument* doc) const {
  if (!doc) {
    doc = document;
  }
  XMLDeclaration* dec = doc->newDeclaration(Value());
  return dec;
}

bool XMLDeclaration::shallowEqual(const XMLNode* compare) const {
  return (compare->ToDeclaration()
      && XMLUtil::StringEqual(compare->ToDeclaration()->Value(), Value()));
}

bool XMLDeclaration::accept(XMLVisitor* visitor) const {
  return visitor->visit(*this);
}

// --------- XMLUnknown ---------- //

XMLUnknown::XMLUnknown(XMLDocument* doc) :
    XMLNode(doc) {
}

XMLUnknown::~XMLUnknown() {
}

char* XMLUnknown::parseDeep(char* p, StrPair*) {
  // Unknown parses as text.
  const char* start = p;

  p = value.parseText(p, ">", StrPair::NEEDS_NEWLINE_NORMALIZATION);
  if (!p) {
    document->setError(XML_ERROR_PARSING_UNKNOWN, start, 0);
  }
  return p;
}

XMLNode* XMLUnknown::shallowClone(XMLDocument* doc) const {
  if (!doc) {
    doc = document;
  }
  XMLUnknown* text = doc->newUnknown(Value());
  return text;
}

bool XMLUnknown::shallowEqual(const XMLNode* compare) const {
  return (compare->ToUnknown()
      && XMLUtil::StringEqual(compare->ToUnknown()->Value(), Value()));
}

bool XMLUnknown::accept(XMLVisitor* visitor) const {
  return visitor->visit(*this);
}

// --------- XMLAttribute ---------- //
char* XMLAttribute::parseDeep(char* p, bool processEntities) {
  // Parse using the name rules: bug fix, was using ParseText before
  p = name.parseName(p);
  if (!p || !*p)
    return 0;

  // Skip white space before =
  p = XMLUtil::SkipWhiteSpace(p);
  if (!p || *p != '=')
    return 0;

  ++p;	// move up to opening quote
  p = XMLUtil::SkipWhiteSpace(p);
  if (*p != '\"' && *p != '\'')
    return 0;

  char endTag[2] = { *p, 0 };
  ++p;	// move past opening quote

  p = value.parseText(p, endTag,
      processEntities ?
          StrPair::ATTRIBUTE_VALUE : StrPair::ATTRIBUTE_VALUE_LEAVE_ENTITIES);
  return p;
}

void XMLAttribute::setName(const char* n) {
  name.setStr(n);
}

int XMLAttribute::queryIntValue(int* value) const {
  if (XMLUtil::toInt(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}

int XMLAttribute::queryUnsignedValue(unsigned int* value) const {
  if (XMLUtil::toUnsigned(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}

int XMLAttribute::queryLongValue(long* value) const {
  if (XMLUtil::toLong(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}


int XMLAttribute::queryBoolValue(bool* value) const {
  if (XMLUtil::toBool(Value(), value)) {
    return XML_NO_ERROR;
  }
  return XML_WRONG_ATTRIBUTE_TYPE;
}

int XMLAttribute::queryFloatValue(float* value) const {
  if (XMLUtil::toFloat(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}

int XMLAttribute::queryDoubleValue(double* value) const {
  if (XMLUtil::toDouble(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}

int XMLAttribute::queryCharValue(char* value) const {
  if (XMLUtil::toChar(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}

int XMLAttribute::queryStringValue(std::string & value) const {
  if (XMLUtil::toString(Value(), value))
    return XML_NO_ERROR;
  return XML_WRONG_ATTRIBUTE_TYPE;
}


void XMLAttribute::setAttribute(const char* v) {
  value.setStr(v);
}

//ERREUR ICI? Plus dans .hpp il n y a pas de setattribute pour des char!
void XMLAttribute::setAttribute( char v ) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}

void XMLAttribute::setAttribute(int v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}

void XMLAttribute::setAttribute(long v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}

void XMLAttribute::setAttribute(unsigned v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}

void XMLAttribute::setAttribute(bool v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}

void XMLAttribute::setAttribute(double v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}

void XMLAttribute::setAttribute(float v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  value.setStr(buf);
}



// --------- XMLElement ---------- //
XMLElement::XMLElement(XMLDocument* doc) :
    XMLNode(doc), closingType(0), rootAttribute(0) {
}

XMLElement::~XMLElement() {
  while (rootAttribute) {
    XMLAttribute* next = rootAttribute->next;
    DELETE_ATTRIBUTE( rootAttribute);
    rootAttribute = next;
  }
}

XMLAttribute* XMLElement::findAttribute(const char* name) {
  XMLAttribute* a = 0;
  for (a = rootAttribute; a; a = a->next) {
    if (XMLUtil::StringEqual(a->Name(), name))
      return a;
  }
  return 0;
}

const XMLAttribute* XMLElement::findAttribute(const char* name) const {
  XMLAttribute* a = 0;
  for (a = rootAttribute; a; a = a->next) {
    if (XMLUtil::StringEqual(a->Name(), name))
      return a;
  }
  return 0;
}

const char* XMLElement::attribute(const char* name, const char* value) const {
  const XMLAttribute* a = findAttribute(name);
  if (!a)
    return 0;
  if (!value || XMLUtil::StringEqual(a->Value(), value))
    return a->Value();
  return 0;
}

const char* XMLElement::getText() const {
  if (FirstChild() && FirstChild()->ToText()) {
    return FirstChild()->ToText()->Value();
  }
  return 0;
}

int XMLElement::queryIntText(int* _value) const {
  if (FirstChild() && FirstChild()->ToText()) {
    const char* t = FirstChild()->ToText()->Value();
    if (XMLUtil::toInt(t, _value)) {
      return XML_SUCCESS;
    }
    return XML_CAN_NOT_CONVERT_TEXT;
  }
  return XML_NO_TEXT_NODE;
}

int XMLElement::queryUnsignedText(unsigned* _value) const {
  if (FirstChild() && FirstChild()->ToText()) {
    const char* t = FirstChild()->ToText()->Value();
    if (XMLUtil::toUnsigned(t, _value)) {
      return XML_SUCCESS;
    }
    return XML_CAN_NOT_CONVERT_TEXT;
  }
  return XML_NO_TEXT_NODE;
}

int XMLElement::queryBoolText(bool* _value) const {
  if (FirstChild() && FirstChild()->ToText()) {
    const char* t = FirstChild()->ToText()->Value();
    if (XMLUtil::toBool(t, _value)) {
      return XML_SUCCESS;
    }
    return XML_CAN_NOT_CONVERT_TEXT;
  }
  return XML_NO_TEXT_NODE;
}

int XMLElement::queryDoubleText(double* _value) const {
  if (FirstChild() && FirstChild()->ToText()) {
    const char* t = FirstChild()->ToText()->Value();
    if (XMLUtil::toDouble(t, _value)) {
      return XML_SUCCESS;
    }
    return XML_CAN_NOT_CONVERT_TEXT;
  }
  return XML_NO_TEXT_NODE;
}

int XMLElement::queryFloatText(float* _value) const {
  if (FirstChild() && FirstChild()->ToText()) {
    const char* t = FirstChild()->ToText()->Value();
    if (XMLUtil::toFloat(t, _value)) {
      return XML_SUCCESS;
    }
    return XML_CAN_NOT_CONVERT_TEXT;
  }
  return XML_NO_TEXT_NODE;
}

XMLAttribute* XMLElement::findOrCreateAttribute(const char* name) {
  XMLAttribute* last = 0;
  XMLAttribute* attrib = 0;
  for (attrib = rootAttribute; attrib; last = attrib, attrib = attrib->next) {
    if (XMLUtil::StringEqual(attrib->Name(), name)) {
      break;
    }
  }
  if (!attrib) {
    attrib = new (document->attributePool.Alloc()) XMLAttribute();
    attrib->memPool = &document->attributePool;
    if (last) {
      last->next = attrib;
    } else {
      rootAttribute = attrib;
    }
    attrib->setName(name);
  }
  return attrib;
}

void XMLElement::deleteAttribute(const char* name) {
  XMLAttribute* prev = 0;
  for (XMLAttribute* a = rootAttribute; a; a = a->next) {
    if (XMLUtil::StringEqual(name, a->Name())) {
      if (prev) {
        prev->next = a->next;
      } else {
        rootAttribute = a->next;
      }
      DELETE_ATTRIBUTE( a);
      break;
    }
    prev = a;
  }
}

char* XMLElement::parseAttributes(char* p) {
  const char* start = p;
  XMLAttribute* prevAttribute = 0;

  // Read the attributes.
  while (p) {
    p = XMLUtil::SkipWhiteSpace(p);
    if (!p || !(*p)) {
      document->setError(XML_ERROR_PARSING_ELEMENT, start, Name());
      return 0;
    }

    // attribute.
    if (XMLUtil::IsAlpha(*p)) {
      XMLAttribute* attrib =
          new (document->attributePool.Alloc()) XMLAttribute();
      attrib->memPool = &document->attributePool;

      p = attrib->parseDeep(p, document->ProcessEntities());
      if (!p || attribute(attrib->Name())) {
        DELETE_ATTRIBUTE( attrib);
        document->setError(XML_ERROR_PARSING_ATTRIBUTE, start, p);
        return 0;
      }
      // There is a minor bug here: if the attribute in the source xml
      // document is duplicated, it will not be detected and the
      // attribute will be doubly added. However, tracking the 'prevAttribute'
      // avoids re-scanning the attribute list. Preferring performance for
      // now, may reconsider in the future.
      if (prevAttribute) {
        prevAttribute->next = attrib;
      } else {
        rootAttribute = attrib;
      }
      prevAttribute = attrib;
    }
    // end of the tag
    else if (*p == '/' && *(p + 1) == '>') {
      closingType = CLOSED;
      return p + 2;	// done; sealed element.
    }
    // end of the tag
    else if (*p == '>') {
      ++p;
      break;
    } else {
      document->setError(XML_ERROR_PARSING_ELEMENT, start, p);
      return 0;
    }
  }
  return p;
}

//
//	<ele></ele>
//	<ele>foo<b>bar</b></ele>
//
char* XMLElement::parseDeep(char* p, StrPair* strPair) {
  // Read the element name.
  p = XMLUtil::SkipWhiteSpace(p);
  if (!p)
    return 0;

  // The closing element is the </element> form. It is
  // parsed just like a regular element then deleted from
  // the DOM.
  if (*p == '/') {
    closingType = CLOSING;
    ++p;
  }

  p = value.parseName(p);
  if (value.Empty())
    return 0;

  p = parseAttributes(p);
  if (!p || !*p || closingType)
    return p;

  p = XMLNode::parseDeep(p, strPair);
  return p;
}

XMLNode* XMLElement::shallowClone(XMLDocument* doc) const {
  if (!doc) {
    doc = document;
  }
  XMLElement* element = doc->newElement(Value());
  for (const XMLAttribute* a = FirstAttribute(); a; a = a->Next()) {
    element->SetAttribute(a->Name(), a->Value());
  }
  return element;
}

bool XMLElement::shallowEqual(const XMLNode* compare) const {
  const XMLElement* other = compare->ToElement();
  if (other && XMLUtil::StringEqual(other->Value(), Value())) {

    const XMLAttribute* a = FirstAttribute();
    const XMLAttribute* b = other->FirstAttribute();

    while (a && b) {
      if (!XMLUtil::StringEqual(a->Value(), b->Value())) {
        return false;
      }
      a = a->Next();
      b = b->Next();
    }
    if (a || b) {
      // different count
      return false;
    }
    return true;
  }
  return false;
}

bool XMLElement::accept(XMLVisitor* visitor) const {
  if (visitor->visitEnter(*this, rootAttribute)) {
    for (const XMLNode* node = FirstChild(); node; node = node->NextSibling()) {
      if (!node->accept(visitor))
        break;
    }
  }
  return visitor->visitExit(*this);
}

// --------- XMLDocument ----------- //
XMLDocument::XMLDocument(bool _processEntities) :
    XMLNode(0), writeBOM(false), processEntities(_processEntities), errorID(0), errorStr1(
        0), errorStr2(0), charBuffer(0) {
  document = this;	// avoid warning about 'this' in initializer list
}

XMLDocument::~XMLDocument() {
  deleteChildren();
  delete[] charBuffer;

#if 0
  textPool.Trace( "text" );
  elementPool.Trace( "element" );
  commentPool.Trace( "comment" );
  attributePool.Trace( "attribute" );
#endif

  TIXMLASSERT( textPool.CurrentAllocs() == 0);
  TIXMLASSERT( elementPool.CurrentAllocs() == 0);
  TIXMLASSERT( commentPool.CurrentAllocs() == 0);
  TIXMLASSERT( attributePool.CurrentAllocs() == 0);
}

void XMLDocument::initDocument() {
  errorID = XML_NO_ERROR;
  errorStr1 = 0;
  errorStr2 = 0;

  delete[] charBuffer;
  charBuffer = 0;

}

XMLElement* XMLDocument::newElement(const char* name) {
  XMLElement* ele = new (elementPool.Alloc()) XMLElement(this);
  ele->memPool = &elementPool;
  ele->SetName(name);
  return ele;
}

XMLComment* XMLDocument::newComment(const char* str) {
  XMLComment* comment = new (commentPool.Alloc()) XMLComment(this);
  comment->memPool = &commentPool;
  comment->setValue(str);
  return comment;
}

XMLText* XMLDocument::newText(const char* str) {
  XMLText* text = new (textPool.Alloc()) XMLText(this);
  text->memPool = &textPool;
  text->setValue(str);
  return text;
}

XMLDeclaration* XMLDocument::newDeclaration(const char* str) {
  XMLDeclaration* dec = new (commentPool.Alloc()) XMLDeclaration(this);
  dec->memPool = &commentPool;
  dec->setValue(str ? str : "xml version=\"1.0\" encoding=\"UTF-8\"");
  return dec;
}

XMLUnknown* XMLDocument::newUnknown(const char* str) {
  XMLUnknown* unk = new (commentPool.Alloc()) XMLUnknown(this);
  unk->memPool = &commentPool;
  unk->setValue(str);
  return unk;
}

int XMLDocument::loadFile(const char* filename) {
  deleteChildren();
  initDocument();

#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4996 )		// Fail to see a compelling reason why this should be deprecated.
#endif
  FILE* fp = fopen(filename, "rb");
#if defined(_MSC_VER)
#pragma warning ( pop )
#endif
  if (!fp) {
    setError(XML_ERROR_FILE_NOT_FOUND, filename, 0);
    return errorID;
  }
  loadFile(fp);
  fclose(fp);
  return errorID;
}

int XMLDocument::loadFile(FILE* fp) {
  deleteChildren();
  initDocument();

  fseek(fp, 0, SEEK_END);
  unsigned size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size == 0) {
    return errorID;
  }

  charBuffer = new char[size + 1];
  size_t read = fread(charBuffer, 1, size, fp);
  if (read != size) {
    setError(XML_ERROR_FILE_READ_ERROR, 0, 0);
    return errorID;
  }

  charBuffer[size] = 0;

  const char* p = charBuffer;
  p = XMLUtil::SkipWhiteSpace(p);
  p = XMLUtil::readBOM(p, &writeBOM);
  if (!p || !*p) {
    setError(XML_ERROR_EMPTY_DOCUMENT, 0, 0);
    return errorID;
  }

  parseDeep(charBuffer + (p - charBuffer), 0);
  return errorID;
}

int XMLDocument::saveFile(const char* filename) {
#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4996 )		// Fail to see a compelling reason why this should be deprecated.
#endif
  FILE* fp = fopen(filename, "w");
#if defined(_MSC_VER)
#pragma warning ( pop )
#endif
  if (!fp) {
    setError(XML_ERROR_FILE_COULD_NOT_BE_OPENED, filename, 0);
    return errorID;
  }
  saveFile(fp);
  fclose(fp);
  return errorID;
}

int XMLDocument::saveFile(FILE* fp) {
  XMLPrinter stream(fp);
  print(&stream);
  return errorID;
}

int XMLDocument::parse(const char* p) {
  deleteChildren();
  initDocument();

  if (!p || !*p) {
    setError(XML_ERROR_EMPTY_DOCUMENT, 0, 0);
    return errorID;
  }
  p = XMLUtil::SkipWhiteSpace(p);
  p = XMLUtil::readBOM(p, &writeBOM);
  if (!p || !*p) {
    setError(XML_ERROR_EMPTY_DOCUMENT, 0, 0);
    return errorID;
  }

  size_t len = strlen(p);
  charBuffer = new char[len + 1];
  memcpy(charBuffer, p, len + 1);

  parseDeep(charBuffer, 0);
  return errorID;
}

void XMLDocument::print(XMLPrinter* streamer) {
  XMLPrinter stdStreamer(stdout);
  if (!streamer)
    streamer = &stdStreamer;
  accept(streamer);
}

void XMLDocument::setError(int error, const char* str1, const char* str2) {
  errorID = error;
  errorStr1 = str1;
  errorStr2 = str2;
}

void XMLDocument::printError() const {
  if (errorID) {
    static const int LEN = 20;
    char buf1[LEN] = { 0 };
    char buf2[LEN] = { 0 };

    if (errorStr1) {
      TIXML_SNPRINTF(buf1, LEN, "%s", errorStr1);
    }
    if (errorStr2) {
      TIXML_SNPRINTF(buf2, LEN, "%s", errorStr2);
    }

    printf("XMLDocument error id=%d str1=%s str2=%s\n", errorID, buf1, buf2);
  }
}

XMLPrinter::XMLPrinter(FILE* file, bool compact) :
    elementJustOpened(false), firstElement(true), fp(file), depth(0), textDepth(
        -1), processEntities(true), compactMode(compact) {
  for (int i = 0; i < ENTITY_RANGE; ++i) {
    entityFlag[i] = false;
    restrictedEntityFlag[i] = false;
  }
  for (int i = 0; i < NUM_ENTITIES; ++i) {
    TIXMLASSERT( entities[i].value < ENTITY_RANGE);
    if (entities[i].value < ENTITY_RANGE) {
      entityFlag[(int) entities[i].value] = true;
    }
  }
  restrictedEntityFlag[(int) '&'] = true;
  restrictedEntityFlag[(int) '<'] = true;
  restrictedEntityFlag[(int) '>'] = true;	// not required, but consistency is nice
  buffer.Push(0);
}

void XMLPrinter::print(const char* format, ...) {
  va_list va;
  va_start(va, format);

  if (fp) {
    vfprintf(fp, format, va);
  } else {
    // This seems brutally complex. Haven't figured out a better
    // way on windows.
#ifdef _MSC_VER
    int len = -1;
    int expand = 1000;
    while ( len < 0 ) {
      len = vsnprintf_s( accumulator.Mem(), accumulator.Capacity(), _TRUNCATE, format, va );
      if ( len < 0 ) {
        expand *= 3/2;
        accumulator.PushArr( expand );
      }
    }
    char* p = buffer.PushArr( len ) - 1;
    memcpy( p, accumulator.Mem(), len+1 );
#else
    int len = vsnprintf(0, 0, format, va);
    // Close out and re-start the va-args
    va_end(va);
    va_start(va, format);
    char* p = buffer.PushArr(len) - 1;
    vsnprintf(p, len + 1, format, va);
#endif
  }
  va_end(va);
}

void XMLPrinter::printSpace(int depth) {
  for (int i = 0; i < depth; ++i) {
    print("    ");
  }
}

void XMLPrinter::printString(const char* p, bool restricted) {
  // Look for runs of bytes between entities to print.
  const char* q = p;
  const bool* flag = restricted ? restrictedEntityFlag : entityFlag;

  if (processEntities) {
    while (*q) {
      // Remember, char is sometimes signed. (How many times has that bitten me?)
      if (*q > 0 && *q < ENTITY_RANGE) {
        // Check for entities. If one is found, flush
        // the stream up until the entity, write the 
        // entity, and keep looking.
        if (flag[(unsigned) (*q)]) {
          while (p < q) {
            print("%c", *p);
            ++p;
          }
          for (int i = 0; i < NUM_ENTITIES; ++i) {
            if (entities[i].value == *q) {
              print("&%s;", entities[i].pattern);
              break;
            }
          }
          ++p;
        }
      }
      ++q;
    }
  }
  // Flush the remaining string. This will be the entire
  // string if an entity wasn't found.
  if (!processEntities || (q - p > 0)) {
    print("%s", p);
  }
}

void XMLPrinter::pushHeader(bool writeBOM, bool writeDec) {
  static const unsigned char bom[] = { TIXML_UTF_LEAD_0, TIXML_UTF_LEAD_1,
      TIXML_UTF_LEAD_2, 0 };
  if (writeBOM) {
    print("%s", bom);
  }
  if (writeDec) {
    pushDeclaration("xml version=\"1.0\"");
  }
}

void XMLPrinter::openElement(const char* name) {
  if (elementJustOpened) {
    sealElement();
  }
  stack.Push(name);

  if (textDepth < 0 && !firstElement && !compactMode) {
    print("\n");
    printSpace(depth);
  }

  print("<%s", name);
  elementJustOpened = true;
  firstElement = false;
  ++depth;
}

void XMLPrinter::pushAttribute(const char* name, const char* value) {
  TIXMLASSERT( elementJustOpened);
  print(" %s=\"", name);
  printString(value, false);
  print("\"");
}

void XMLPrinter::pushAttribute(const char* name, int v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  pushAttribute(name, buf);
}

void XMLPrinter::pushAttribute(const char* name, unsigned v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  pushAttribute(name, buf);
}

void XMLPrinter::pushAttribute(const char* name, bool v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  pushAttribute(name, buf);
}

void XMLPrinter::pushAttribute(const char* name, double v) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(v, buf, BUF_SIZE);
  pushAttribute(name, buf);
}

void XMLPrinter::closeElement() {
  --depth;
  const char* name = stack.Pop();

  if (elementJustOpened) {
    print("/>");
  } else {
    if (textDepth < 0 && !compactMode) {
      print("\n");
      printSpace(depth);
    }
    print("</%s>", name);
  }

  if (textDepth == depth)
    textDepth = -1;
  if (depth == 0 && !compactMode)
    print("\n");
  elementJustOpened = false;
}

void XMLPrinter::sealElement() {
  elementJustOpened = false;
  print(">");
}

void XMLPrinter::pushText(const char* text, bool cdata) {
  textDepth = depth - 1;

  if (elementJustOpened) {
    sealElement();
  }
  if (cdata) {
    print("<![CDATA[");
    print("%s", text);
    print("]]>");
  } else {
    printString(text, true);
  }
}

void XMLPrinter::pushText(int value) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(value, buf, BUF_SIZE);
  pushText(buf, false);
}

void XMLPrinter::pushText(unsigned value) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(value, buf, BUF_SIZE);
  pushText(buf, false);
}

void XMLPrinter::pushText(bool value) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(value, buf, BUF_SIZE);
  pushText(buf, false);
}

void XMLPrinter::pushText(float value) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(value, buf, BUF_SIZE);
  pushText(buf, false);
}

void XMLPrinter::pushText(double value) {
  char buf[BUF_SIZE];
  XMLUtil::toStr(value, buf, BUF_SIZE);
  pushText(buf, false);
}

void XMLPrinter::pushComment(const char* comment) {
  if (elementJustOpened) {
    sealElement();
  }
  if (textDepth < 0 && !firstElement && !compactMode) {
    print("\n");
    printSpace(depth);
  }
  firstElement = false;
  print("<!--%s-->", comment);
}

void XMLPrinter::pushDeclaration(const char* value) {
  if (elementJustOpened) {
    sealElement();
  }
  if (textDepth < 0 && !firstElement && !compactMode) {
    print("\n");
    printSpace(depth);
  }
  firstElement = false;
  print("<?%s?>", value);
}

void XMLPrinter::pushUnknown(const char* value) {
  if (elementJustOpened) {
    sealElement();
  }
  if (textDepth < 0 && !firstElement && !compactMode) {
    print("\n");
    printSpace(depth);
  }
  firstElement = false;
  print("<!%s>", value);
}

bool XMLPrinter::visitEnter(const XMLDocument& doc) {
  processEntities = doc.ProcessEntities();
  if (doc.HasBOM()) {
    pushHeader(true, false);
  }
  return true;
}

bool XMLPrinter::visitEnter(const XMLElement& element,
    const XMLAttribute* attribute) {
  openElement(element.Name());
  while (attribute) {
    pushAttribute(attribute->Name(), attribute->Value());
    attribute = attribute->Next();
  }
  return true;
}

bool XMLPrinter::visitExit(const XMLElement&) {
  closeElement();
  return true;
}

bool XMLPrinter::visit(const XMLText& text) {
  pushText(text.Value(), text.CData());
  return true;
}

bool XMLPrinter::visit(const XMLComment& comment) {
  pushComment(comment.Value());
  return true;
}

bool XMLPrinter::visit(const XMLDeclaration& declaration) {
  pushDeclaration(declaration.Value());
  return true;
}

bool XMLPrinter::visit(const XMLUnknown& unknown) {
  pushUnknown(unknown.Value());
  return true;
}
