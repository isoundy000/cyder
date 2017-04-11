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


#ifndef CYDER_ENVIRONMENT_H
#define CYDER_ENVIRONMENT_H

#include <v8.h>
#include <string>
#include "utils/USE.h"
#include "platform/Log.h"

namespace cyder {

#define CHECK_EMPTY(maybe, local, failValue) \
            ASSERT(!maybe.IsEmpty()); \
            if(maybe.IsEmpty()){ \
                return failValue; \
            } \
            auto local = maybe.ToLocalChecked()


    namespace {
        template<class T>
        inline v8::Local<T>& StrongPersistentToLocal(const v8::Persistent<T>& persistent) {
            return *reinterpret_cast<v8::Local<T>*>(const_cast<v8::Persistent<T>*>(&persistent));
        }

        inline size_t getSizeOfPointer() {
            uintptr_t ptr;
            return sizeof(ptr);
        }
    }

    enum class ErrorType {
        ERROR = 1,
        TYPE_ERROR = 2,
        RANGE_ERROR = 3,
        REFERENCE_ERROR = 4,
        SYNTAX_ERROR = 5
    };

    class Environment {
    public:

        static const int CONTEXT_EMBEDDER_DATA_INDEX = 1;


        static Environment* GetCurrent(v8::Isolate* isolate) {
            return GetCurrent(isolate->GetCurrentContext());
        }

        static Environment* GetCurrent(const v8::Local<v8::Context>& context) {
            return static_cast<Environment*>(
                    context->GetAlignedPointerFromEmbedderData(CONTEXT_EMBEDDER_DATA_INDEX));
        }

        static Environment* GetCurrent(const v8::FunctionCallbackInfo<v8::Value>& info) {
            ASSERT(info.Data()->IsExternal());
            return static_cast<Environment*>(info.Data().As<v8::External>()->Value());
        }


        Environment(const v8::Local<v8::Context>& context);

        ~Environment();

        /**
         * Execute a script file by file path.
         * @param path The path of the script file to be executed.
         * @return The result of executing script.
         */
        v8::MaybeLocal<v8::Value> executeScript(const std::string& path);

        void throwError(ErrorType errorType, const std::string& errorText);

        void printStackTrace(const v8::TryCatch& tryCatch);

        v8::MaybeLocal<v8::Function> attachClass(v8::Local<v8::Object> parent,
                                                 const std::string& className,
                                                 const v8::Local<v8::FunctionTemplate> classTemplate,
                                                 int internalFieldCount = 1) const;


        ///////////////////////////////////// Inline Methods /////////////////////////////////////

        v8::Isolate* isolate() const {
            return _isolate;
        }

        v8::Local<v8::Context> context() const {
            return StrongPersistentToLocal(_context);
        }

        v8::Local<v8::Object> global() const {
            return StrongPersistentToLocal(_global);
        }

        v8::Local<v8::External> external() const {
            return StrongPersistentToLocal(_external);
        }

        //==================================== Aligned Methods ====================================

        /**
         * Save a local handle to persistent list, then use readAlignedValue or other readAligned methods with the index
         * to retrieve the local handle. <br/>
         */
        void saveAlignedValue(int index, const v8::Local<v8::Value>& handle) {
            auto maxIndex = persistentList.size();
            while (maxIndex <= index) {
                v8::UniquePersistent<v8::Value> persistent;
                persistentList.push_back(std::move(persistent));
                maxIndex++;
            }
            persistentList[index].Reset(_isolate, handle);
        }

        /**
         * Gets a local handle of v8::Value from persistent list.
         * @param index The index of handle in persistent list, which is returned by saveAlignedValue.
         * @returns The local handle.
         */
        v8::Local<v8::Value> readAlignedValue(int index) {
            ASSERT(index < persistentList.size())
            auto& persistent = persistentList[index];
            return *reinterpret_cast<v8::Local<v8::Value>*>(&persistent);
        }

        /**
         * Gets a local handle of v8::Function from persistent list.
         * @param index The index of handle in persistent list, which is returned by saveAlignedValue.
         * @returns The local handle.
         */
        v8::Local<v8::Function> readAlignedFunction(int index) {
            auto value = readAlignedValue(index);
            auto function = v8::Local<v8::Function>::Cast(value);
            return function;
        }

        /**
         * Gets a local handle of v8::Object from persistent list.
         * @param index The index of handle in persistent list, which is returned by saveAlignedValue.
         * @returns The local handle.
         */
        v8::Local<v8::Object> readAlignedObject(int index) {
            auto value = readAlignedValue(index);
            auto object = v8::Local<v8::Object>::Cast(value);
            return object;
        }



        //==================================== To Methods ====================================

        template<class T>
        v8::Local<T> toLocal(const v8::Persistent<T>& persistent) const {
            if (persistent.IsWeak()) {
                return v8::Local<T>::New(_isolate, persistent);
            }
            return StrongPersistentToLocal(persistent);
        }

        v8::MaybeLocal<v8::Function> toFunction(const v8::Local<v8::FunctionTemplate>& functionTemplate) {
            return functionTemplate->GetFunction(context());
        }

