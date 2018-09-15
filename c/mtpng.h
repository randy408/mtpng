//
// mtpng - a multithreaded parallel PNG encoder in Rust
// mtpng.h - C API header
//
// Copyright (c) 2018 Brion Vibber
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef MTPNG_H_INCLUDED
#define MTPNG_H_INCLUDED 1

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#pragma mark Consts and enums

//
// Pass to mtpng_threadpool_new() as number of threads to
// use the default, which is the detected number of logical
// CPU cores on the system.
//
#define MTPNG_THREADS_DEFAULT 0

//
// Return type for mtpng functions.
// Always check the return value, errors are real!
//
typedef enum mtpng_result_t {
    MTPNG_RESULT_OK = 0,
    MTPNG_RESULT_ERR = 1
} mtpng_result;

//
// Filter types for mtpng_encoder_set_filter_mode().
//
// MTPNG_FILTER_ADAPTIVE is the default behavior, which uses
// a heuristic to try to guess the best compressing filter.
//
typedef enum mtpng_filter_t {
    MTPNG_FILTER_ADAPTIVE = -1,
    MTPNG_FILTER_NONE = 0,
    MTPNG_FILTER_SUB = 1,
    MTPNG_FILTER_UP = 2,
    MTPNG_FILTER_AVERAGE = 3,
    MTPNG_FILTER_PAETH = 4
} mtpng_filter;

//
// Color types for mtpng_encoder_set_color().
//
typedef enum mtpng_color_t {
    MTPNG_COLOR_GREYSCALE = 0,
    MTPNG_COLOR_TRUECOLOR = 2,
    MTPNG_COLOR_INDEXED_COLOR = 3,
    MTPNG_COLOR_GREYSCALE_ALPHA = 4,
    MTPNG_COLOR_TRUECOLOR_ALPHA = 6
} mtpng_color;

#pragma mark Structs

//
// Represents a thread pool, which may be shared between
// multiple encoders at once or over time.
//
// The contents are private; you will only ever use pointers.
//
typedef struct mtpng_threadpool_struct mtpng_threadpool;

//
// Represents a PNG encoder instance, which can encode a single
// image and then must be released. Multiple encoders may share
// a single thread pool.
//
// The contents are private; you will only ever use pointers.
//
typedef struct mtpng_encoder_struct mtpng_encoder;

#pragma mark Function types

//
// Read callback type for mtpng_encoder_new().
//
// When an additional input row is required, this callback is given
// a data buffer to copy into. If data is not yet available, you
// should block until it is.
//
// Return the number of bytes copied, or less on failure.
//
// Callbacks must report all bytes copied to be considered successful;
// failure will propagate to abort the encoding process.
//
typedef size_t (*mtpng_read_func)(void* user_data,
                                  uint8_t* p_bytes,
                                  size_t len);

//
// Write callback type for mtpng_encoder_new().
//
// When output data is available from the decoder, it is sent to this
// callback where you may write it to a file, network socket, memory
// buffer, etc.
//
// Return the number of bytes written, or less on failure.
//
// Callbacks must report all provided data written to be considered
// successful; failure will propagate to abort the encoding process.
//
typedef size_t (*mtpng_write_func)(void* user_data,
                                   const uint8_t* p_bytes,
                                   size_t len);

//
// Flush callback type for mtpng_encoder_new().
//
// If buffering output to a socket or file, you should flush it
// at this point.
//
// This may be called when streaming output at block boundaries,
// allowing a realtime consumer of the data to see and decode
// the additional data.
//
// Return true on success, or false on failure; failure will
// propagate to  abort the encoding process.
//
typedef bool (*mtpng_flush_func)(void* user_data);

#pragma mark ThreadPool

//
// Creates a new thread pool with the given number of threads.
// MTPNG_THREADS_DEFAULT (0) means to auto-detect the number of
// logical processors.
//
// On input, *pp_pool must be NULL.
// On output, *pp_pool will be a pointer to a thread pool instance
// if successful, or remain unchanged in case of error.
//
// If you do not create a thread pool, a default global one will
// be created when you first create an encoder.
//
// A thread pool may be used with multiple encoders, but caller
// is responsible for ensuring that the pool lives longer than
// all the encoders using it.
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_threadpool_new(mtpng_threadpool** pp_pool,
                     size_t threads);

//
// Releases the pool's memory and clears the pointer.
//
// On input, *pp_pool must be a valid instance pointer.
// On output, *pp_pool will be NULL on success or remain unchanged
// in case of failure.
//
// Caller's responsibility to ensure that no encoders are using
// the pool.
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_threadpool_release(mtpng_threadpool** pp_pool);

#pragma mark Encoder

