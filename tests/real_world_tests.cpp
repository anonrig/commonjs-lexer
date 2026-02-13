#include "merve.h"
#include "gtest/gtest.h"

TEST(real_world_tests, esbuild_hint_style) {
  auto result = lexer::parse_commonjs("0 && (module.exports = {a, b, c}) && __exportStar(require('fs'));");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 3);
  ASSERT_EQ(result->re_exports.size(), 1);
  SUCCEED();
}

TEST(real_world_tests, getter_opt_outs) {
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
  ASSERT_TRUE(result.has_value());
  // The lexer doesn't evaluate conditionals like if(false), so the first
  // Object.defineProperty with enumerable:true is detected as an export
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  SUCCEED();
}

TEST(real_world_tests, typescript_reexports) {
  auto result = lexer::parse_commonjs("\
    \"use strict\";\
    function __export(m) {\
        for (var p in m) if (!exports.hasOwnProperty(p)) exports[p] = m[p];\
    }\
    Object.defineProperty(exports, \"__esModule\", { value: true });\
    __export(require(\"external1\"));\
    tslib.__export(require(\"external2\"));\
    __exportStar(require(\"external3\"));\
    tslib1.__exportStar(require(\"external4\"));\
\
    \"use strict\";\
    Object.defineProperty(exports, \"__esModule\", { value: true });\
    var color_factory_1 = require(\"./color-factory\");\
    Object.defineProperty(exports, \"colorFactory\", { enumerable: true, get: function () { return color_factory_1.colorFactory; }, });\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "__esModule");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "colorFactory");
  ASSERT_EQ(result->re_exports.size(), 4);
  ASSERT_EQ(lexer::get_string_view(result->re_exports[0]), "external1");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[1]), "external2");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[2]), "external3");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[3]), "external4");
  SUCCEED();
}

TEST(real_world_tests, rollup_babel_reexport_getter) {
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
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 4);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "c");
  ASSERT_EQ(lexer::get_string_view(result->exports[2]), "d");
  ASSERT_EQ(lexer::get_string_view(result->exports[3]), "e");
  SUCCEED();
}

