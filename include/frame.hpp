#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <opencv2/opencv.hpp>

class Frame
{
public:
    Frame(cv::Mat mat) :
        _buf(nullptr),
        _height(mat.rows), 
        _width(mat.cols),
        _type(mat.type()),
        _has_mat(true),
        _has_callback(false),
        _mat(mat)
    {} ;

    Frame(unsigned char *buf, size_t height, size_t width, int type, boost::function<void()> release_callback) : 
       _buf(buf),
      _height(height),
      _width(width),
      _type(type),
      _has_mat(false),
      _has_callback(true),
      _release_callback(release_callback)
    {}  ; 

    virtual ~Frame() { if(_has_callback) {  _release_callback(); } } 

    cv::Mat to_mat() {
         if (_has_mat) return _mat;
         else return cv::Mat(_height, _width, _type, _buf); }

    static int get_cv_type(size_t depth, size_t n_channels)
    {
        switch(depth)
        {
            case 8:
                switch(n_channels)
                {
                    case 1: return CV_8U;
                    case 3: return CV_8UC3;
                    default: 
                            return -1; 
                }
                break;
            case 16:
                switch(n_channels)
                {
                    case 1: return CV_16U;
                    case 3: return CV_16UC3;
                    default:
                            return -1;
                }
                break;
            default:
                return -2; 
                break;
        }
    }    
private:
    unsigned char* _buf;
    size_t _height; 
    size_t _width;
    int _type; 
    cv::Mat _mat; 
    bool _has_mat;
    bool _has_callback;
    boost::function<void()> _release_callback; 
} ; 

typedef boost::shared_ptr < Frame > FramePtr; 
