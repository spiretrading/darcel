#ifndef DARCEL_SYNTAX_PARSER_STATEMENTS_HPP
#define DARCEL_SYNTAX_PARSER_STATEMENTS_HPP
#include "darcel/data_types/function_data_type.hpp"
#include "darcel/lexicon/token.hpp"
#include "darcel/syntax/bind_function_statement.hpp"
#include "darcel/syntax/bind_variable_statement.hpp"
#include "darcel/syntax/syntax_builders.hpp"
#include "darcel/syntax/syntax.hpp"
#include "darcel/syntax/syntax_error.hpp"
#include "darcel/syntax/syntax_parser.hpp"

namespace darcel {
  inline std::unique_ptr<bind_function_statement>
      syntax_parser::parse_bind_function_statement(token_iterator& cursor) {
    auto c = cursor;
    if(!match(*c, keyword::word::LET)) {
      return nullptr;
    }
    ++c;
    auto name_location = c.get_location();
    auto& name = parse_identifier(c);
    if(!match(*c, bracket::type::OPEN_ROUND_BRACKET)) {
      return nullptr;
    }
    ++c;
    std::vector<function_data_type::parameter> parameters;
    std::vector<std::shared_ptr<variable>> parameter_elements;
    push_scope();
    m_generic_index = 0;
    if(!match(*c, bracket::type::CLOSE_ROUND_BRACKET)) {
      while(true) {
        auto name_location = c.get_location();
        auto& parameter_name = parse_identifier(c);
        auto existing_parameter = std::find_if(parameters.begin(),
          parameters.end(),
          [&] (auto& p) {
            return p.m_name == parameter_name;
          });
        if(existing_parameter != parameters.end()) {
          throw syntax_error(
            syntax_error_code::FUNCTION_PARAMETER_ALREADY_DEFINED,
            name_location);
        }
        expect(c, punctuation::mark::COLON);
        auto parameter_type = expect_data_type(c);
        parameters.emplace_back(parameter_name, std::move(parameter_type));
        parameter_elements.push_back(std::make_shared<variable>(name_location,
          parameter_name, parameters.back().m_type));
        if(match(*c, bracket::type::CLOSE_ROUND_BRACKET)) {
          break;
        }
        expect(c, punctuation::mark::COMMA);
      }
    }
    ++c;
    expect(c, operation::symbol::ASSIGN);
    for(auto& parameter : parameter_elements) {
      get_current_scope().add(parameter);
    }
    push_scope();
    auto initializer = expect_expression(c);
    pop_scope();
    pop_scope();
    auto type = std::make_shared<function_data_type>(std::move(parameters),
      initializer->get_data_type());
    auto existing_element = get_current_scope().find_within(name);
    auto v = std::make_shared<variable>(cursor.get_location(), name,
      std::move(type));
    auto f = [&] {
      if(existing_element == nullptr) {
        auto f = std::make_shared<function>(v);
        get_current_scope().add(f);
        return f;
      }
      auto f = std::dynamic_pointer_cast<function>(existing_element);
      if(f == nullptr) {
        throw redefinition_syntax_error(name_location, name,
          existing_element->get_location());
      }
      if(!f->add(v)) {
        throw redefinition_syntax_error(name_location, name,
          existing_element->get_location());
      }
      return f;
    }();
    auto statement = std::make_unique<bind_function_statement>(
      cursor.get_location(), std::move(f), std::move(v),
      std::move(parameter_elements), std::move(initializer));
    cursor = c;
    return statement;
  }

  inline std::unique_ptr<bind_variable_statement>
      syntax_parser::parse_bind_variable_statement(token_iterator& cursor) {
    auto c = cursor;
    if(!match(*c, keyword::word::LET)) {
      return nullptr;
    }
    ++c;
    auto name_location = c.get_location();
    auto& name = parse_identifier(c);
    expect(c, operation::symbol::ASSIGN);
    auto initializer = expect_expression(c);
    auto statement = bind_variable(cursor.get_location(), name,
      std::move(initializer), get_current_scope());
    cursor = c;
    return statement;
  }

  inline std::unique_ptr<terminal_node> syntax_parser::parse_terminal_node(
      token_iterator& cursor) {
    if(!cursor.is_empty() && match(*cursor, terminal::type::end_of_file)) {
      auto t = std::make_unique<terminal_node>(cursor.get_location());
      ++cursor;
      return t;
    }
    return nullptr;
  }

  inline std::unique_ptr<statement> syntax_parser::parse_statement(
      token_iterator& cursor) {
    auto c = cursor;
    while(!c.is_empty() && match(*c, terminal::type::new_line)) {
      ++c;
    }
    std::unique_ptr<statement> node;
    if((node = parse_bind_function_statement(c)) != nullptr ||
        (node = parse_bind_variable_statement(c)) != nullptr) {
      if(!is_syntax_node_end(*c)) {
        throw syntax_error(syntax_error_code::NEW_LINE_EXPECTED,
          c.get_location());
      }
      while(!c.is_empty() && match(*c, terminal::type::new_line)) {
        ++c;
      }
      cursor = c;
      return node;
    } else if(auto expression = parse_expression(c)) {
      if(c.is_empty()) {
        return nullptr;
      } else {
        node = std::move(expression);
      }
      if(!is_syntax_node_end(*c)) {
        throw syntax_error(syntax_error_code::NEW_LINE_EXPECTED,
          c.get_location());
      }
      while(!c.is_empty() && match(*c, terminal::type::new_line)) {
        ++c;
      }
      cursor = c;
      return node;
    }
    return nullptr;
  }

  inline std::unique_ptr<statement> syntax_parser::expect_statement(
      token_iterator& cursor) {
    auto s = parse_statement(cursor);
    if(s == nullptr) {
      throw syntax_error(syntax_error_code::STATEMENT_EXPECTED,
        cursor.get_location());
    }
    return s;
  }
}

#endif
