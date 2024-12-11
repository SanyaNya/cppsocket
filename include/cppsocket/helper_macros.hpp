#pragma once

//Define platform
#if defined(WIN32) || defined(__MINGW32__)
  #define CPPS_WIN_IMPL 1
  #define CPPS_POSIX_IMPL 0
#else
  #define CPPS_WIN_IMPL 0
  #define CPPS_POSIX_IMPL 1
#endif

#define DETAIL_PP_IF0(...)
#define DETAIL_PP_IF1(...) __VA_ARGS__
#define DETAIL_PP_IF(COND) DETAIL_PP_IF ## COND
#define PP_IF(COND) DETAIL_PP_IF(COND)

#define DETAIL_PP_EMPTY(...)
#define DETAIL_PP_NON_EMPTY(...) __VA_ARGS__

#define DETAIL_PP_IFE0(...) DETAIL_PP_NON_EMPTY
#define DETAIL_PP_IFE1(...) __VA_ARGS__ DETAIL_PP_EMPTY
#define DETAIL_PP_IFE(COND) DETAIL_PP_IFE ## COND
#define PP_IFE(COND) DETAIL_PP_IFE(COND)
