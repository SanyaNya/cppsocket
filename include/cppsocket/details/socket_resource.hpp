#pragma once

#include "platform_headers.hpp"

#include <type_traits>
#include <utility>

namespace cpps::details
{

struct socket_resource
{
  using Handle = std::invoke_result_t<decltype(::socket), int, int, int>;

  static constexpr auto INVALID_HANDLE = HPP_IFE(HPP_WIN_IMPL)(INVALID_SOCKET)(-1);

  socket_resource(Handle handle) noexcept : m_handle_(handle) {}

  // Prevent copy
  socket_resource(const socket_resource&) noexcept = delete;
  socket_resource& operator=(const socket_resource&) noexcept = delete;
  
  socket_resource(socket_resource&& s) noexcept : m_handle_(s.m_handle_)
  {
    s.m_handle_ = INVALID_HANDLE;
  }

  socket_resource& operator=(socket_resource&& s) noexcept
  {
    // Check if this is not the same object
    if(&s != this)
    {
      try_close(m_handle_);

      m_handle_ = std::exchange(s.m_handle_, INVALID_HANDLE);
    }

    return *this;
  }

  ~socket_resource() { try_close(m_handle_); }

  operator Handle() const noexcept { return m_handle_; }

  bool is_invalid() const noexcept { return m_handle_ == INVALID_HANDLE; }

private:
  static void try_close(Handle h) noexcept
  {
    if(h != INVALID_HANDLE)
    {
    #if HPP_WIN_IMPL
      ::closesocket(h);
    #elif HPP_POSIX_IMPL
      ::close(h);
    #endif
    }
  }

  Handle m_handle_;
};

} //namespace cpps::details