TEST(real_world_tests, rollup_babel_reexports) {
  auto result = lexer::parse_commonjs("\
    \"use strict\";\
\
    exports.__esModule = true;\
\
    not.detect = require(\"ignored\");\
\
    var _external = require(\"external\");\
\n\
    // Babel <7.12.0, loose mode\n\
    Object.keys(_external).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      exports[key] = _external[key];\
    });\
\
    var _external2 = require(\"external2\");\
\n\
    // Babel <7.12.0\n\
    Object.keys(_external2).forEach(function (key) {\
      if (key === \"default\" || /*comment!*/ key === \"__esModule\") return;\
      Object.defineProperty(exports, key, {\
        enumerable: true,\
        get: function () {\
          return _external2[key];\
        }\
      });\
    });\
\
    var _external001 = require(\"external001\");\
\n\
    // Babel >=7.12.0, loose mode\n\
    Object.keys(_external001).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (key in exports && exports[key] === _external001[key]) return;\
      exports[key] = _external001[key];\
    });\
\
    var _external003 = require(\"external003\");\
\n\
    // Babel >=7.12.0, loose mode, reexports conflicts filter\n\
    Object.keys(_external003).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;\
      if (key in exports && exports[key] === _external003[key]) return;\
      exports[key] = _external003[key];\
    });\
\
    var _external002 = require(\"external002\");\
\n\
    // Babel >=7.12.0\n\
    Object.keys(_external002).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (key in exports && exports[key] === _external002[key]) return;\
      Object.defineProperty(exports, key, {\
        enumerable: true,\
        get: function () {\
          return _external002[key];\
        }\
      });\
    });\
\
    var _external004 = require(\"external004\");\
\n\
    // Babel >=7.12.0, reexports conflict filter\n\
    Object.keys(_external004).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;\
      if (key in exports && exports[key] === _external004[key]) return;\
      Object.defineProperty(exports, key, {\
        enumerable: true,\
        get: function () {\
          return _external004[key];\
        }\
      });\
    });\
\
    let external3 = require('external3');\
    const external4 = require('external4');\
\
    Object.keys(external3).forEach(function (k) {\
      if (k !== 'default') Object.defineProperty(exports, k, {\
        enumerable: true,\
        get: function () {\
          return external3[k];\
        }\
      });\
    });\
    Object.keys(external4).forEach(function (k) {\
      if (k !== 'default') exports[k] = external4[k];\
    });\
\
    const externalǽ = require('external😃');\
    Object.keys(externalǽ).forEach(function (k) {\
      if (k !== 'default') exports[k] = externalǽ[k];\
    });\
\
    let external5 = require('e5');\
    let external6 = require('e6');\
    Object.keys(external5).forEach(function (k) {\
      if (k !== 'default' && !Object.hasOwnProperty.call(exports, k)) exports[k] = external5[k];\
    });\
\
    const not = require('not');\
    Object.keys(not).forEach(function (k) {\
      if (k !== 'default' && !a().hasOwnProperty(k)) exports[k] = not[k];\
    });\
\
    Object.keys(external6).forEach(function (k) {\
      if (k !== 'default' && !exports.hasOwnProperty(k)) exports[k] = external6[k];\
    });\
\
    const external𤭢 = require('external𤭢');\
    Object.keys(external𤭢).forEach(function (k) {\
      if (k !== 'default') exports[k] = external𤭢[k];\
    });\
\
    const notexternal1 = require('notexternal1');\
    Object.keys(notexternal1);\
\
    const notexternal2 = require('notexternal2');\
    Object.keys(notexternal2).each(function(){\
    });\
\
    const notexternal3 = require('notexternal3');\
    Object.keys(notexternal2).forEach(function () {\
    });\
\
    const notexternal4 = require('notexternal4');\
    Object.keys(notexternal2).forEach(function (x) {\
    });\
\
    const notexternal5 = require('notexternal5');\
    Object.keys(notexternal5).forEach(function (x) {\
      if (true);\
    });\
\
    const notexternal6 = require('notexternal6');\
    Object.keys(notexternal6).forEach(function (x) {\
      if (x);\
    });\
\
    const notexternal7 = require('notexternal7');\
    Object.keys(notexternal7).forEach(function(x){\
      if (x ==='default');\
    });\
\
    const notexternal8 = require('notexternal8');\
    Object.keys(notexternal8).forEach(function(x){\
      if (x ==='default'||y);\
    });\
\
    const notexternal9 = require('notexternal9');\
    Object.keys(notexternal9).forEach(function(x){\
      if (x ==='default'||x==='__esM');\
    });\
\
    const notexternal10 = require('notexternal10');\
    Object.keys(notexternal10).forEach(function(x){\
      if (x !=='default') return\
    });\
\
    const notexternal11 = require('notexternal11');\
    Object.keys(notexternal11).forEach(function(x){\
      if (x ==='default'||x==='__esModule') return\
    });\
\
    const notexternal12 = require('notexternal12');\
    Object.keys(notexternal12).forEach(function(x){\
      if (x ==='default'||x==='__esModule') return\
      export[y] = notexternal12[y];\
    });\
\
    const notexternal13 = require('notexternal13');\
    Object.keys(notexternal13).forEach(function(x){\
      if (x ==='default'||x==='__esModule') return\
      exports[y] = notexternal13[y];\
    });\
\
    const notexternal14 = require('notexternal14');\
    Object.keys(notexternal14).forEach(function(x){\
      if (x ==='default'||x==='__esModule') return\
      Object.defineProperty(exports, k, {\
        enumerable: false,\
        get: function () {\
          return external14[k];\
        }\
      });\
    });\
\
    const notexternal15 = require('notexternal15');\
    Object.keys(notexternal15).forEach(function(x){\
      if (x ==='default'||x==='__esModule') return\
      Object.defineProperty(exports, k, {\
        enumerable: false,\
        get: function () {\
          return externalnone[k];\
        }\
      });\
    });\
\
    const notexternal16 = require('notexternal16');\
    Object.keys(notexternal16).forEach(function(x){\
      if (x ==='default'||x==='__esModule') return\
      exports[x] = notexternal16[x];\
      extra;\
    });\
\
    {\
      const notexternal17 = require('notexternal17');\
      Object.keys(notexternal17).forEach(function(x){\
        if (x ==='default'||x==='__esModule') return\
        exports[x] = notexternal17[x];\
      });\
    }\
\
    var _styles = require(\"./styles\");\
    Object.keys(_styles).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;\
      Object.defineProperty(exports, key, {\
        enumerable: true,\
        get: function get() {\
          return _styles[key];\
        }\
      });\
    });\
\
    var _styles2 = require(\"./styles2\");\
    Object.keys(_styles2).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;\
      Object.defineProperty(exports, key, {\
        enumerable: true,\
        get () {\
          return _styles2[key];\
        }\
      });\
    });\
\
    var _Accordion = _interopRequireWildcard(require(\"./Accordion\"));\
    Object.keys(_Accordion).forEach(function (key) {\
      if (key === \"default\" || key === \"__esModule\") return;\
      if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;\
      Object.defineProperty(exports, key, {\
        enumerable: true,\
        get: function () {\
          return _Accordion[key];\
        }\
      });\
    });\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "__esModule");
  ASSERT_EQ(result->re_exports.size(), 15);
  ASSERT_EQ(lexer::get_string_view(result->re_exports[0]), "external");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[1]), "external2");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[2]), "external001");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[3]), "external003");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[4]), "external002");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[5]), "external004");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[6]), "external3");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[7]), "external4");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[8]), "external😃");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[9]), "e5");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[10]), "e6");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[11]), "external𤭢");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[12]), "./styles");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[13]), "./styles2");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[14]), "./Accordion");
  SUCCEED();
}

