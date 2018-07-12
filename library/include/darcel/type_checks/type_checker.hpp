#ifndef DARCEL_TYPE_CHECKER_HPP
#define DARCEL_TYPE_CHECKER_HPP
#include <deque>
#include <unordered_map>
#include "darcel/data_types/callable_data_type.hpp"
#include "darcel/semantics/scope.hpp"
#include "darcel/semantics/variable.hpp"
#include "darcel/syntax/syntax_node_visitor.hpp"
#include "darcel/syntax/syntax_nodes.hpp"
#include "darcel/syntax/variable_not_found_error.hpp"
#include "darcel/type_checks/constraints.hpp"
#include "darcel/type_checks/function_overloads.hpp"
#include "darcel/type_checks/type_checks.hpp"
#include "darcel/type_checks/type_map.hpp"

namespace darcel {

  //! Performs type checking on syntax nodes.
  class type_checker {
    public:

      //! Constructs a type checker.
      type_checker();

      //! Constructs a type checker.
      /*!
        \param s The top level scope.
      */
      type_checker(const scope& s);

      //! Returns the type map used to track data types.
      const type_map& get_types() const;

      //! Returns a statement's definition.
      const std::shared_ptr<function_definition>& get_definition(
        const bind_function_statement& s) const;

      //! Returns an expression's particular overload.
      const std::shared_ptr<function_definition>& get_definition(
        const expression& e) const;

      //! Type checks a syntax node.
      void check(const syntax_node& node);

    private:
      std::deque<std::unique_ptr<scope>> m_scopes;
      type_map m_types;
      std::unordered_map<const bind_function_statement*,
        std::shared_ptr<function_definition>> m_definitions;
      std::unordered_map<const expression*,
        std::shared_ptr<function_definition>> m_call_definitions;
  };

  //! Performs type inference on an expression.
  /*!
    \param e The expression to infer.
    \param t The map of types.
    \param s The scope used to get function definitions.
  */
  inline type_map infer_types(const expression& e, const type_map& t,
      const scope& s) {
    struct constraints_visitor final : syntax_node_visitor {
      const type_map* m_types;
      const scope* m_scope;
      constraints m_constraints;
      std::unordered_map<std::shared_ptr<variable>,
        std::vector<std::shared_ptr<data_type>>> m_candidates;

      constraints_visitor(const expression& e, const type_map& t,
          const scope& s)
          : m_types(&t),
            m_scope(&s) {
        e.apply(*this);
      }

      void visit(const call_expression& node) override {
        node.get_callable().apply(*this);
        auto t = m_types->get_type(node.get_callable());
        std::vector<std::shared_ptr<function_data_type>> overloads;
        if(auto f = std::dynamic_pointer_cast<function_data_type>(t)) {
          overloads.push_back(std::move(f));
        } else {
          auto callable_type = std::static_pointer_cast<callable_data_type>(t);
          m_scope->find(*callable_type->get_function(),
            [&] (auto& definition) {
              overloads.push_back(definition.get_type());
              return false;
            });
        }
        disjunctive_set d;
        for(auto& overload : overloads) {
          if(overload->get_parameters().size() ==
              node.get_parameters().size()) {
            conjunctive_set c;
            for(std::size_t i = 0; i != node.get_parameters().size(); ++i) {
              c.add(*node.get_parameters()[i],
                overload->get_parameters()[i].m_type);
              if(auto v = dynamic_cast<const variable_expression*>(
                  node.get_parameters()[i].get())) {
                m_candidates[v->get_variable()].push_back(
                  overload->get_parameters()[i].m_type);
              }
            }
            d.add(std::move(c));
          }
        }
        m_constraints.add(std::move(d));
        for(auto& argument : node.get_parameters()) {
          argument->apply(*this);
        }
      }
    };
    constraints_visitor v(e, t, s);
    std::size_t index = 0;
    std::vector<std::shared_ptr<variable>> inferred_variables;
    for(auto& candidate : v.m_candidates) {
      inferred_variables.push_back(candidate.first);
    }
    while(true) {
      type_map candidate_map = t;
      std::size_t round = 1;
      for(std::size_t i = 0; i < inferred_variables.size(); ++i) {
        auto o = inferred_variables[i];
        candidate_map.add(*o,
          v.m_candidates[o][(i / round) % v.m_candidates[o].size()]);
        round += v.m_candidates[o].size();
      }
      if(v.m_constraints.is_satisfied(candidate_map)) {
        return candidate_map;
      }
      if(round > 10000) {
        break;
      }
    }
    return {};
  }

  inline type_checker::type_checker() {
    m_scopes.push_back(std::make_unique<scope>());
  }

