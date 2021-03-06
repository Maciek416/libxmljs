// Copyright 2009, Squish Tech, LLC.
#include "./element.h"

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "./document.h"
#include "./attribute.h"

namespace libxmljs {

#define NAME_SYMBOL     v8::String::NewSymbol("name")
#define CONTENT_SYMBOL  v8::String::NewSymbol("content")

v8::Persistent<v8::FunctionTemplate> Element::constructor_template;

// doc, name, attrs, content, callback
v8::Handle<v8::Value>
Element::New(const v8::Arguments& args) {
  v8::HandleScope scope;
  // was created by BUILD_NODE
  if (args.Length() == 0 || args[0]->StrictEquals(v8::Null()))
    return args.This();

  Document *document = LibXmlObj::Unwrap<Document>(args[0]->ToObject());
  v8::String::Utf8Value name(args[1]);

  v8::String::Utf8Value *content = NULL;
  v8::Handle<v8::Function> callback;

  if (args[3]->IsString())
    content = new v8::String::Utf8Value(args[3]->ToString());

  if (args[4]->IsFunction())
    callback = v8::Handle<v8::Function>::Cast(args[4]);

  xmlNode* elem = xmlNewDocNode(document->xml_obj,
                                NULL,
                                (const xmlChar*)*name,
                                content ? (const xmlChar*)**content : NULL);

  v8::Persistent<v8::Object> obj =
    LIBXMLJS_GET_MAYBE_BUILD(Element, xmlNode, elem);
  Element *element = LibXmlObj::Unwrap<Element>(obj);

  if (args[2]->IsObject()) {
    v8::Handle<v8::Object> attributes = args[2]->ToObject();
    v8::Handle<v8::Array> props = attributes->GetPropertyNames();
    for (unsigned int i = 0; i < props->Length(); i++) {
      v8::String::Utf8Value name(
        props->Get(v8::Number::New(i)));
      v8::String::Utf8Value value(
        attributes->Get(props->Get(v8::Number::New(i))));
      element->set_attr(*name, *value);
    }
  }

  obj->Set(v8::String::NewSymbol("document"), args[0]->ToObject());

  if (*callback && !callback->IsNull()) {
    v8::Handle<v8::Value> argv[1] = { obj };
    *callback->Call(obj, 1, argv);
  }

  return obj;
}

v8::Handle<v8::Value>
Element::Name(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  if (args.Length() == 0)
    return element->get_name();

  v8::String::Utf8Value name(args[0]->ToString());
  element->set_name(*name);
  return args.This();
}

v8::Handle<v8::Value>
Element::Attr(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  v8::Handle<v8::Object> attrs;

  switch (args.Length()) {
    case 1:
      // return the named attribute
      if (args[0]->IsString()) {
        v8::String::Utf8Value name(args[0]);
        return element->get_attr(*name);

      // create a new attribute from a hash
      } else {
        attrs = args[0]->ToObject();
      }
      break;

    // 1 or 2 arguments only dude
    default:
      LIBXMLJS_THROW_EXCEPTION(
        "Bad argument(s): #attr(name) or #attr({name: value})");
  }

  v8::Handle<v8::Array> properties = attrs->GetPropertyNames();
  for (unsigned int i = 0; i < properties->Length(); i++) {
    v8::Local<v8::String> prop_name = properties->Get(
      v8::Number::New(i))->ToString();
    v8::String::Utf8Value name(prop_name);
    v8::String::Utf8Value value(attrs->Get(prop_name));
    element->set_attr(*name, *value);
  }

  return args.This();
}

v8::Handle<v8::Value>
Element::Attrs(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  return element->get_attrs();
}

v8::Handle<v8::Value>
Element::AddChild(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  Element *child = LibXmlObj::Unwrap<Element>(args[0]->ToObject());
  assert(child);

  element->add_child(child);
  return args.This();
}

v8::Handle<v8::Value>
Element::Find(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  v8::String::Utf8Value xpath(args[0]);
  return element->find(*xpath);
}

v8::Handle<v8::Value>
Element::Text(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  if (args.Length() == 0) {
    return element->get_content();

  } else {
    element->set_content(*v8::String::Utf8Value(args[0]));
  }

  return args.This();
}

v8::Handle<v8::Value>
Element::Child(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  double idx = 1;

  if (args.Length() > 0) {
    if (!args[0]->IsNumber()) {
      LIBXMLJS_THROW_EXCEPTION(
        "Bad argument: must provide #child() with a number");

    } else {
      idx = args[0]->ToNumber()->Value();
    }
  }

  return element->get_child(idx);
}

v8::Handle<v8::Value>
Element::Children(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  return element->get_children();
}

v8::Handle<v8::Value>
Element::Path(const v8::Arguments& args) {
  v8::HandleScope scope;
  Element *element = LibXmlObj::Unwrap<Element>(args.This());
  assert(element);

  return element->get_path();
}

void
Element::set_name(const char* name) {
  xmlNodeSetName(xml_obj, (const xmlChar*)name);
}

v8::Handle<v8::Value>
Element::get_name() {
  return v8::String::New((const char*)xml_obj->name);
}

// TODO(sprsquish) make these work with namespaces
v8::Handle<v8::Value>
Element::get_attr(const char* name) {
  xmlAttr* attr = xmlHasProp(xml_obj, (const xmlChar*)name);
  if (attr)
    return LIBXMLJS_GET_MAYBE_BUILD(Attribute, xmlAttr, attr);

  return v8::Null();
}

// TODO(sprsquish) make these work with namespaces
void
Element::set_attr(const char* name,
                  const char* value) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> argv[3] = { JsObj::Unwrap(xml_obj),
                                    v8::String::New(name),
                                    v8::String::New(value) };
  v8::Persistent<v8::Object>::New(
    Attribute::constructor_template->GetFunction()->NewInstance(3, argv));
}