TEST(real_world_tests, module_exports_reexport_spread) {
  auto result = lexer::parse_commonjs("\
    module.exports = {\
      ...a,\
      ...b,\
      ...require('dep1'),\
      c: d,\
      ...require('dep2'),\
      name\
    };\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "c");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "name");
  ASSERT_EQ(result->re_exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->re_exports[0]), "dep1");
  ASSERT_EQ(lexer::get_string_view(result->re_exports[1]), "dep2");
  SUCCEED();
}

TEST(real_world_tests, regexp_case) {
  auto result = lexer::parse_commonjs("\
      class Number {\
\
      }\
      \
      /(\"|')(?<value>(\\\\(\\1)|[^\\1])*)?(\\1)/.exec(`'\\\\\"\\\\'aa'`);\
      \
      const x = `\"${label.replace(/\"/g, \"\\\\\\\"\")}\"`\
  ");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, regexp_division) {
  auto result = lexer::parse_commonjs("\nconst x = num / /'/.exec(l)[0].slice(1, -1)//'\"");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, multiline_string_escapes) {
  auto result = lexer::parse_commonjs("const str = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAeCAYAAAA7MK6iAAAABmJLR0QA/wAAAAAzJ3zzAAAGTElEQV\\\r\n\t\tRIx+VXe1BU1xn/zjn7ugvL4sIuQnll5U0ELAQxig7WiQYz6NRHa6O206qdSXXSxs60dTK200zNY9q0dcRpMs1jkrRNWmaijCVoaU';\r\n");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, dotted_number) {
  auto result = lexer::parse_commonjs("\
    const x = 5. / 10;\
  ");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, division_operator_case) {
  auto result = lexer::parse_commonjs("\
    function log(r){\
      if(g>=0){u[g++]=m;g>=n.logSz&&(g=0)}else{u.push(m);u.length>=n.logSz&&(g=0)}/^(DBG|TICK): /.test(r)||t.Ticker.tick(454,o.slice(0,200));\
    }\
    \
    (function(n){\
    })();\
  ");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, single_parse_cases) {
  {
    auto result = lexer::parse_commonjs("\'asdf'");
    ASSERT_TRUE(result.has_value());
  }
  {
    auto result = lexer::parse_commonjs("/asdf/");
    ASSERT_TRUE(result.has_value());
  }
  {
    auto result = lexer::parse_commonjs("`asdf`");
    ASSERT_TRUE(result.has_value());
  }
  {
    auto result = lexer::parse_commonjs("/**/");
    ASSERT_TRUE(result.has_value());
  }
  {
    auto result = lexer::parse_commonjs("//");
    ASSERT_TRUE(result.has_value());
  }
  SUCCEED();
}

