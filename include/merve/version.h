/**
 * @file version.h
 * @brief Definitions for merve's version number.
 */
#ifndef MERVE_VERSION_H
#define MERVE_VERSION_H

#define MERVE_VERSION "1.0.1" // x-release-please-version

namespace lexer {

enum {
  MERVE_VERSION_MAJOR = 1,     // x-release-please-major
  MERVE_VERSION_MINOR = 0,     // x-release-please-minor
  MERVE_VERSION_REVISION = 1,  // x-release-please-patch
};

}  // namespace lexer

#endif  // MERVE_VERSION_H
