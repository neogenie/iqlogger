//
// ObjectWrapper.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include "Engine.h"
#include "formats/MessageInterface.h"
#include "processor/ProcessorException.h"

namespace iqlogger::processor::V8 {

class ObjectWrapper
{
public:
  explicit ObjectWrapper(v8::Isolate* isolate);
  ~ObjectWrapper() = default;

  template<typename T, typename = std::enable_if_t<!std::is_base_of_v<formats::MessageInterface, T>>>
  v8::Local<v8::Object> wrapObject(T* object) const;

  template<typename T, typename D = T, typename = std::enable_if_t<std::is_base_of_v<formats::MessageInterface, T>>>
  v8::Local<v8::Object> wrapObject(T* object) const {
    auto jsonString =
        v8::String::NewFromUtf8(m_isolate, object->exportMessage2Json().c_str(), v8::NewStringType::kNormal)
            .ToLocalChecked();

    auto result = v8::JSON::Parse(m_isolate->GetCurrentContext(), jsonString);
    if (result.IsEmpty()) {
      std::stringstream oss;
      v8::String::Utf8Value str(m_isolate, jsonString);
      oss << "Can't parse JSON Object in Processor: " << *str;
      throw ProcessorException(oss.str());
    }

    v8::Local<v8::Value> value = result.ToLocalChecked();

    if (!value->IsObject()) {
      std::stringstream oss;
      v8::String::Utf8Value str(m_isolate, jsonString);
      oss << "Can't create JSON Object in Processor: " << *str;
      throw ProcessorException(oss.str());
    }

    return v8::Local<v8::Object>::Cast(value);
  }

  template<typename T>
  static T* unwrapObject(v8::Local<v8::Object> obj) {
    v8::Local<v8::External> field = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<T*>(ptr);
  }

private:
  v8::Isolate* m_isolate;
};

}  // namespace iqlogger::processor::V8
