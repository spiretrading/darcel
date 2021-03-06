#ifndef DARCEL_CALLABLE_DATA_TYPE_HPP
#define DARCEL_CALLABLE_DATA_TYPE_HPP
#include "darcel/data_types/data_type.hpp"
#include "darcel/data_types/data_type_visitor.hpp"
#include "darcel/data_types/data_types.hpp"
#include "darcel/semantics/function.hpp"

namespace darcel {

  //! A data type used to represent all of a function declaration's overloads.
  class CallableDataType final : public DataType {
    public:

      //! Constructs a callable data type.
      /*!
        \param f The function this data type represents.
      */
      CallableDataType(std::shared_ptr<Function> f);

      //! Returns the function represented.
      const std::shared_ptr<Function> get_function() const;

      const Location& get_location() const override;

      const std::string& get_name() const override;

      void apply(DataTypeVisitor& visitor) const override;

    protected:
      bool is_equal(const DataType& rhs) const override;

    private:
      std::string m_name;
      std::shared_ptr<Function> m_function;
  };

  inline CallableDataType::CallableDataType(std::shared_ptr<Function> f)
      : m_name("@" + f->get_name()),
        m_function(std::move(f)) {}

  inline const std::shared_ptr<Function>
      CallableDataType::get_function() const {
    return m_function;
  }

  inline const Location& CallableDataType::get_location() const {
    return m_function->get_location();
  }

  inline const std::string& CallableDataType::get_name() const {
    return m_name;
  }

  inline void CallableDataType::apply(DataTypeVisitor& visitor) const {
    visitor.visit(*this);
  }

  inline bool CallableDataType::is_equal(const DataType& rhs) const {
    auto& other = static_cast<const CallableDataType&>(rhs);
    return m_function == other.get_function();
  }

  inline void DataTypeVisitor::visit(const CallableDataType& node) {
    visit(static_cast<const DataType&>(node));
  }
}

#endif
