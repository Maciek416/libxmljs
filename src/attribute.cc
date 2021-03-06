// Copyright 2009, Squish Tech, LLC.
#include "./attribute.h"
#include "./element.h"
#include "./namespace.h"

namespace libxmljs {

v8::Persistent<v8::FunctionTemplate> Attribute::constructor_template;

v8::Handle<v8::Value>
Attribute::New(const v8::Arguments& args) {
  v8::HandleScope scope;
  // was created by BUILD_NODE
  if (args.Length() == 1 && args[0]->StrictEquals(v8::Null()))
    return args.This();

  Element *element = LibXmlObj::Unwrap<Element>(args[0]->ToObject());

  v8::String::Utf8Value name(args[1]->ToString());
  v8::String::Utf8Value value(args[2]->ToString());

  xmlAttr *elem = xmlSetProp(element->xml_obj,
                             (const xmlChar*)*name,
                             (const xmlChar*)*value);

  // namespace passed in
  if (args.Length() == 4 && args[3]->IsObject()) {
    libxmljs::Namespace *ns = LibXmlObj::Unwrap<libxmljs::Namespace>(
                                args[3]->ToObject());
    assert(ns);

    v8::Persistent<v8::Object> js_attr =
      LIBXMLJS_GET_MAYBE_BUILD(Attribute, xmlAttr, elem);
    Attribute *attr = LibXmlObj::Unwrap<Attribute>(js_attr);
    attr->set_namespace(ns->xml_obj);
  }

  return LIBXMLJS_GET_MAYBE_BUILD(Attribute, xmlAttr, elem);
}

v8::Handle<v8::Value>
Attribute::Name(const v8::Arguments& args) {
  v8::HandleScope scope;
  Attribute *attr = LibXmlObj::Unwrap<Attribute>(args.This());
  assert(attr);

  return attr->get_name();
}

v8::Handle<v8::Value>
Attribute::Value(const v8::Arguments& args) {
  v8::HandleScope scope;
  Attribute *attr = LibXmlObj::Unwrap<Attribute>(args.This());
  assert(attr);

  // attr.value('new value');
  if (args.Length() > 0) {
    attr->set_value(*v8::String::Utf8Value(args[0]));
    return args.This();
  }

  // attr.value();
  return attr->get_value();
}

v8::Handle<v8::Value>
Attribute::Node(const v8::Arguments& args) {
  v8::HandleScope scope;
  Attribute *attr = LibXmlObj::Unwrap<Attribute>(args.This());
  assert(attr);

  return attr->get_element();
}

v8::Handle<v8::Value>
Attribute::get_name() {
  if (xml_obj->name)
    return v8::String::New((const char*)xml_obj->name,
                           xmlStrlen(xml_obj->name));

  return v8::Null();
}

v8::Handle<v8::Value>
Attribute::get_value() {
  xmlChar* value = xmlNodeGetContent(xml_obj);
  if (value != NULL) {
    v8::Handle<v8::String> ret_value = v8::String::New((const char*)value,
                                                       xmlStrlen(value));
    xmlFree(value);
    return ret_value;
  }

  return v8::Null();
}

void
Attribute::set_value(const char* value) {
  if (xml_obj->children)
    xmlFreeNodeList(xml_obj->children);

  xml_obj->children = xml_obj->last = NULL;

  if (value) {
    xmlChar *buffer;
    xmlNode *tmp;

    // Encode our content
    buffer = xmlEncodeEntitiesReentrant(xml_obj->doc, (const xmlChar*)value);

    xml_obj->children = xmlStringGetNodeList(xml_obj->doc, buffer);
    xml_obj->last = NULL;
    tmp = xml_obj->children;

    // Loop through the children
    for (tmp = xml_obj->children; tmp; tmp = tmp->next) {
      tmp->parent = reinterpret_cast<xmlNode *>(xml_obj);
      tmp->doc = xml_obj->doc;
      if (tmp->next == NULL) xml_obj->last = tmp;
    }

    // Free up memory
    xmlFree(buffer);
  }
}

v8::Handle<v8::Value>
Attribute::get_element() {
  return JsObj::Unwrap<xmlNode>(xml_obj->parent);
}

void
Attribute::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(Attribute::New);
  constructor_template = v8::Persistent<v8::FunctionTemplate>::New(t);
  constructor_template->Inherit(Node::constructor_template);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);

  LXJS_SET_PROTO_METHOD(constructor_template, "name", Attribute::Name);
  LXJS_SET_PROTO_METHOD(constructor_template, "value", Attribute::Value);
  LXJS_SET_PROTO_METHOD(constructor_template, "node", Attribute::Node);

  target->Set(v8::String::NewSymbol("Attribute"),
              constructor_template->GetFunction());
}

}  // namespace libxmljs