TEST(real_world_tests, shebang) {
  {
    auto result = lexer::parse_commonjs("#!");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->exports.size(), 0);
  }
  {
    auto result = lexer::parse_commonjs("#! (  {\n      exports.asdf = 'asdf';\n    ");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->exports.size(), 1);
    ASSERT_EQ(lexer::get_string_view(result->exports[0]), "asdf");
  }
  SUCCEED();
}

TEST(real_world_tests, module_exports) {\
  auto result = lexer::parse_commonjs("\
    module.exports.asdf = 'asdf';\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "asdf");
  SUCCEED();
}

TEST(real_world_tests, non_identifiers) {\
  auto result = lexer::parse_commonjs("\
    module.exports = { 'ab cd': foo };\
    exports['not identifier'] = 'asdf';\
    exports['\\u{D83C}\\u{DF10}'] = 1;\
    exports['\\u{D83C}'] = 1;\
    exports['\\''] = 1;\
    exports['@notidentifier'] = 'asdf';\
    Object.defineProperty(exports, \"%notidentifier\", { value: x });\
    Object.defineProperty(exports, 'hm🤔', { value: x });\
    exports['⨉'] = 45;\
    exports['α'] = 54;\
    exports.package = 'STRICT RESERVED!';\
    exports.var = 'RESERVED';\
  ");
  ASSERT_TRUE(result.has_value());
  // Note: \u{D83C} alone is a lone high surrogate, which is invalid and skipped
  // But \u{D83C}\u{DF10} together form a valid surrogate pair for U+1F310 (🌐)
  ASSERT_EQ(result->exports.size(), 11);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "ab cd");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "not identifier");
  ASSERT_EQ(lexer::get_string_view(result->exports[2]), "🌐");  // \u{D83C}\u{DF10} -> U+1F310
  // exports[3] would be \u{D83C} but it's skipped (lone surrogate)
  ASSERT_EQ(lexer::get_string_view(result->exports[3]), "'");   // \\' -> '
  ASSERT_EQ(lexer::get_string_view(result->exports[4]), "@notidentifier");
  ASSERT_EQ(lexer::get_string_view(result->exports[5]), "%notidentifier");
  ASSERT_EQ(lexer::get_string_view(result->exports[6]), "hm🤔");
  ASSERT_EQ(lexer::get_string_view(result->exports[7]), "⨉");
  ASSERT_EQ(lexer::get_string_view(result->exports[8]), "α");
  ASSERT_EQ(lexer::get_string_view(result->exports[9]), "package");
  ASSERT_EQ(lexer::get_string_view(result->exports[10]), "var");
  SUCCEED();
}

