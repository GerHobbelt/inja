// Copyright (c) 2020 Pantor. All rights reserved.

#include <doctest/doctest.h>

#include "inja/environment.hpp"

#include "test-common.hpp"

TEST_CASE("loading") {
  inja::Environment env;
  inja::json data;
  data["name"] = "Jeff";

  SUBCASE("Files should be loaded") {
    CHECK(env.load_file(test_file_directory + "simple.txt") == "Hello {{ name }}.");
  }

  SUBCASE("Files should be rendered") {
    CHECK(env.render_file(test_file_directory + "simple.txt", data) == "Hello Jeff.");
  }

  SUBCASE("File includes should be rendered") {
    CHECK(env.render_file(test_file_directory + "include.txt", data) == "Answer: Hello Jeff.");
  }

  SUBCASE("File error should throw") {
    std::string path(test_file_directory + "does-not-exist");

    std::string file_error_message = "[inja.exception.file_error] failed accessing file at '" + path + "'";
    CHECK_THROWS_WITH(env.load_file(path), file_error_message.c_str());
    CHECK_THROWS_WITH(env.load_json(path), file_error_message.c_str());
  }
}

TEST_CASE("complete-files") {
  inja::Environment env {test_file_directory};

  for (std::string test_name : {"simple-file", "nested", "nested-line", "html", "html-extend"}) {
    SUBCASE(test_name.c_str()) {
      CHECK(env.render_file_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_file(test_name + "/result.txt"));
    }
  }

  for (std::string test_name : {"error-unknown"}) {
    SUBCASE(test_name.c_str()) {
      CHECK_THROWS_WITH(env.render_file_with_json_file(test_name + "/template.txt", test_name + "/data.json"),
                        "[inja.exception.parser_error] (at 2:10) expected 'in', got 'ins'");
    }
  }
}

TEST_CASE("complete-files-whitespace-control") {
  inja::Environment env {test_file_directory};
  env.set_trim_blocks(true);
  env.set_lstrip_blocks(true);

  for (std::string test_name : {"nested-whitespace"}) {
    SUBCASE(test_name.c_str()) {
      CHECK(env.render_file_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_file(test_name + "/result.txt"));
    }
  }
}

TEST_CASE("global-path") {
  inja::Environment env {test_file_directory, "./"};
  inja::Environment env_result {"./"};
  inja::json data;
  data["name"] = "Jeff";

  SUBCASE("Files should be written") {
    env.write("simple.txt", data, "global-path-result.txt");

    // Fails repeatedly on windows CI
    // CHECK(env_result.load_file("global-path-result.txt") == "Hello Jeff.");
  }
}

TEST_CASE("include-files") {
  inja::Environment env {test_file_directory};
  inja::json data;
  data["name"] = "Jeff";

  SUBCASE("from text") {
    CHECK(env.render_file("include.txt", data) == "Answer: Hello Jeff.");
    CHECK(env.render("Answer: {% include \"simple.txt\" %}", data) == "Answer: Hello Jeff.");

    CHECK_NOTHROW(env.render_file_with_json_file("html/template.txt", "html/data.json"));
  }

  SUBCASE("without local files") {
    env.set_search_included_templates_in_files(false);
    CHECK_THROWS_WITH(env.render_file_with_json_file("html/template.txt", "html/data.json"),
                    "[inja.exception.render_error] (at 3:14) include 'header.txt' not found");
  }
}

TEST_CASE("include-in-memory-and-file-template") {
  inja::Environment env {test_file_directory};

  inja::json data;
  data["name"] = "Jeff";

  std::string error_message = "[inja.exception.file_error] failed accessing file at '" + test_file_directory + "body'";
  CHECK_THROWS_WITH(env.render_file("include-both.txt", data), error_message.c_str());

  const auto parsed_body_template = env.parse("Bye {{ name }}.");
  env.include_template("body", parsed_body_template);

  CHECK(env.render_file("include-both.txt", data) == "Hello Jeff. - Bye Jeff.");
}
