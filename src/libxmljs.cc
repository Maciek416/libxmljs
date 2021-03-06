// Copyright 2009, Squish Tech, LLC.
#include "./libxmljs.h"

#include <v8.h>
#include <string>

#include "./natives.h"
#include "./object_wrap.h"
#include "./document.h"
#include "./element.h"
#include "./attribute.h"
#include "./namespace.h"
#include "./parser.h"
#include "./sax_parser.h"

namespace libxmljs {

namespace {

void on_libxml_destruct(xmlNode* node) {
  switch (node->type) {
    case XML_DOCUMENT_NODE:
      delete static_cast<JsObj*>(node->doc->_private);
      node->doc->_private = NULL;
      break;

    default:
      delete static_cast<JsObj*>(node->_private);
      node->_private = NULL;
  }
}

}  // namespace

LibXMLJS::LibXMLJS() {
  xmlInitParser();  // Not always necessary, but necessary for thread safety.
  // xmlRegisterNodeDefault(on_libxml_construct);
  xmlDeregisterNodeDefault(on_libxml_destruct);
  // xmlThrDefRegisterNodeDefault(on_libxml_construct);
  xmlThrDefDeregisterNodeDefault(on_libxml_destruct);
}

LibXMLJS::~LibXMLJS() {
  xmlCleanupParser();  // As per xmlInitParser(), or memory leak will happen.
}

LibXMLJS LibXMLJS::init_;

static void
OnFatalError(const char* location,
             const char* message) {
#define FATAL_ERROR "\033[1;31mV8 FATAL ERROR.\033[m"
  if (location)
    fprintf(stderr, FATAL_ERROR " %s %s\n", location, message);
  else
    fprintf(stderr, FATAL_ERROR " %s\n", message);

  exit(1);
}

// Extracts a C str from a V8 Utf8Value.
const char *
ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<str conversion failed>";
}

static void
ReportException(v8::TryCatch* try_catch) {
  v8::Handle<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    fprintf(stderr, "Error: (no message)\n");
    fflush(stderr);
    return;
  }
  v8::Handle<v8::Value> error = try_catch->Exception();
  v8::Handle<v8::String> stack;
  if (error->IsObject()) {
    v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(error);
    v8::Handle<v8::Value> raw_stack = obj->Get(v8::String::New("stack"));
    if (raw_stack->IsString()) stack = v8::Handle<v8::String>::Cast(raw_stack);
  }
  if (stack.IsEmpty()) {
    v8::String::Utf8Value exception(error);

    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, *exception);
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    fprintf(stderr, "%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++) {
      fprintf(stderr, " ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");

    message->PrintCurrentStackTrace(stderr);


  } else {
    v8::String::Utf8Value trace(stack);
    fprintf(stderr, "%s\n", *trace);
  }
  fflush(stderr);
}

// Executes a str within the current v8 context.
v8::Handle<v8::Value>
ExecuteString(v8::Handle<v8::String> source,
              v8::Handle<v8::Value> filename) {
  v8::HandleScope scope;
  v8::TryCatch try_catch;

  v8::Handle<v8::Script> script = v8::Script::Compile(source, filename);
  if (script.IsEmpty()) {
    ReportException(&try_catch);
    exit(1);
  }

  v8::Handle<v8::Value> result = script->Run();
  if (result.IsEmpty()) {
    ReportException(&try_catch);
    exit(1);
  }

  return scope.Close(result);
}

static void
ExecuteNativeJS(const char* filename,
                const char* data) {
  v8::HandleScope scope;
  v8::TryCatch try_catch;
  ExecuteString(v8::String::New(data), v8::String::New(filename));
  if (try_catch.HasCaught())  {
    puts("There is an error in Node's built-in javascript");
    puts("This should be reported as a bug!");
    ReportException(&try_catch);
    exit(1);
  }
}

void
InitializeLibXMLJS(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  Document::Initialize(target);

  Parser::Initialize(target);
  SaxParser::Initialize(target);

  v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
  v8::Handle<v8::Context> context = v8::Context::New(NULL, global);

  v8::Context::Scope context_scope(context);
  context->Global()->Set(v8::String::NewSymbol("libxml"), target);

  ExecuteNativeJS("sax_parser.js", native_sax_parser);
  ExecuteNativeJS("document.js", native_document);
  ExecuteNativeJS("element.js", native_element);
}

// used by node.js to initialize libraries
extern "C" void
init(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;
  InitializeLibXMLJS(target);
}

int
main(int argc,
     char* argv[]) {
  v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
  v8::V8::Initialize();
  v8::V8::SetFatalErrorHandler(OnFatalError);

  // Create a stack-allocated handle scope.
  v8::HandleScope handle_scope;
  // Create a new context.
  v8::Handle<v8::Context> context = v8::Context::New();
  // Enter the created context for compiling and
  // running the hello world script.
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> global_obj = v8::Context::GetCurrent()->Global();
  v8::Local<v8::Object> libxml_obj = v8::Object::New();

  InitializeLibXMLJS(libxml_obj);

  global_obj->Set(v8::String::NewSymbol("libxml"), libxml_obj);

  // for (int i = 1; i < argc; i++) {
  //   // Create a string containing the JavaScript source code.
  //   Handle<String> source = ReadFile(argv[i]);
  //   // Compile the source code.
  //   Handle<Script> script = Script::Compile(source);
  //   // Run the script to get the result.
  //   Handle<Value> result = script->Run();
  // }

  v8::V8::Dispose();

  return 0;
}
}  // namespace libxmljs
