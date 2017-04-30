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



#include "ScreenBuffer.h"
#include "GPUContext.h"
#include <platform/Log.h>
#include "OSWindow.h"

namespace cyder {

    ScreenBuffer::ScreenBuffer(OSWindow* window) : window(window) {
    }

    ScreenBuffer::~ScreenBuffer() {
        if(grContext){
            grContext->abandonContext();
        }
        SkSafeUnref(grContext);
        SkSafeUnref(_surface);
        [openGLContext release];
    }

    void ScreenBuffer::reset(NSView* view) {
        if (!isValid) {
            return;
        }
        NSSize size = view.bounds.size;
        float scaleFactor = 1;
        if (view.window) {
            scaleFactor = view.window.backingScaleFactor;
        }
        int width = SkScalarRoundToInt(size.width * scaleFactor);
        int height = SkScalarRoundToInt(size.height * scaleFactor);
        if(grContext){
            grContext->abandonContext();
        }
        SkSafeUnref(grContext);
        if(openGLContext){
            [openGLContext release];
        }
        static const CGLPixelFormatAttribute attributes[] = {
                kCGLPFAStencilSize, (CGLPixelFormatAttribute) 8,
                kCGLPFAAccelerated,
                kCGLPFADoubleBuffer,
                kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
                (CGLPixelFormatAttribute)0
        };
        CGLPixelFormatObj format = nullptr;
        GLint npix = 0;
        CGLChoosePixelFormat(attributes, &format, &npix);
        CGLDescribePixelFormat(format, 0, kCGLPFASamples, &sampleCount);
        CGLDescribePixelFormat(format, 0, kCGLPFAStencilSize, &stencilBits);

        CGLContextObj ctx;
        auto globalOpenGLContext = GPUContext::OpenGLContext();
        CGLCreateContext(format, (CGLContextObj)[globalOpenGLContext CGLContextObj], &ctx);
        CGLDestroyPixelFormat(format);
        ASSERT(ctx);

        static const GLint interval = 1;
        CGLSetParameter(ctx, kCGLCPSwapInterval, &interval);
        CGLSetCurrentContext(ctx);

        openGLContext = [[NSOpenGLContext alloc] initWithCGLContextObj:ctx];
        ASSERT(openGLContext);
        CGLReleaseContext(ctx);
        [openGLContext makeCurrentContext];
        [openGLContext setView:view];

        auto glInterface = GPUContext::GLInterface();
        grContext = GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)glInterface);
        ASSERT(grContext);
        updateSize(width, height);
    }

    void ScreenBuffer::updateSize(int width, int height) {
        if (!isValid) {
            return;
        }
        _width = width;
        _height = height;
        sizeChanged = true;
    }

    void ScreenBuffer::setWidth(int value) {
        if(!isValid || value < 0){
            return;
        }
        if(value < 0){
            return;
        }
        sizeChanged = true;
        _width = value;
        window->setContentSize(_width, _height);
    }

    void ScreenBuffer::setHeight(int value) {
        if(!isValid || value < 0){
            return;
        }
        sizeChanged = true;
        _height = value;
        window->setContentSize(_width, _height);
    }

    SkSurface* ScreenBuffer::surface() {
        if(!sizeChanged){
            return _surface;
        }
        sizeChanged = false;
        SkSafeUnref(_surface);

        [openGLContext makeCurrentContext];

        GrBackendRenderTargetDesc desc;
        desc.fWidth = _width;
        desc.fHeight = _height;
        desc.fConfig = kSkia8888_GrPixelConfig;
        desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
        desc.fSampleCnt = sampleCount;
        desc.fStencilBits = stencilBits;
        GrGLint buffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &buffer);
        desc.fRenderTargetHandle = buffer;
        _surface = SkSurface::MakeFromBackendRenderTarget(grContext, desc, nullptr).release();

        // the GL_COLOR_BUFFER_BIT has been cleared already.
        glClear(GL_STENCIL_BUFFER_BIT);
        [openGLContext update];
        return _surface;
    }

    void ScreenBuffer::flush() {
        if (!isValid) {
            return;
        }
        [openGLContext makeCurrentContext];
        grContext->flush();
        [openGLContext flushBuffer];
    }
}  // namespace cyder