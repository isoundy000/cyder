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



#include "V8NativeApplication.h"
#include <iostream>

namespace cyder {

    void stdoutWriteMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto env = Environment::GetCurrent(args);
        auto text = env->toStdString(args[0]);
        std::cout << text;
    }

    void stderrWriteMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto env = Environment::GetCurrent(args);
        auto text = env->toStdString(args[0]);
        std::cerr << text;
    }

    void V8NativeApplication::install(const v8::Local<v8::Object>& parent, Environment* env,
                                      v8::Local<v8::Function> EventEmitter) {
        auto application = env->newInstance(EventEmitter).ToLocalChecked();
        env->setPropertyOfObject(parent, "nativeApplication", application);
        auto stdoutObject = env->makeObject();
        env->setPropertyOfObject(stdoutObject, "write", stdoutWriteMethod);
        env->setPropertyOfObject(application, "standardOutput", stdoutObject);
        auto stderrObject = env->makeObject();
        env->setPropertyOfObject(stderrObject, "write", stderrWriteMethod);
        env->setPropertyOfObject(application, "standardError", stderrObject);
    }

}// namespace cyder