#ifndef DARCEL_VARIABLE_EXPRESSION_HPP
#define DARCEL_VARIABLE_EXPRESSION_HPP
#include <memory>
#include <utility>
#include "darcel/semantics/variable.hpp"
#include "darcel/syntax/expression.hpp"
#include "darcel/syntax/syntax.hpp"
#include "darcel/syntax/syntax_node_visitor.hpp"

namespace darcel {

  //! An expression that evaluates to a variable.
  class variable_expression final : public expression {
    public:

      //! Constructs a variable expression.
      /*!
        \param l The location of the identifier token.
        \param v The variable to evaluate.
      */
      variable_expression(Location l, std::shared_ptr<Variable> v);

      //! Returns the variable to evaluate.
      const std::shared_ptr<Variable>& get_variable() const;

      void apply(syntax_node_visitor& visitor) const override;

    private:
      std::shared_ptr<Variable> m_variable;
  };

  inline variable_expression::variable_expression(Location l,
      std::shared_ptr<Variable> v)
      : expression(std::move(l)),
        m_variable(std::move(v)) {}

  inline const std::shared_ptr<Variable>& variable_expression::
      get_variable() const {
    return m_variable;
  }

  inline void variable_expression::apply(syntax_node_visitor& visitor) const {
    visitor.visit(*this);
  }

  inline void syntax_node_visitor::visit(const variable_expression& node) {
    visit(static_cast<const expression&>(node));
  }
}

#endif