TEST(real_world_tests, literal_exports) {
  auto result = lexer::parse_commonjs("\
    module.exports = { a, b: c, d, 'e': f };\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 4);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "b");
  ASSERT_EQ(lexer::get_string_view(result->exports[2]), "d");
  ASSERT_EQ(lexer::get_string_view(result->exports[3]), "e");
  SUCCEED();
}

TEST(real_world_tests, literal_exports_unsupported) {
  auto result = lexer::parse_commonjs("\
    module.exports = { a = 5, b };\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  SUCCEED();
}

TEST(real_world_tests, literal_exports_example) {
  auto result = lexer::parse_commonjs("\n\
    module.exports = {\n\
      // These WILL be detected as exports\n\
      a: a,\n\
      b: b,\n\
      \n\
      // This WILL be detected as an export\n\
      e: require('d'),\n\
    \n\
      // These WONT be detected as exports\n\
      // because the object parser stops on the non-identifier\n\
      // expression \"require('d')\"\n\
      f: 'f'\n\
    }\n\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 3);
  ASSERT_EQ(lexer::get_string_view(result->exports[2]), "e");
  SUCCEED();
}

TEST(real_world_tests, literal_exports_complex) {
  auto result = lexer::parse_commonjs("\n\
    function defineProp(name, value) {\n\
      delete module.exports[name];\n\
      module.exports[name] = value;\n\
      return value;\n\
    }\n\
\n\
    module.exports = {\n\
      Parser: Parser,\n\
      Tokenizer: require(\"./Tokenizer.js\"),\n\
      ElementType: require(\"domelementtype\"),\n\
      DomHandler: DomHandler,\n\
      get FeedHandler() {\n\
          return defineProp(\"FeedHandler\", require(\"./FeedHandler.js\"));\n\
      },\n\
      get Stream() {\n\
          return defineProp(\"Stream\", require(\"./Stream.js\"));\n\
      },\n\
      get WritableStream() {\n\
          return defineProp(\"WritableStream\", require(\"./WritableStream.js\"));\n\
      },\n\
      get ProxyHandler() {\n\
          return defineProp(\"ProxyHandler\", require(\"./ProxyHandler.js\"));\n\
      },\n\
      get DomUtils() {\n\
          return defineProp(\"DomUtils\", require(\"domutils\"));\n\
      },\n\
      get CollectingHandler() {\n\
          return defineProp(\n\
              \"CollectingHandler\",\n\
              require(\"./CollectingHandler.js\")\n\
          );\n\
      },\n\
      // For legacy support\n\
      DefaultHandler: DomHandler,\n\
      get RssHandler() {\n\
          return defineProp(\"RssHandler\", this.FeedHandler);\n\
      },\n\
      //helper methods\n\
      parseDOM: function(data, options) {\n\
          var handler = new DomHandler(options);\n\
          new Parser(handler, options).end(data);\n\
          return handler.dom;\n\
      },\n\
      parseFeed: function(feed, options) {\n\
          var handler = new module.exports.FeedHandler(options);\n\
          new Parser(handler, options).end(feed);\n\
          return handler.dom;\n\
      },\n\
      createDomStream: function(cb, options, elementCb) {\n\
          var handler = new DomHandler(cb, options, elementCb);\n\
          return new Parser(handler, options);\n\
      },\n\
      // List of all events that the parser emits\n\
      EVENTS: {\n\
          /* Format: eventname: number of arguments */\n\
          attribute: 2,\n\
          cdatastart: 0,\n\
          cdataend: 0,\n\
          text: 1,\n\
          processinginstruction: 2,\n\
          comment: 1,\n\
          commentend: 0,\n\
          closetag: 1,\n\
          opentag: 2,\n\
          opentagname: 1,\n\
          error: 1,\n\
          end: 0\n\
      }\n\
    };\n\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "Parser");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "Tokenizer");
  SUCCEED();
}

TEST(real_world_tests, define_property_value) {
  auto result = lexer::parse_commonjs("\
    Object.defineProperty(exports, 'namedExport', { enumerable: false, value: true });\
    Object.defineProperty(exports, 'namedExport', { configurable: false, value: true });\
\
    Object.defineProperty(exports, 'a', {\
      enumerable: false,\
      get () {\
        return p;\
      }\
    });\
    Object.defineProperty(exports, 'b', {\
      configurable: true,\
      get () {\
        return p;\
      }\
    });\
    Object.defineProperty(exports, 'c', {\
      get: () => p\
    });\
    Object.defineProperty(exports, 'd', {\
      enumerable: true,\
      get: function () {\
        return dynamic();\
      }\
    });\
    Object.defineProperty(exports, 'e', {\
      enumerable: true,\
      get () {\
        return 'str';\
      }\
    });\
\
    Object.defineProperty(module.exports, 'thing', { value: true });\
    Object.defineProperty(exports, \"other\", { enumerable: true, value: true });\
    Object.defineProperty(exports, \"__esModule\", { value: true });\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 3);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "thing");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "other");
  ASSERT_EQ(lexer::get_string_view(result->exports[2]), "__esModule");
  SUCCEED();
}

TEST(real_world_tests, module_assign) {
  auto result = lexer::parse_commonjs("\
    module.exports.asdf = 'asdf';\
    exports = 'asdf';\
    module.exports = require('./asdf');\
    if (maybe)\
      module.exports = require(\"./another\");\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "asdf");
  ASSERT_EQ(result->re_exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->re_exports[0]), "./another");
  SUCCEED();
}

TEST(real_world_tests, simple_export_with_unicode_conversions) {
  auto source = "export var p𓀀s,q";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_EXPORT);
  SUCCEED();
}

TEST(real_world_tests, simple_import) {
  auto source = "\
    import test from \"test\";\
    console.log(test);\
  ";

  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_IMPORT);
  SUCCEED();
}

TEST(real_world_tests, exported_function) {
  auto source = "\
    export function a𓀀 () {\
\
    }\
    export class Q{\
\
    }\
  ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_EXPORT);
  SUCCEED();
}

TEST(real_world_tests, export_destructuring) {
  auto source = "\
    export const { a, b } = foo;\
\
    export { ok };\
  ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_EXPORT);
  SUCCEED();
}

TEST(real_world_tests, minified_import_syntax) {
  auto source = "import{TemplateResult as t}from\"lit-html\";import{a as e}from\"./chunk-4be41b30.js\";export{j as SVGTemplateResult,i as TemplateResult,g as html,h as svg}from\"./chunk-4be41b30.js\";window.JSCompiler_renameProperty='asdf';";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_IMPORT);
  SUCCEED();
}

TEST(real_world_tests, plus_plus_division) {
  auto result = lexer::parse_commonjs("\
    tick++/fetti;f=(1)+\")\";\
  ");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, return_bracket_division) {
  auto source = "function variance(){return s/(a-1)}";
  auto result = lexer::parse_commonjs(source);
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, import_meta) {
  auto source = "\
    export var hello = 'world';\
    console.log(import.meta.url);\
  ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_EXPORT);
  SUCCEED();
}

TEST(real_world_tests, import_meta_edge_cases) {
  auto source = R"(    // Import meta
    import.
      meta
    // Not import meta
    a.
    import.
      meta
)";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_IMPORT_META);
  SUCCEED();
}

TEST(real_world_tests, dynamic_import_method) {
  auto source = "\
    class A {\
      import() {\
      }\
    }\
  ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, comments) {
  auto source = "/*\
    VERSION\
  */import util from 'util';\
\
//\
function x() {\
}\
\
      /**/\
      // '\
      /* / */\
      /*\
\
         * export { b }\
      \\*/export { a }\
\
      function () {\
        /***/\
      }\
    ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_IMPORT);
  SUCCEED();
}

TEST(real_world_tests, bracket_matching) {
  auto result = lexer::parse_commonjs("\
    instance.extend('parseExprAtom', function (nextMethod) {\
      return function () {\
        function parseExprAtom(refDestructuringErrors) {\
          if (this.type === tt._import) {\
            return parseDynamicImport.call(this);\
          }\
          return c(refDestructuringErrors);\
        }\
      }();\
    });\
  ");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, division_regex_ambiguity) {
  auto source = "\
    /as)df/; x();\
    a / 2; '  /  '\
    while (true)\
      /test'/\
    x-/a'/g\
    try {}\
    finally{}/a'/g\
    (x);{f()}/d'export { b }/g\
    ;{}/e'/g;\
    {}/f'/g\
    a / 'b' / c;\
    /a'/ - /b'/;\
    +{} /g -'/g'\
    ('a')/h -'/g'\
    if //x\
    ('a')/i'/g;\
    /asdf/ / /as'df/; // '\
    p = `${/test/ + 5}`;\
    /regex/ / x;\
    function m() {\
      return /*asdf8*// 5/;\
    }\
  ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, template_string_expression_ambiguity) {
  auto source = "\
    `$`\
    import('a');\
    ``\
    exports.a = 'a';\
    `a$b`\
    exports['b'] = 'b';\
    `{$}`\
    exports['b'].b;\
  ";
  auto result = lexer::parse_commonjs(source);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "b");
  SUCCEED();
}

