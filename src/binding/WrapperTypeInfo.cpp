//////////////////////////////////////////////////////////////////////////////////////
//
//  The MIT License (MIT)
//
//  Copyright (c) 2017-present, cyder.org
//  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of
//  this software and associated documentation files (the "Software"), to deal in the
//  Software without restriction, including without limitation the rights to use, copy,
//  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the
//  following conditions:
//
//      The above copyright notice and this permission notice shall be included in all
//      copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////////////////

#include "WrapperTypeInfo.h"
#include "ToV8.h"

namespace cyder {

    DelayedScriptValue ConstantValue(double value) {
        return std::bind((v8::Local<v8::Number>(*)(v8::Isolate*, double)) ToV8, std::placeholders::_1, value);
    }

    DelayedScriptValue ConstantValue(int value) {
        return std::bind((v8::Local<v8::Value>(*)(v8::Isolate*, int)) ToV8, std::placeholders::_1, value);
    }

    DelayedScriptValue ConstantValue(const char* value) {
        return std::bind((v8::Local<v8::String>(*)(v8::Isolate*, const char*)) ToV8, std::placeholders::_1, value);
    }
}