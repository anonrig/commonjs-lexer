#include "lexer.h"
#include <iostream>

void test1() {
  std::cout << "=== Test 1: getter_opt_outs ===" << std::endl;
  auto result = lexer::parse_commonjs("\
    Object.defineProperty(exports, 'a', {\
      enumerable: true,\
      get: function () {\
        return q.p;\
      }\
    });\
  \
    if (false) {\
      Object.defineProperty(exports, 'a', {\
        enumerable: false,\
        get: function () {\
          return dynamic();\
        }\
      });\
    }\
  \
  ");
  
  if (result.has_value()) {
    std::cout << "Exports (" << result->exports.size() << "): expected 0" << std::endl;
    for (const auto& exp : result->exports) {
      std::cout << "  - \"" << exp << "\"" << std::endl;
    }
  } else {
    std::cout << "Parse failed" << std::endl;
  }
}

void test2() {
  std::cout << "\n=== Test 2: rollup_babel_reexport_getter ===" << std::endl;
  auto result = lexer::parse_commonjs("\
    Object.defineProperty(exports, 'a', {\
      enumerable: true,\
      get: function () {\
        return q.p;\
      }\
    });\
\
    Object.defineProperty(exports, 'b', {\
      enumerable: false,\
      get: function () {\
        return q.p;\
      }\
    });\
\
    Object.defineProperty(exports, \"c\", {\
      get: function get () {\
        return q['p' ];\
      }\
    });\
\
    Object.defineProperty(exports, 'd', {\
      get: function () {\
        return __ns.val;\
      }\
    });\
\
    Object.defineProperty(exports, 'e', {\
      get () {\
        return external;\
      }\
    });\
\
    Object.defineProperty(exports, \"f\", {\
      get: functionget () {\
        return q['p' ];\
      }\
    });\
  ");
  
  if (result.has_value()) {
    std::cout << "Exports (" << result->exports.size() << "): expected 4 (a,c,d,e)" << std::endl;
    for (const auto& exp : result->exports) {
      std::cout << "  - \"" << exp << "\"" << std::endl;
    }
  } else {
    std::cout << "Parse failed" << std::endl;
  }
}

int main() {
  test1();
  test2();
  return 0;
}