  inline type_checker::type_checker(const scope& s) {
    m_scopes.push_back(std::make_unique<scope>(&s));
  }

  inline const type_map& type_checker::get_types() const {
    return m_types;
  }

  inline const std::shared_ptr<function_definition>&
      type_checker::get_definition(const bind_function_statement& s) const {
    auto i = m_definitions.find(&s);
    if(i == m_definitions.end()) {
      static const std::shared_ptr<function_definition> NONE;
      return NONE;
    }
    return i->second;
  }

  inline const std::shared_ptr<function_definition>&
      type_checker::get_definition(const expression& e) const {
    auto i = m_call_definitions.find(&e);
    if(i == m_call_definitions.end()) {
      static const std::shared_ptr<function_definition> NONE;
      return NONE;
    }
    return i->second;
  }

  inline void type_checker::check(const syntax_node& node) {
    struct type_check_visitor final : syntax_node_visitor {
      type_checker* m_checker;
      std::shared_ptr<data_type> m_last;

      void operator ()(type_checker& checker, const syntax_node& node) {
        m_checker = &checker;
        node.apply(*this);
      }

      void visit(const bind_function_statement& node) override {
        std::vector<function_data_type::parameter> parameters;
        auto infer = false;
        for(auto& parameter : node.get_parameters()) {
          parameters.emplace_back(parameter.m_variable->get_name(),
            parameter.m_type);
          if(parameter.m_type != nullptr) {
            m_checker->m_types.add(*parameter.m_variable, parameter.m_type);
          } else {
            infer = true;
          }
        }
        m_checker->m_scopes.back()->add(node.get_function());
        if(infer) {
          auto inference = infer_types(node.get_expression(),
            m_checker->get_types(), *m_checker->m_scopes.back());
          m_checker->m_types = std::move(inference);
        }
        node.get_expression().apply(*this);
        if(m_checker->m_types.get_type(*node.get_function()) == nullptr) {
          auto callable_type = std::make_shared<callable_data_type>(
            node.get_function());
          m_checker->m_types.add(*node.get_function(), callable_type);
          m_checker->m_scopes.back()->add(callable_type);
        }
        auto t = std::make_shared<function_data_type>(std::move(parameters),
          std::move(m_last));
        auto definition = std::make_shared<function_definition>(
          node.get_location(), node.get_function(), std::move(t));
        m_checker->m_definitions.insert(std::make_pair(&node, definition));
        m_checker->m_scopes.back()->add(definition);
      }

      void visit(const bind_variable_statement& node) override {
        node.get_expression().apply(*this);
        m_checker->m_types.add(*node.get_variable(), std::move(m_last));
      }

      void visit(const call_expression& node) override {
        node.get_callable().apply(*this);
        auto t = std::move(m_last);
        if(auto callable_type =
            std::dynamic_pointer_cast<callable_data_type>(t)) {
          std::vector<function_data_type::parameter> parameters;
          for(auto& parameter : node.get_parameters()) {
            parameter->apply(*this);
            parameters.emplace_back("", std::move(m_last));
          }
          auto overload = find_overload(*callable_type->get_function(),
            parameters, *m_checker->m_scopes.back());
          if(overload == nullptr) {
            throw syntax_error(syntax_error_code::OVERLOAD_NOT_FOUND,
              node.get_callable().get_location());
          }
          auto instance = instantiate(*overload, parameters,
            *m_checker->m_scopes.back());
          for(std::size_t i = 0; i < parameters.size(); ++i) {
            auto& p = parameters[i].m_type;
            auto& o = instance->get_parameters()[i].m_type;
            if(auto callable_type =
                std::dynamic_pointer_cast<callable_data_type>(p)) {
              if(auto signature =
                  std::dynamic_pointer_cast<function_data_type>(o)) {
                auto call_overload = find_overload(
                  *callable_type->get_function(), *signature,
                  *m_checker->m_scopes.back());
                m_checker->m_types.add(*node.get_parameters()[i],
                  std::move(signature));
                m_checker->m_call_definitions.insert(
                  std::make_pair(node.get_parameters()[i].get(),
                  std::move(call_overload)));
              }
            }
          }
          m_checker->m_types.add(node.get_callable(), std::move(instance));
          m_checker->m_call_definitions.insert(
            std::make_pair(&node.get_callable(), std::move(overload)));
        }
        visit(static_cast<const expression&>(node));
      }

      void visit(const expression& node) override {
        m_last = m_checker->m_types.get_type(node);
      }

      void visit(const function_expression& node) override {
        m_last = std::make_shared<callable_data_type>(node.get_function());
      }
    };
    type_check_visitor()(*this, node);
  }
}

#endif
