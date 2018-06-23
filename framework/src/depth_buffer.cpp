#include "stdafx.h"

#include "depth_buffer.h"
#include "util.h"
#include <IL/il.h>
#include <IL/ilu.h>

namespace graphics_framework {
// Creates a depth buffer object
depth_buffer::depth_buffer(GLuint width, GLuint height) throw(...)
    : _width(width), _height(height), _depth(texture(width, height)) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _depth.get_id());
  // Check for error
  if (CHECK_GL_ERROR) {
    // Display error
    std::cerr << "ERROR - building depth buffer" << std::endl;
    std::cerr << "Could not allocate depth texture with OpenGL" << std::endl;
    // Throw exception
    throw std::runtime_error("Error creating depth texture with OpenGL");
  }

  // Create the depth image data
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
  // Check if error
  if (CHECK_GL_ERROR) {
    // Display error
    std::cerr << "ERROR - building depth buffer" << std::endl;
    std::cerr << "Could not create depth image data with OpenGL" << std::endl;
    // Throw exception
    throw std::runtime_error("Error creating depth texture with OpenGL");
  }

  // Set texture properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
  CHECK_GL_ERROR; // Not considered fatal here

  // Create and set up the FBO
  glGenFramebuffers(1, &_buffer);
  glBindFramebuffer(GL_FRAMEBUFFER, _buffer);
  // Check for errors
  if (CHECK_GL_ERROR) {
    // Display error
    std::cerr << "ERROR - building depth buffer" << std::endl;
    std::cerr << "Could not allocate frame buffer with OpenGL" << std::endl;
    // Delete framebuffer
    glDeleteFramebuffers(1, &_buffer);
    // Throw exception
    throw std::runtime_error("Error creating depth buffer with OpenGL");
  }

  // Attach the frame and depth buffers
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depth.get_id(), 0);
  // Check for errors
  if (CHECK_GL_ERROR) {
    // Display error
    std::cerr << "ERROR - building depth buffer" << std::endl;
    std::cerr << "Could not attach texture to frame buffer" << std::endl;
    // Delete frame buffer
    glDeleteFramebuffers(1, &_buffer);
    // Throw exception
    throw std::runtime_error("Error creating depth buffer with OpenGL");
  }

  // Set draw buffer
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  // Check for errors
  if (CHECK_GL_ERROR) {
    // Display error
    std::cerr << "ERROR - building depth buffer" << std::endl;
    std::cerr << "Could not set draw buffer" << std::endl;
    // Delete frame buffer
    glDeleteFramebuffers(1, &_buffer);
    // Throw exception
    throw std::runtime_error("Error creating depth buffer with OpenGL");
  }

  // Unbind frame buffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  CHECK_GL_ERROR; // Non-fatal here

  // Log
  std::clog << "LOG - depth bufer built" << std::endl;
}

// Saves the framebuffer
void depth_buffer::save(const std::string &filename, const bool linear) const {
  // Allocate memory to read image data into

  GLfloat *data = new GLfloat[(_width * _height)];
  // Bind the frame
  glBindFramebuffer(GL_FRAMEBUFFER, _buffer);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glReadPixels(0, 0, _width, _height, GL_DEPTH_COMPONENT, GL_FLOAT, data);
  GLuint *udata = nullptr;

  if (linear) {
    glReadPixels(0, 0, _width, _height, GL_DEPTH_COMPONENT, GL_FLOAT, data);
    udata = new GLuint[(_width * _height)];
    const double f = 1000.0;
    const double n = 0.1;
    const double maxm = static_cast<double>(UINT_MAX);
    for (size_t i = 0; i < (_width * _height); i++) {
      if (data[i] == 0.0f) {
        udata[i] = 0;
      } else if (data[i] == 1.0f) {
        udata[i] = UINT_MAX;
      } else {
        double dd = (2.0 * n) / (f + n - data[i] * (f - n));
        // double dd = n / (data[i] + f);
        // double dd = n + (-data[i] / (f - n));
        // double dd = -1.0 * (-data[i] - n) * (f - n);
        // double dd = (data[i] - (2.0 * f*n) / (n - f)) / ((n + f) / (n - f));
        data[i] = dd;
        udata[i] = static_cast<GLuint>(maxm / dd);
      }
    }
  } else {
    glReadPixels(0, 0, _width, _height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, data);
  }

  if (CHECK_GL_ERROR) {
    // Display error
    std::cerr << "ERROR - Couldn't Read glReadPixels" << std::endl;
    // Throw exception
    throw std::runtime_error("ERROR - Couldn't Read glReadPixel");
  }
  // Create bitmap
  ILuint ImgId = -1;
  // Generate the main image name to use.
  ilGenImages(1, &ImgId);
  // Bind this image name.
  ilBindImage(ImgId);
  if (linear) {
    ilTexImage(_width, _height, 0, 1, IL_LUMINANCE, IL_UNSIGNED_INT, udata);
  } else {
    ilTexImage(_width, _height, 0, 1, IL_LUMINANCE, IL_UNSIGNED_INT, data);
  }
  // ilSetData(data);
  // Save image
  auto ret = ilSave(IL_PNG, filename.c_str());
  if (!ret) {
    std::cerr << "ERROR - Can't save image (may already exist, I'm not allowed to overwrite)" << std::endl;
  }
  // Unload bitmap
  ilDeleteImage(ImgId);
  // Unbind framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // Delete allocated memory
  delete[] data;
  if (udata) {
    delete[] udata;
  }
}
}