TEST(real_world_tests, export_star_failure) {
  // This should not crash - __exportStar with non-require argument
  auto result = lexer::parse_commonjs("__exportStar((0));");
  ASSERT_TRUE(result.has_value());
  SUCCEED();
}

TEST(real_world_tests, esm_syntax_error_import) {
  auto result = lexer::parse_commonjs("\n        import 'x';\n      ");
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_IMPORT);
}

TEST(real_world_tests, esm_syntax_error_export) {
  auto result = lexer::parse_commonjs("\n        export { x };\n      ");
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_EXPORT);
}

TEST(real_world_tests, esm_syntax_error_export_function) {
  auto result = lexer::parse_commonjs("\n        syntax?error;\n\n        export function x () {\n\n        }\n      ");
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_EXPORT);
}

TEST(real_world_tests, esm_syntax_error_import_meta) {
  auto result = lexer::parse_commonjs("\n        import.meta.url\n      ");
  ASSERT_FALSE(result.has_value());
  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err, lexer::lexer_error::UNEXPECTED_ESM_IMPORT_META);
}

TEST(real_world_tests, unicode_escape_sequences) {
  // Test various unicode escape sequences in exports
  auto result = lexer::parse_commonjs("\
    exports['\\u58b8'] = 1;\
    exports['\\n'] = 1;\
    exports['\\xFF'] = 1;\
    exports['\\011'] = 1;\
    exports['\\3z'] = 1;\
  ");
  ASSERT_TRUE(result.has_value());
  // The lexer preserves escape sequences as-is
  ASSERT_EQ(result->exports.size(), 5);
  SUCCEED();
}

