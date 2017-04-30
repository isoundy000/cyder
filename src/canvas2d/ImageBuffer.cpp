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

#include "ImageBuffer.h"
#include "platform/GPUSurface.h"

namespace cyder {
    ImageBuffer::ImageBuffer(int width, int height, bool alpha, bool useGPU) :
            _width(width), _height(height), alpha(alpha), useGPU(useGPU) {

    }

    ImageBuffer::~ImageBuffer() {
        SkSafeUnref(_surface);
    }

    SkSurface* ImageBuffer::surface() {
        if (sizeChanged) {
            sizeChanged = false;
            SkImageInfo info = SkImageInfo::MakeN32(_width, _height, alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
            if (useGPU) {
                _surface = GPUSurface::Make(info).release();
            } else {
                _surface = SkSurface::MakeRaster(info).release();
            }
        }
        return _surface;
    }

    void ImageBuffer::flush() {
        if (useGPU) {
            GPUSurface::flush();
        }
    }
}