        int toInt(const v8::Local<v8::Value>& value) const {
            return value->Int32Value(context()).FromMaybe(0);
        }

        unsigned int toUint(const v8::Local<v8::Value>& value) const {
            return value->Uint32Value(context()).FromMaybe(0);
        }

        float toFloat(const v8::Local<v8::Value>& value) const {
            auto number = value->NumberValue(context()).FromMaybe(0);
            return static_cast<float>(number);
        }

        double toDouble(const v8::Local<v8::Value>& value) const {
            return value->NumberValue(context()).FromMaybe(0);
        }

        bool toBoolean(const v8::Local<v8::Value>& value) const {
            return value->BooleanValue(context()).FromMaybe(false);
        }

        std::string toStdString(const v8::Local<v8::Value>& value) const {
            if (!value->IsString()) {
                return "";
            }
            auto local = v8::Local<v8::String>::Cast(value);
            v8::String::Utf8Value v(local);
            return std::string(*v);
        }

        //==================================== Call Methods ====================================

        v8::MaybeLocal<v8::Value> call(const v8::Local<v8::Function>& function,
                                       const v8::Local<v8::Value>& recv) const {
            return function->Call(context(), recv, 0, nullptr);
        }

        v8::MaybeLocal<v8::Value> call(const v8::Local<v8::Function>& function,
                                       const v8::Local<v8::Value>& recv,
                                       const v8::Local<v8::Value>& arg) const {
            v8::Local<v8::Value> argv[] = {arg};
            return function->Call(context(), recv, 1, argv);
        }

        v8::MaybeLocal<v8::Value> call(const v8::Local<v8::Function>& function,
                                       const v8::Local<v8::Value>& recv,
                                       const v8::Local<v8::Value>& arg0,
                                       const v8::Local<v8::Value>& arg1) const {
            v8::Local<v8::Value> argv[] = {arg0, arg1};
            return function->Call(context(), recv, 2, argv);
        }

        v8::MaybeLocal<v8::Value> call(const v8::Local<v8::Function>& function,
                                       const v8::Local<v8::Value>& recv,
                                       const v8::Local<v8::Value>& arg0,
                                       const v8::Local<v8::Value>& arg1,
                                       const v8::Local<v8::Value>& arg2) const {
            v8::Local<v8::Value> argv[] = {arg0, arg1, arg2};
            return function->Call(context(), recv, 3, argv);
        }


        //==================================== Make Methods ====================================

        v8::Local<v8::Number> makeValue(double value) const {
            return v8::Number::New(_isolate, value);
        }

        v8::Local<v8::Integer> makeValue(int value) const {
            return v8::Integer::New(_isolate, value);
        }

        v8::Local<v8::Integer> makeValue(unsigned int value) const {
            return v8::Integer::NewFromUnsigned(_isolate, value);
        }

        v8::Local<v8::Boolean> makeValue(bool value) const {
            return value ? makeTrue() : makeFalse();
        }

        v8::MaybeLocal<v8::String> makeString(const std::string& text) const {
            static auto type = v8::NewStringType::kNormal;
            return v8::String::NewFromUtf8(_isolate, text.c_str(), type);
        }

        v8::MaybeLocal<v8::Function> makeFunction(v8::FunctionCallback callback = 0) const {
            return v8::Function::New(context(), callback, external());
        }

        v8::Local<v8::FunctionTemplate> makeFunctionTemplate(v8::FunctionCallback callback = 0) const {
            return v8::FunctionTemplate::New(isolate(), callback, external());
        }

        v8::Local<v8::Object> makeObject() const {
            return v8::Object::New(_isolate);
        };

        v8::Local<v8::ObjectTemplate> makeObjectTemplate() const {
            return v8::ObjectTemplate::New(_isolate);
        }

        v8::Local<v8::Primitive> makeNull() const {
            static auto value = v8::Null(_isolate);
            return value;
        }

        v8::Local<v8::Primitive> makeUndefined() const {
            static auto value = v8::Undefined(_isolate);
            return value;
        }

        v8::Local<v8::Boolean> makeTrue() const {
            static auto value = v8::Boolean::New(_isolate, true);
            return value;
        }

        v8::Local<v8::Boolean> makeFalse() const {
            static auto value = v8::Boolean::New(_isolate, false);
            return value;
        }

        v8::Local<v8::External> makeExternal(void* value) const {
            return v8::External::New(_isolate, value);
        }

        v8::Local<v8::Uint32Array> makeHandle(const void* handle) {
            static auto pointerSize = getSizeOfPointer();
            auto arrayBuffer = v8::ArrayBuffer::New(_isolate, pointerSize);
            auto data = reinterpret_cast<uintptr_t*>(arrayBuffer->GetContents().Data());
            data[0] = reinterpret_cast<uintptr_t>(handle);
            return v8::Uint32Array::New(arrayBuffer, 0, pointerSize / 4);
        }

        //==================================== Get Methods ====================================