//
// Create a new PNG encoder instance.
//
// On input, *pp_encoder must be NULL.
// On output, *pp_encoder will be an instance pointer on success,
// or remain unchanged in case of failure.
//
// The write_func and flush_func callbacks are required, and must
// not be NULL.
//
// user_data is passed to the callback functions, and may be any
// value such as a private object pointer or NULL.
//
// p_pool may be NULL, in which case a default global thread pool
// will be used. If a thread pool is provided, it is the caller's
// responsibility to keep the thread pool alive until all encoders
// using it have been released.
//
// Check the return values for errors.
//
extern mtpng_result
mtpng_encoder_new(mtpng_encoder** pp_encoder,
                  mtpng_write_func write_func,
                  mtpng_flush_func flush_func,
                  void* const user_data,
                  mtpng_threadpool* p_pool);

//
// Releases the encoder's memory and clears the pointer.
//
// This need only be used if aborting encoding early due to
// errors etc; normally the call to mtpng_encoder_finish()
// at the end of encoding will consume the instance and
// release its memory.
//
// On input, *pp_encoder must be a valid instance pointer.
// On output, *pp_encoder will be NULL on success, or remain
// unchanged in case of failure.
//
// If using a threadpool, must be called before releasing the
// threadpool!
//
// If the encoder is still in use, this may explode.
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_release(mtpng_encoder** pp_encoder);

//
// Set the image size in pixels. The given width and height
// values must not be 0, but are not otherwise limited.
//
// Caller is responsible for ensuring that at least one row
// of image data copied a few times fits in memory, or you're
// gonna have a bad time.
//
// Required before calling mtpng_encoder_write_header().
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_set_size(mtpng_encoder* p_encoder,
                       uint32_t width,
                       uint32_t height);

//
// Set the color type and depth for the image.
//
// Any valid combination of color type and depth is accepted;
// see https://www.w3.org/TR/PNG/#table111 for the specs.
//
// If you do not call this function, mtpng will assume you want
// truecolor with alpha at 8-bit depth.
//
// Must be called before mtpng_encoder_write_header().
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_set_color(mtpng_encoder* p_encoder,
                        mtpng_color color_type,
                        uint8_t depth);

//
// Override the default PNG filter mode selection.
//
// The default is MTPNG_FILTER_NONE for indexed images and
// MTPNG_FILTER_ADAPTIVE for all others. Some images compress
// better with a particular filter.
//
extern mtpng_result
mtpng_encoder_set_filter(mtpng_encoder* p_encoder,
                         mtpng_filter filter_mode);

//
// Override the default chunk size for parallel encoding
// of larger files. Actual chunking will be in terms of
// rows, so data chunks will be at least the given size
// in bytes.
//
// If there are more chunks in the image's raw data bytes
// than available CPUs on the thread pool, you should see
// parallel speedups as long as input data is provided
// fast enough.
//
// If the file is smaller than the chunk size, currently
// the speed will be equivalent to running single-threaded.
//
// chunk_size must be at least 32768 bytes, required for
// maintaining compression across chunks.
//
// Must be called before mtpng_encoder_write_header().
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_set_chunk_size(mtpng_encoder* p_encoder,
                             size_t chunk_size);

//
// Signal that we're done setting up, and start writing
// header data to the output.
//
// Must be called before mtpng_encoder_append_row() or
// mtpng_encoder_finish().
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_write_header(mtpng_encoder *p_encoder);

//
// Encode, compress, and write an image to the output stream
// using pixel data provided by a callback in a "pull" manner.
//
// Must be called after mtpng_encoder_write_header() and before
// mtpng_encoder_finish(). The read_func callback will be called
// once for each row of the image data.
//
// Image data must be pre-packed in the correct bit depth and
// channel order.
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_write_image(mtpng_encoder* p_encoder,
                          mtpng_read_func read_func,
                          void* user_data);

//
// Load one or more rows of input data into the encoder, to be
// filtered and compressed as data is provided.
//
// Use this in preference to mtpng_encoder_write_image() to drive
// the encoder in a "push" manner, or if all data is conveniently
// available in a buffer ahead of time.
//
// Must be called after mtpng_encoder_write_header() and before
// mtpng_encoder_finish().
//
// Image data must be pre-packed in the correct bit depth and
// channel order. If not all rows are provided before calling
// mtpng_encoder_finish(), failure will result.
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_write_image_rows(mtpng_encoder* p_encoder,
                               const uint8_t* p_bytes,
                               size_t len);

//
// Wait for any outstanding work blocks, flush output,
// release the encoder instance and clear the pointer.
//
// Must be called after all rows have been appended with
// mtpng_encoder_append_row().
//
// On input, *pp_encoder must be a valid instance pointer.
// On output, *pp_encoder will be NULL on success, or remain
// unchanged in case of failure.
//
// You do not need to call mtpng_encoder_release after
// this returns, and should not try.
//
// If using a threadpool, must be called before releasing
// the threadpool!
//
// Check the return value for errors.
//
extern mtpng_result
mtpng_encoder_finish(mtpng_encoder** pp_encoder);

#pragma mark footer


#endif /* MTPNG_H_INCLUDED */