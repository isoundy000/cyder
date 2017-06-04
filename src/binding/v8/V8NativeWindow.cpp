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

#include "V8NativeWindow.h"
#include "platform/Window.h"
#include "modules/desktop/NativeWindow.h"

namespace cyder {

    static void activateMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto window = NativeWindow::GetCurrent(args);
        window->activate();
    }

    static void constructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto env = Environment::GetCurrent(args);
        v8::HandleScope scope(env->isolate());
        NativeWindow* nativeWindow = new NativeWindow(args);
        auto self = args.This();
        self->SetAlignedPointerInInternalField(0, nativeWindow);
        env->bind(self, nativeWindow);
    }

    void V8NativeWindow::install(v8::Local<v8::Object> parent, Environment* env) {
        auto classTemplate = env->makeFunctionTemplate(constructor);
        auto prototypeTemplate = classTemplate->PrototypeTemplate();
        env->setTemplateProperty(prototypeTemplate, "activate", activateMethod);
        env->attachClass(parent, "NativeWindow", classTemplate);
    }
}