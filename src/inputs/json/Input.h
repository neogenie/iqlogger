//
// Input.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include "tcp/JsonInput.h"
#include "udp/JsonInput.h"

#define JSON_INPUT_INSTANTIATE(...) \
{\
    if(auto p = sourceConfig.getParam<config::Protocol>("protocol"); p)\
    {\
        auto protocol = p.value();\
        if(protocol == config::Protocol::UDP) {\
            return std::make_unique<iqlogger::inputs::json::udp::JsonInput>(__VA_ARGS__);\
        }\
        else if(protocol == config::Protocol::TCP) {\
            return std::make_unique<iqlogger::inputs::json::tcp::JsonInput>(__VA_ARGS__);\
        }\
        else\
        {\
            std::ostringstream oss;\
            oss << "Protocol for input " << sourceConfig.name << " is undefined!";\
            throw Exception(oss.str());\
        }\
    }\
    else\
    {\
        std::ostringstream oss;\
        oss << "Protocol for input " << sourceConfig.name << " not specified!";\
        throw Exception(oss.str());\
    }\
}