v8::Handle<v8::Value>
Element::get_attrs() {
  v8::HandleScope scope;
  xmlAttr* attr = xml_obj->properties;

  if (!attr)
    return v8::Array::New();

  v8::Handle<v8::Array> attributes = v8::Array::New();
  v8::Handle<v8::Function> push = v8::Handle<v8::Function>::Cast(
    attributes->Get(v8::String::NewSymbol("push")));
  v8::Handle<v8::Value> argv[1];
  do {
    argv[0] = LIBXMLJS_GET_MAYBE_BUILD(Attribute, xmlAttr, attr);
    push->Call(attributes, 1, argv);
  } while ((attr = attr->next));

  return attributes;
}

void
Element::add_child(Element* child) {
  xmlAddChild(xml_obj, child->xml_obj);
}

v8::Handle<v8::Value>
Element::get_child(double idx) {
  double i = 1;
  xmlNode* child = xml_obj->children;

  while (child && i < idx) {
    child = child->next;
    i++;
  }

  if (!child)
    return v8::Null();

  return LIBXMLJS_GET_MAYBE_BUILD(Element, xmlNode, child);
}

v8::Handle<v8::Value>
Element::get_children() {
  v8::HandleScope scope;
  xmlNode* child = xml_obj->children;
  xmlNodeSetPtr set = xmlXPathNodeSetCreate(child);

  if (!child)
    return v8::Array::New();

  do {
    xmlXPathNodeSetAdd(set, child);
  } while ((child = child->next));

  v8::Handle<v8::Array> children = v8::Array::New(set->nodeNr);
  for (int i = 0; i < set->nodeNr; ++i) {
    xmlNode *node = set->nodeTab[i];
    children->Set(v8::Number::New(i),
                  LIBXMLJS_GET_MAYBE_BUILD(Element, xmlNode, node));
  }

  return children;
}

v8::Handle<v8::Value>
Element::get_path() {
  xmlChar* path = xmlGetNodePath(xml_obj);
  const char* return_path = path ? reinterpret_cast<char*>(path) : "";
  int str_len = xmlStrlen((const xmlChar*)return_path);
  v8::Handle<v8::String> js_obj = v8::String::New(return_path, str_len);
  xmlFree(path);
  return js_obj;
}

void
Element::set_content(const char* content) {
  xmlNodeSetContent(xml_obj, (const xmlChar*)content);
}

v8::Handle<v8::Value>
Element::get_content() {
  xmlChar* content = xmlNodeGetContent(xml_obj);
  if (*content != 0) {
    v8::Handle<v8::String> ret_content = v8::String::New((const char *)content);
    xmlFree(content);
    return ret_content;
  }

  return v8::Null();
}

v8::Handle<v8::Value>
Element::find(const char* xpath) {
  xmlXPathContext* ctxt = xmlXPathNewContext(xml_obj->doc);
  ctxt->node = xml_obj;
  xmlXPathObject* result = xmlXPathEval((const xmlChar*)xpath, ctxt);

  if (!result) {
    xmlXPathFreeContext(ctxt);
    return v8::Array::New(0);
  }

  if (result->type != XPATH_NODESET) {
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(ctxt);
    return v8::Array::New(0);
  }

  v8::Handle<v8::Array> nodes = v8::Array::New(result->nodesetval->nodeNr);
  for (int i = 0; i != result->nodesetval->nodeNr; ++i) {
    xmlNode *node = result->nodesetval->nodeTab[i];
    nodes->Set(v8::Number::New(i),
               LIBXMLJS_GET_MAYBE_BUILD(Element, xmlNode, node));
  }

  xmlXPathFreeObject(result);
  xmlXPathFreeContext(ctxt);

  return nodes;
}

void
Element::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  constructor_template = v8::Persistent<v8::FunctionTemplate>::New(t);
  constructor_template->Inherit(Node::constructor_template);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);

  LXJS_SET_PROTO_METHOD(constructor_template, "addChild", Element::AddChild);
  LXJS_SET_PROTO_METHOD(constructor_template, "attr", Element::Attr);
  LXJS_SET_PROTO_METHOD(constructor_template, "attrs", Element::Attrs);
  LXJS_SET_PROTO_METHOD(constructor_template, "child", Element::Child);
  LXJS_SET_PROTO_METHOD(constructor_template, "children", Element::Children);
  LXJS_SET_PROTO_METHOD(constructor_template, "find", Element::Find);
  LXJS_SET_PROTO_METHOD(constructor_template, "name", Element::Name);
  LXJS_SET_PROTO_METHOD(constructor_template, "path", Element::Path);
  LXJS_SET_PROTO_METHOD(constructor_template, "text", Element::Text);

  target->Set(v8::String::NewSymbol("Element"),
              constructor_template->GetFunction());
}

}  // namespace libxmljs
