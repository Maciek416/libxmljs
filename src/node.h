// Copyright 2009, Squish Tech, LLC.
#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include <libxml/xmlstring.h>

#include "./libxmljs.h"
#include "./object_wrap.h"


namespace libxmljs {

class Node : public LibXmlObj {
  public:

  _xmlNode *xml_obj;

  explicit Node(xmlNode* node);
  virtual ~Node();

  static void Initialize(v8::Handle<v8::Object> target);
  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  protected:

  static v8::Handle<v8::Value> Doc(const v8::Arguments& args);
  static v8::Handle<v8::Value> Namespace(const v8::Arguments& args);
  static v8::Handle<v8::Value> Parent(const v8::Arguments& args);
  static v8::Handle<v8::Value> NextSibling(const v8::Arguments& args);
  static v8::Handle<v8::Value> PrevSibling(const v8::Arguments& args);

  v8::Handle<v8::Value> get_doc();
  v8::Handle<v8::Value> remove_namespace();
  v8::Handle<v8::Value> get_namespace();
  void set_namespace(xmlNs* ns);
  xmlNs * find_namespace(const char * search_str);
  v8::Handle<v8::Value> get_parent();
  v8::Handle<v8::Value> get_prev_sibling();
  v8::Handle<v8::Value> get_next_sibling();
};

}  // namespace libxmljs

#endif  // SRC_NODE_H_
