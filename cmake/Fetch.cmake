include(FetchContent)

macro(fetch_project NAME REPO TAG)
  FetchContent_Declare(
      ${NAME}
      GIT_REPOSITORY ${REPO}
      GIT_TAG ${TAG})

  FetchContent_MakeAvailable(${NAME})
endmacro()
