//
// IsolateScope.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "IsolateScope.h"

#include "Console.h"
#include "core/Log.h"
#include "processor/ProcessorException.h"

using namespace iqlogger;
using namespace iqlogger::processor;
using namespace iqlogger::processor::V8;

IsolateScope::IsolateScope(v8::Isolate* isolate, ScriptMap scriptMap) :
    m_enginePtr(Engine::getInstance()),
    m_isolate(isolate),
    m_wrapper(m_isolate),
    m_isolate_scope(m_isolate),
    m_handle_scope(m_isolate),
    m_context(v8::Context::New(m_isolate)),
    m_context_scope(m_context),
    m_try_catch(m_isolate),
    m_console(m_wrapper.wrapObject(Console::getInstance().get())) {
  TRACE("IsolateScope::IsolateScope()");

  if (auto result = m_context->Global()->Set(
          m_context, v8::String::NewFromUtf8(m_isolate, "console", v8::NewStringType::kNormal).ToLocalChecked(),
          m_console);
      !result.ToChecked()) {
    if (m_try_catch.HasCaught()) {
      processException("Can't initialize console: ");
    } else {
      throw ProcessorException("Something strange...");
    }
  }

  for (const auto& it : scriptMap) {
    DEBUG("Processor init script for index: " << it.first);

    v8::Local<v8::String> source;

    if (auto result = v8::String::NewFromUtf8(m_isolate, it.second.c_str(), v8::NewStringType::kNormal);
        !result.IsEmpty()) {
      source = result.ToLocalChecked();
    } else {
      std::stringstream oss;
      oss << "Can't create JS script from: " << it.second;
      throw ProcessorException(oss.str().c_str());
    }

    if (m_try_catch.HasCaught()) {
      processException("Can't create JS script: ");
    }

    v8::Local<v8::Script> script;

    if (auto result = v8::Script::Compile(m_context, source); !result.IsEmpty()) {
      script = result.ToLocalChecked();
    } else {
      throw ProcessorException("Something strange...");
    }

    if (m_try_catch.HasCaught()) {
      processException("Can't compile JS script: ");
    }

    v8::Local<v8::Value> value;

    if (auto result = script->Run(m_context); !result.IsEmpty()) {
      value = result.ToLocalChecked();
    } else {
      throw ProcessorException("Something strange...");
    }

    if (m_try_catch.HasCaught()) {
      processException("Can't run JS script: ");
    }

    while (v8::platform::PumpMessageLoop(m_enginePtr->getPlatformPtr(), m_isolate))
      continue;

    if (!value->IsFunction()) {
      throw ProcessorException("Compiled script is not a function");
    }

    m_functions[it.first] = v8::Handle<v8::Function>::Cast(value);
  }
  DEBUG("IsolateScope init done");
}

UniqueMessagePtr IsolateScope::process(ProcessorRecordPtr processorRecordPtr) {
  TRACE("IsolateScope::process()");

  const auto& message = processorRecordPtr->first;
  const auto& scriptIndex = processorRecordPtr->second;

  DEBUG("Process message " << processorRecordPtr.get() << " with script index " << scriptIndex);

  if (scriptIndex > m_functions.size()) {
    throw ProcessorException("Error script index");
  }

  v8::HandleScope handle_scope(m_isolate);
  v8::Local<v8::Context> context = v8::Local<v8::Context>::New(m_isolate, m_context);
  v8::Context::Scope context_scope(context);

  TRACE("IsolateScope make jsObject");

  auto jsObject = makeJsObject(message);

  TRACE("IsolateScope make jsObject - ok");

  v8::Local<v8::Value> args[] = {jsObject};
  v8::Local<v8::Value> result = {};

  TRACE("IsolateScope Call function");

  if (auto r = m_functions[scriptIndex]->Call(context, context->Global(), 1, args); !r.IsEmpty()) {
    result = r.ToLocalChecked();
  } else {
    throw ProcessorException("Something strange...");
  }

  if (m_try_catch.HasCaught()) {
    processException("JavaScript Error: ");
  }

  UniqueMessagePtr resultMessage;

  if (result->IsBoolean() && result->IsFalse()) {
    DEBUG("Processor return false value, ignoring...");
    resultMessage = nullptr;
  } else if (result->IsObject()) {
    v8::Local<v8::String> newJsonString;

    if (auto r = v8::JSON::Stringify(m_context, result); !r.IsEmpty()) {
      newJsonString = r.ToLocalChecked();
    } else {
      throw ProcessorException("Something strange...");
    }

    if (m_try_catch.HasCaught()) {
      processException("JavaScript Error: ");
    }

    v8::String::Utf8Value str(m_isolate, newJsonString);
    resultMessage = makeMessageObject(*str);
  } else if (result->IsString()) {
    v8::String::Utf8Value str(m_isolate, result->ToString(m_context).ToLocalChecked());
    resultMessage = makeMessageObject(*str);
  } else {
    std::stringstream oss;
    v8::String::Utf8Value utf8(m_isolate, result);
    oss << "Result from processor script " << *utf8 << " is not object!";
    throw ProcessorException(oss.str());
  }

  while (v8::platform::PumpMessageLoop(m_enginePtr->getPlatformPtr(), m_isolate)) {
  }

  return resultMessage;
}

IsolateScope::~IsolateScope() {
  TRACE("IsolateScope::~IsolateScope()");
}

void IsolateScope::processException(const std::string& msg) const {
  std::stringstream oss;
  oss << msg << std::endl;

  v8::HandleScope handle_scope(m_isolate);
  v8::String::Utf8Value exception(m_isolate, m_try_catch.Exception());

  v8::Local<v8::Message> message = m_try_catch.Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    oss << *exception << std::endl;
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(m_isolate, message->GetScriptOrigin().ResourceName());
    v8::Local<v8::Context> context(m_isolate->GetCurrentContext());

    int linenum = message->GetLineNumber(context).FromJust();

    oss << *filename << ":" << linenum << " " << *exception << std::endl;

    // Print line of source code.
    v8::String::Utf8Value sourceline(m_isolate, message->GetSourceLine(context).ToLocalChecked());

    oss << *sourceline << std::endl;

    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn(context).FromJust();
    for (int i = 0; i < start; i++) {
      oss << " ";
    }
    int end = message->GetEndColumn(context).FromJust();
    for (int i = start; i < end; i++) {
      oss << "^";
    }

    oss << std::endl;

    v8::Local<v8::Value> stack_trace_string;

    if (auto r = m_try_catch.StackTrace(context); !r.IsEmpty()) {
      stack_trace_string = r.ToLocalChecked();
    } else {
      throw ProcessorException(oss.str());
    }

    if (stack_trace_string->IsString() && v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
      v8::String::Utf8Value stack_trace(m_isolate, stack_trace_string);
      oss << *stack_trace << std::endl;
    }

    throw ProcessorException(oss.str());
  }
}

v8::Local<v8::Object> IsolateScope::makeJsObject(const UniqueMessagePtr& messagePtr) const {
  return std::visit([this](auto& message) { return m_wrapper.wrapObject(&message); }, *messagePtr);
}

UniqueMessagePtr IsolateScope::makeMessageObject(std::string&& message) {
  TRACE("IsolateScope Make Message Object from: " << message);
  auto processorMessage = ProcessorMessage(std::move(message));
  return std::make_unique<Message>(std::move(processorMessage));
}