TEST(real_world_tests, empty_source) {
  auto result = lexer::parse_commonjs("");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 0);
  ASSERT_EQ(result->re_exports.size(), 0);
  SUCCEED();
}

TEST(real_world_tests, whitespace_only) {
  auto result = lexer::parse_commonjs("   \n\t\r\n   ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 0);
  ASSERT_EQ(result->re_exports.size(), 0);
  SUCCEED();
}

TEST(real_world_tests, nested_require) {
  auto result = lexer::parse_commonjs("\
    const a = require('a');\
    const b = require('b').default;\
    const { c } = require('c');\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 0);
  ASSERT_EQ(result->re_exports.size(), 0);
  SUCCEED();
}

TEST(real_world_tests, conditional_exports) {
  auto result = lexer::parse_commonjs("\
    if (condition) {\
      exports.a = 1;\
    } else {\
      exports.b = 2;\
    }\
  ");
  ASSERT_TRUE(result.has_value());
  // Both exports should be detected
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "b");
  SUCCEED();
}

TEST(real_world_tests, exports_in_function) {
  auto result = lexer::parse_commonjs("\
    function setup() {\
      exports.internal = 1;\
    }\
    exports.external = 2;\
  ");
  ASSERT_TRUE(result.has_value());
  // Both exports should be detected (lexer doesn't track scope)
  ASSERT_EQ(result->exports.size(), 2);
  SUCCEED();
}

TEST(real_world_tests, multiple_module_exports_assign) {
  // Note: module.exports = { a: 1 } with value assignments doesn't extract exports
  // Only shorthand syntax like { a, b } extracts exports
  auto result = lexer::parse_commonjs("\
    module.exports = { a: 1 };\
    module.exports = { b: 2 };\
  ");
  ASSERT_TRUE(result.has_value());
  // No exports detected - object literal with value assignments not supported
  ASSERT_EQ(result->exports.size(), 0);
  SUCCEED();
}

TEST(real_world_tests, string_with_keywords) {
  // Strings containing keywords should not trigger ESM detection
  auto result = lexer::parse_commonjs("\
    const str1 = 'import x from y';\
    const str2 = \"export default foo\";\
    const str3 = `import.meta.url`;\
    exports.a = 1;\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  SUCCEED();
}

TEST(real_world_tests, comment_with_keywords) {
  // Comments containing keywords should not trigger ESM detection
  auto result = lexer::parse_commonjs("\
    // import x from y\n\
    /* export default foo */\
    exports.a = 1;\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  SUCCEED();
}

TEST(real_world_tests, complex_object_literal) {
  // Note: module.exports = { key: value } with value assignments doesn't extract exports
  // Only shorthand syntax like { a, b } extracts exports
  auto result = lexer::parse_commonjs("\
    module.exports = {\
      foo: 'bar',\
      baz: function() {},\
      qux: () => {},\
      nested: { a: 1 }\
    };\
  ");
  ASSERT_TRUE(result.has_value());
  // No exports detected - object literal with value assignments not supported
  ASSERT_EQ(result->exports.size(), 0);
  SUCCEED();
}

TEST(real_world_tests, prototype_exports) {
  auto result = lexer::parse_commonjs("\
    function MyClass() {}\
    MyClass.prototype.method = function() {};\
    module.exports = MyClass;\
  ");
  ASSERT_TRUE(result.has_value());
  // No named exports, just the default
  SUCCEED();
}

TEST(real_world_tests, exports_shorthand_syntax) {
  auto result = lexer::parse_commonjs("\
    const a = 1, b = 2, c = 3;\
    module.exports = { a, b, c };\
  ");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 3);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "b");
  ASSERT_EQ(lexer::get_string_view(result->exports[2]), "c");
  SUCCEED();
}

TEST(real_world_tests, line_numbers_lf) {
  auto result = lexer::parse_commonjs(
    "// line 1\n"
    "exports.a = 1;\n"
    "\n"
    "exports.b = 2;\n"
  );
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "a");
  ASSERT_EQ(result->exports[0].line, 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "b");
  ASSERT_EQ(result->exports[1].line, 4);
}

TEST(real_world_tests, line_numbers_crlf) {
  auto result = lexer::parse_commonjs(
    "// line 1\r\n"
    "exports.x = 1;\r\n"
    "\r\n"
    "exports.y = 2;\r\n"
  );
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "x");
  ASSERT_EQ(result->exports[0].line, 2);
  ASSERT_EQ(lexer::get_string_view(result->exports[1]), "y");
  ASSERT_EQ(result->exports[1].line, 4);
}

TEST(real_world_tests, line_numbers_reexports) {
  auto result = lexer::parse_commonjs(
    "// line 1\n"
    "module.exports = require('dep1');\n"
  );
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->re_exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->re_exports[0]), "dep1");
  ASSERT_EQ(result->re_exports[0].line, 2);
}

TEST(real_world_tests, line_numbers_after_block_comment) {
  auto result = lexer::parse_commonjs(
    "/*\n"
    " * multi-line\n"
    " * comment\n"
    " */\n"
    "exports.after_comment = 1;\n"
  );
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1);
  ASSERT_EQ(lexer::get_string_view(result->exports[0]), "after_comment");
  ASSERT_EQ(result->exports[0].line, 5);
}
