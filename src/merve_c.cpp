#include "merve.h"
#include "merve_c.h"

#include <new>

struct merve_analysis_impl {
  std::optional<lexer::lexer_analysis> result{};
};

static merve_string merve_string_create(const char* data, size_t length) {
  merve_string out{};
  out.data = data;
  out.length = length;
  return out;
}

extern "C" {

merve_analysis merve_parse_commonjs(const char* input, size_t length) {
  merve_analysis_impl* impl = new (std::nothrow) merve_analysis_impl();
  if (!impl) return nullptr;
  if (input != nullptr) {
    impl->result = lexer::parse_commonjs(std::string_view(input, length));
  } else {
    impl->result = lexer::parse_commonjs(std::string_view("", 0));
  }
  return static_cast<merve_analysis>(impl);
}

bool merve_is_valid(merve_analysis result) {
  if (!result) return false;
  return static_cast<merve_analysis_impl*>(result)->result.has_value();
}

void merve_free(merve_analysis result) {
  if (!result) return;
  delete static_cast<merve_analysis_impl*>(result);
}

size_t merve_get_exports_count(merve_analysis result) {
  if (!result) return 0;
  merve_analysis_impl* impl = static_cast<merve_analysis_impl*>(result);
  if (!impl->result.has_value()) return 0;
  return impl->result->exports.size();
}

size_t merve_get_reexports_count(merve_analysis result) {
  if (!result) return 0;
  merve_analysis_impl* impl = static_cast<merve_analysis_impl*>(result);
  if (!impl->result.has_value()) return 0;
  return impl->result->re_exports.size();
}

merve_string merve_get_export_name(merve_analysis result, size_t index) {
  if (!result) return merve_string_create(nullptr, 0);
  merve_analysis_impl* impl = static_cast<merve_analysis_impl*>(result);
  if (!impl->result.has_value()) return merve_string_create(nullptr, 0);
  if (index >= impl->result->exports.size())
    return merve_string_create(nullptr, 0);
  std::string_view sv =
      lexer::get_string_view(impl->result->exports[index]);
  return merve_string_create(sv.data(), sv.size());
}

uint32_t merve_get_export_line(merve_analysis result, size_t index) {
  if (!result) return 0;
  merve_analysis_impl* impl = static_cast<merve_analysis_impl*>(result);
  if (!impl->result.has_value()) return 0;
  if (index >= impl->result->exports.size()) return 0;
  return impl->result->exports[index].line;
}

merve_string merve_get_reexport_name(merve_analysis result, size_t index) {
  if (!result) return merve_string_create(nullptr, 0);
  merve_analysis_impl* impl = static_cast<merve_analysis_impl*>(result);
  if (!impl->result.has_value()) return merve_string_create(nullptr, 0);
  if (index >= impl->result->re_exports.size())
    return merve_string_create(nullptr, 0);
  std::string_view sv =
      lexer::get_string_view(impl->result->re_exports[index]);
  return merve_string_create(sv.data(), sv.size());
}

uint32_t merve_get_reexport_line(merve_analysis result, size_t index) {
  if (!result) return 0;
  merve_analysis_impl* impl = static_cast<merve_analysis_impl*>(result);
  if (!impl->result.has_value()) return 0;
  if (index >= impl->result->re_exports.size()) return 0;
  return impl->result->re_exports[index].line;
}

int merve_get_last_error(void) {
  const std::optional<lexer::lexer_error>& err = lexer::get_last_error();
  if (!err.has_value()) return -1;
  return static_cast<int>(err.value());
}

const char* merve_get_version(void) { return MERVE_VERSION; }

merve_version_components merve_get_version_components(void) {
  merve_version_components vc{};
  vc.major = lexer::MERVE_VERSION_MAJOR;
  vc.minor = lexer::MERVE_VERSION_MINOR;
  vc.revision = lexer::MERVE_VERSION_REVISION;
  return vc;
}

}  /* extern "C" */
