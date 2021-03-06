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

#include "V8Canvas.h"
#include "modules/canvas2d/CanvasRenderingContext2D.h"
#include "modules/canvas/OffScreenBuffer.h"
#include "modules/canvas/Canvas.h"

namespace cyder {

    static void widthGetter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args) {
        auto canvas = static_cast<Canvas*>(args.This()->GetAlignedPointerFromInternalField(0));
        args.GetReturnValue().Set(canvas->width());
    }

    static void widthSetter(v8::Local<v8::Name> property, v8::Local<v8::Value> value,
                            const v8::PropertyCallbackInfo<void>& args) {
        auto env = Environment::GetCurrent(args);
        auto canvas = static_cast<Canvas*>(args.This()->GetAlignedPointerFromInternalField(0));
        canvas->setWidth(env->toInt(value));
    }

    static void heightGetter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args) {
        auto canvas = static_cast<Canvas*>(args.This()->GetAlignedPointerFromInternalField(0));
        args.GetReturnValue().Set(canvas->height());
    }

    static void heightSetter(v8::Local<v8::Name> property, v8::Local<v8::Value> value,
                             const v8::PropertyCallbackInfo<void>& args) {
        auto env = Environment::GetCurrent(args);
        auto canvas = static_cast<Canvas*>(args.This()->GetAlignedPointerFromInternalField(0));
        canvas->setHeight(env->toInt(value));
    }

    static void getContextMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto env = Environment::GetCurrent(args);
        v8::HandleScope scope(env->isolate());
        auto self = args.This();
        auto canvas = static_cast<Canvas*>(self->GetAlignedPointerFromInternalField(0));
        auto contextType = env->toStdString(args[0]);
        if (canvas->context) {
            if (canvas->contextType == contextType) {
                args.GetReturnValue().Set(env->toLocal(canvas->contextObject));
            } else {
                args.GetReturnValue().Set(env->makeNull());
            }
            return;
        }

        canvas->contextType = contextType;
        if (contextType == "2d") {
            auto CanvasRenderingContext2DClass = env->readGlobalFunction("CanvasRenderingContext2D");
            if (!canvas->buffer) {
                bool hasAlpha = true;
                bool useGPU = true;
                if (args[1]->IsObject()) {
                    auto contextAttributes = v8::Local<v8::Object>::Cast(args[1]);
                    auto alphaValue = env->getValue(contextAttributes, "alpha");
                    if (!alphaValue.IsEmpty()) {
                        hasAlpha = alphaValue.ToLocalChecked()->BooleanValue(env->context()).FromMaybe(false);
                    }
                    auto willReadFrequentlyValue = env->getValue(contextAttributes, "willReadFrequently");
                    if (!willReadFrequentlyValue.IsEmpty()) {
                        useGPU = !willReadFrequentlyValue.ToLocalChecked()->
                                BooleanValue(env->context()).FromMaybe(false);
                    }
                }
                canvas->buffer = new OffScreenBuffer(canvas->width(), canvas->height(), hasAlpha, useGPU);
            }
            canvas->context = new CanvasRenderingContext2D(canvas->buffer);
            auto contextObject = env->newInstance(CanvasRenderingContext2DClass,
                                                  env->makeExternal(canvas->context)).ToLocalChecked();
            canvas->contextObject.Reset(env->isolate(), contextObject);
            args.GetReturnValue().Set(contextObject);
        } else {
            args.GetReturnValue().Set(env->makeNull());
        }
    }

    static void makeImageSnapshotMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto env = Environment::GetCurrent(args);
        v8::HandleScope scope(env->isolate());
        auto self = args.This();
        auto canvas = static_cast<Canvas*>(self->GetAlignedPointerFromInternalField(0));
        Image* image;
        if (canvas->buffer) {
            image = canvas->buffer->makeImageSnapshot();
        } else {
            SkBitmap bitmap;
            bitmap.allocN32Pixels(canvas->width(), canvas->height());
            auto pixels = SkImage::MakeFromBitmap(bitmap).release();
            image = new Image(pixels);
        }
        auto ImageClass = env->readGlobalFunction("Image");
        auto imageObject = env->newInstance(ImageClass, env->makeExternal(image)).ToLocalChecked();
        args.GetReturnValue().Set(imageObject);
    }

    static void constructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
        auto env = Environment::GetCurrent(args);
        v8::HandleScope scope(env->isolate());
        Canvas* canvas;
        if (args[0]->IsExternal()) {
            auto external = v8::Local<v8::External>::Cast(args[0]);
            auto buffer = reinterpret_cast<DrawingBuffer*>(external->Value());
            canvas = new Canvas(buffer);
        } else {
            int width = env->toInt(args[0]);
            int height = env->toInt(args[1]);
            canvas = new Canvas(width, height);
        }
        auto self = args.This();
        self->SetAlignedPointerInInternalField(0, canvas);
        env->bind(self, canvas);
    }

    void V8Canvas::install(v8::Local<v8::Object> parent, Environment* env) {
        auto classTemplate = env->makeFunctionTemplate(constructor);
        auto prototypeTemplate = classTemplate->PrototypeTemplate();
        env->setTemplateAccessor(prototypeTemplate, "width", widthGetter, widthSetter);
        env->setTemplateAccessor(prototypeTemplate, "height", heightGetter, heightSetter);
        env->setTemplateProperty(prototypeTemplate, "getContext", getContextMethod);
        env->setTemplateProperty(prototypeTemplate, "makeImageSnapshot", makeImageSnapshotMethod);
        env->attachClass(parent, "Canvas", classTemplate);
    }
}