#pragma once

#include "frame.hpp"
#include "frame_processor.hpp"

typedef struct _camera_context_t {
    int width;
    int height;

    //void FrameProcessor::process_frame(FramePtr frame, const bool block)
    //void (*processingCb)(FramePtr, const bool);
    FrameProcessorPtr frame_processor;
    //char *lib_file;
    //void *lib_handler;
    //opencv_handler_open_t opencv_handler_open;
    //opencv_handler_close_t opencv_handler_close;
    //opencv_img_processing_t opencv_img_processing;
    //opencv_set_config_t opencv_set_config;
    //void *opencv_handler;

    pthread_t thread_id;

} camera_context;

class Camera {
public:
    Camera(std::shared_ptr<camera_context> ctx) : _ctx(ctx), _width(ctx->width), _height(ctx->height), _fps(0), _initialized(false) {}
    virtual ~Camera() {}

    int get_width() { return _width; }
    int get_height() { return _height; };
    double get_frame_rate() { return _fps; };
    void set_frame_rate(double fps) { if (true == _set_frame_rate(fps)) {_fps = fps;} };
    void init() {
        if (true == _initialized) {
            std::cout << "Camera is already initialized" << std::endl;
        } else {
            _initialized = _init();
            if (false == _initialized) {
                std::cout << "Failed to init camera" << std::endl;
            }
        }
    }
    bool isInit() { return _initialized; }
    virtual void stop() = 0;
    virtual bool usesCallback() = 0;

    virtual void set_exposure(const float exposure_time_ms) = 0;
    virtual void set_gain(const int gain) = 0;
    virtual void set_gain_red(const int gain) = 0;
    virtual void set_gain_green(const int gain) = 0;
    virtual void set_gain_blue(const int gain) = 0;
    virtual void set_gain_boost(bool gain_boost) = 0;

    virtual void set_pixel_clock(unsigned int speed_mhz) = 0;

    virtual void set_shutter_mode(const std::string &shutter_mode) = 0;
    virtual void set_mirror(bool mirror) = 0;

    virtual void set_flash_mode(const std::string &mode) = 0;
    virtual std::string get_flash_mode() = 0;
    virtual void set_strobe_auto(bool) = 0;
    virtual void set_strobe_parameters(int, unsigned int) = 0;

    virtual void set_log_mode(bool log_mode) = 0;
    virtual int get_log_mode_gain() = 0;
    virtual void set_log_mode_gain(const int gain) = 0;
    virtual int get_log_mode_value() = 0;
    virtual void set_log_mode_value(const int value) = 0;

    virtual FramePtr grab() = 0;
    virtual FramePtr grab_y() = 0;
    virtual FramePtr grab_u() = 0;
    virtual FramePtr grab_v() = 0;
    virtual void start_capture() = 0;
protected:
    std::shared_ptr<camera_context>     _ctx;
private:
    virtual bool _set_frame_rate(double fps) = 0;

    // Perform any initialization required for the camera
    // Returns true if successful
    //         false if failed
    virtual bool _init() = 0;

    size_t              _width;
    size_t              _height;
    double              _fps;
    bool                _initialized;
};

typedef boost::shared_ptr<Camera> CameraPtr;