        v8::MaybeLocal<v8::Value> getValue(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeString = makeString(name);
            CHECK_EMPTY(maybeString, nameValue, v8::MaybeLocal<v8::Value>());
            return target->Get(context(), nameValue);
        }

        v8::MaybeLocal<v8::Function> getFunction(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, v8::MaybeLocal<v8::Function>());
            if (!value->IsFunction()) {
                return v8::MaybeLocal<v8::Function>();
            }
            auto function = v8::Local<v8::Function>::Cast(value);
            return v8::MaybeLocal<v8::Function>(function);
        }

        v8::MaybeLocal<v8::Object> getObject(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, v8::MaybeLocal<v8::Object>());
            if (!value->IsObject()) {
                return v8::MaybeLocal<v8::Object>();
            }
            auto object = v8::Local<v8::Object>::Cast(value);
            return v8::MaybeLocal<v8::Object>(object);
        }


        std::string getStdString(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, "");
            return toStdString(value);
        }

        float getFloat(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, 0);
            auto returnValue = value->NumberValue(context()).FromMaybe(0);
            return static_cast<float>(returnValue);
        }

        int getInt(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, 0);
            return value->Int32Value(context()).FromMaybe(0);
        }

        unsigned int getUint(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, 0);
            return value->Uint32Value(context()).FromMaybe(0);
        }

        bool getBoolean(const v8::Local<v8::Object>& target, const std::string& name) const {
            auto maybeValue = getValue(target, name);
            CHECK_EMPTY(maybeValue, value, false);
            return value->BooleanValue(context()).FromMaybe(false);
        }


        //==================================== Set Object Methods ====================================

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, double value) {
            setPropertyToObject(target, name, v8::Number::New(_isolate, value));
        }

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, int value) {
            setPropertyToObject(target, name, v8::Integer::New(_isolate, value));
        }

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, unsigned int value) {
            setPropertyToObject(target, name, v8::Integer::NewFromUnsigned(_isolate, value));
        }

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, bool value) {
            setPropertyToObject(target, name, v8::Boolean::New(_isolate, value));
        }

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, const std::string& value) {
            auto maybeString = makeString(value);
            ASSERT(!maybeString.IsEmpty());
            if (!maybeString.IsEmpty()) {
                setPropertyToObject(target, name, maybeString.ToLocalChecked());
            }

        }

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, v8::FunctionCallback callback) {
            auto maybeFunction = makeFunction(callback);
            ASSERT(!maybeFunction.IsEmpty());
            if (maybeFunction.IsEmpty()) {
                return;
            }
            auto function = maybeFunction.ToLocalChecked();
            setPropertyToObject(target, name, function);
            auto maybeString = makeString(name);
            ASSERT(!maybeString.IsEmpty());
            if (!maybeString.IsEmpty()) {
                function->SetName(maybeString.ToLocalChecked());
            }
        }

        void setPropertyToObject(v8::Local<v8::Object> target, const std::string& name, v8::Local<v8::Value> value) {
            auto maybeString = makeString(name);
            ASSERT(!maybeString.IsEmpty());
            if (!maybeString.IsEmpty()) {
                auto result = target->Set(context(), maybeString.ToLocalChecked(), value);
                USE(result);
            }
        }


        //==================================== Set Template Methods ====================================

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, double value) {
            setPropertyToTemplate(target, name, v8::Number::New(_isolate, value));
        }

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, int value) {
            setPropertyToTemplate(target, name, v8::Integer::New(_isolate, value));
        }

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, unsigned int value) {
            setPropertyToTemplate(target, name, v8::Integer::NewFromUnsigned(_isolate, value));
        }

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, bool value) {
            setPropertyToTemplate(target, name, v8::Boolean::New(_isolate, value));
        }

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, const std::string& value) {
            auto maybeString = makeString(value);
            ASSERT(!maybeString.IsEmpty());
            if (!maybeString.IsEmpty()) {
                setPropertyToTemplate(target, name, maybeString.ToLocalChecked());
            }
        }

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, v8::FunctionCallback callback) {
            auto functionTemplate = makeFunctionTemplate(callback);
            auto nameValue = makeString(name);
            ASSERT(!nameValue.IsEmpty());
            if (!nameValue.IsEmpty()) {
                functionTemplate->SetClassName(nameValue.ToLocalChecked());
            }
            setPropertyToTemplate(target, name, functionTemplate);
        }

        void setPropertyToTemplate(v8::Local<v8::Template> target, const std::string& name, v8::Local<v8::Data> value) {
            auto maybeString = makeString(name);
            ASSERT(!maybeString.IsEmpty());
            if (!maybeString.IsEmpty()) {
                target->Set(maybeString.ToLocalChecked(), value);
            }
        }

    private:
        v8::Isolate* _isolate;
        v8::Persistent<v8::Context> _context;
        v8::Persistent<v8::Object> _global;
        v8::Persistent<v8::External> _external;
        std::vector<v8::UniquePersistent<v8::Value>> persistentList;
    };

#undef CHECK_EMPTY

}  // namespace cyder

#endif //CYDER_ENVIRONMENT_H
