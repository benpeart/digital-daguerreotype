//
// texture class to simplify displaying Intel RealSense video frame via Open GL
//

#pragma once

struct float2 { float x, y; };

struct rect
{
    float x, y;
    float w, h;

    // Create new rect within original boundaries with give aspect ration
    rect adjust_ratio(float2 size) const
    {
        auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
        if (W > w)
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
    }
};

// Convert rs2::frame to cv::Mat
cv::Mat frame_to_mat(const rs2::frame& frame)
{
    using namespace cv;
    using namespace rs2;

    auto vf = frame.as<video_frame>();
    const int width = vf.get_width();
    const int height = vf.get_height();
    auto format = frame.get_profile().format();

    switch (format)
    {
    case RS2_FORMAT_BGR8:
        return Mat(Size(width, height), CV_8UC3, (void*)frame.get_data(), Mat::AUTO_STEP);

    case RS2_FORMAT_RGB8:
    {
        auto r_rgb = Mat(Size(width, height), CV_8UC3, (void*)frame.get_data(), Mat::AUTO_STEP);
        Mat r_bgr;
        cvtColor(r_rgb, r_bgr, COLOR_RGB2BGR);
        return r_bgr;
    }

    case RS2_FORMAT_Z16:
        return Mat(Size(width, height), CV_16UC1, (void*)frame.get_data(), Mat::AUTO_STEP);

    case RS2_FORMAT_Y8:
        return Mat(Size(width, height), CV_8UC1, (void*)frame.get_data(), Mat::AUTO_STEP);

    case RS2_FORMAT_DISPARITY32:
        return Mat(Size(width, height), CV_32FC1, (void*)frame.get_data(), Mat::AUTO_STEP);

    default:
        throw std::runtime_error("Frame format is not supported yet!");
    }
}

// convert cv::Mat to OpenGL texture
GLuint mat_to_gl_texture(cv::Mat& mat, GLuint &image_texture)
{
    using namespace cv;

    if (!image_texture)
        glGenTextures(1, &image_texture);
    GLenum err = glGetError();

    // convert the color order (as we can't use the OpenGL 3 GL_BGR or GL_BGRA flags below)
    Mat m_rgb;
    cvtColor(mat, m_rgb, COLOR_BGR2RGB);

    auto format = m_rgb.type();
    auto width = m_rgb.cols;
    auto height = m_rgb.rows;

    glBindTexture(GL_TEXTURE_2D, image_texture);

    switch (format)
    {
    case CV_8UC3:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_rgb.data);
        break;
    case CV_8UC4:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_rgb.data);
        break;
    default:
        throw std::runtime_error("The requested format is not supported!");
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return image_texture;
}


// convert rs2::frame to glTexture
GLuint frame_to_gl_texture(const rs2::frame& frame)
{
    GLuint image_texture = 0;

    if (!frame)
        return 0;

    rs2::video_frame vf = frame.as<rs2::video_frame>();
    if (!vf)
        throw std::runtime_error("Only video, motion and pose frames are currently supported!");

    glGenTextures(1, &image_texture);
    GLenum err = glGetError();

    auto format = vf.get_profile().format();
    auto width = vf.get_width();
    auto height = vf.get_height();

    glBindTexture(GL_TEXTURE_2D, image_texture);

    switch (format)
    {
    case RS2_FORMAT_RGB8:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.get_data());
        break;
    case RS2_FORMAT_RGBA8:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.get_data());
        break;
    case RS2_FORMAT_Y8:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.get_data());
        break;
    case RS2_FORMAT_Y10BPACK:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, frame.get_data());
        break;
    default:
        throw std::runtime_error("The requested format is not supported!");
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return image_texture;
}

void render_gl_texture(GLuint image_texture, const rect& r, float alpha = 1.f)
{
    if (!image_texture)
        return;

    glViewport((int)r.x, (int)r.y, (int)r.w, (int)r.h);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glOrtho(0, r.w, r.h, 0, -1, +1);

    glBindTexture(GL_TEXTURE_2D, image_texture);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(0, 1); glVertex2f(0, r.h);
    glTexCoord2f(1, 1); glVertex2f(r.w, r.h);
    glTexCoord2f(1, 0); glVertex2f(r.w, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

#if 0
////////////////////////
// Image display code //
////////////////////////
/// \brief The texture class
class texture
{
public:

    void upload(const rs2::video_frame& frame)
    {
        if (!frame) return;

        if (!_gl_handle)
            glGenTextures(1, &_gl_handle);
        GLenum err = glGetError();

        auto format = frame.get_profile().format();
        auto width = frame.get_width();
        auto height = frame.get_height();
//        _stream_type = frame.get_profile().stream_type();
//        _stream_index = frame.get_profile().stream_index();

        glBindTexture(GL_TEXTURE_2D, _gl_handle);

        switch (format)
        {
        case RS2_FORMAT_RGB8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.get_data());
            break;
        case RS2_FORMAT_RGBA8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.get_data());
            break;
        case RS2_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.get_data());
            break;
        case RS2_FORMAT_Y10BPACK:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, frame.get_data());
            break;
        default:
            throw std::runtime_error("The requested format is not supported by this demo!");
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void show(const rect& r, float alpha = 1.f) const
    {
        if (!_gl_handle)
            return;

        // set_viewport(r);
        glViewport((int)r.x, (int)r.y, (int)r.w, (int)r.h);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glOrtho(0, r.w, r.h, 0, -1, +1);

        glBindTexture(GL_TEXTURE_2D, _gl_handle);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(0, 1); glVertex2f(0, r.h);
        glTexCoord2f(1, 1); glVertex2f(r.w, r.h);
        glTexCoord2f(1, 0); glVertex2f(r.w, 0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        //draw_text(int(0.05f * r.w), int(0.05f * r.h), rs2_stream_to_string(_stream_type));
    }

    GLuint get_gl_handle() { return _gl_handle; }

    void render(const rs2::frame& frame, const rect& rect, float alpha = 1.f)
    {
        if (auto vf = frame.as<rs2::video_frame>())
        {
            upload(vf);
            show(rect.adjust_ratio({ (float)vf.get_width(), (float)vf.get_height() }), alpha);
        }
        else
            throw std::runtime_error("Rendering is currently supported for video, motion and pose frames only");
    }

private:
    GLuint          _gl_handle = 0;
//    rs2_stream      _stream_type = RS2_STREAM_ANY;
//    int             _stream_index{};
};
#endif