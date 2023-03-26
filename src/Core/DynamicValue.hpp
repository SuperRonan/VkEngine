#pragma once
#include <functional>
#include <memory>
#include <cassert>

namespace vkl
{
    namespace impl
    {
        class DynamicValueInstanceBase
        {
        public:

            virtual bool isConstant() const = 0;
        };

        template <class T>
        class DynamicValueInstance : public DynamicValueInstanceBase
        {
        protected:

            mutable T _value = {};

        public:

            DynamicValueInstance() = default;

            DynamicValueInstance(T const& t) :
                _value(t)
            {}

            DynamicValueInstance(T&& t) :
                _value(std::move(t))
            {}

            DynamicValueInstance(DynamicValueInstance<T> const&) = delete;


            DynamicValueInstance(DynamicValueInstance<T>&& other) :
                _value(std::move(other._value))
            {}

            DynamicValueInstance& operator=(DynamicValueInstance const&) = delete;

            DynamicValueInstance& operator=(DynamicValueInstance&& other)
            {
                std::swap(_value, other.value);
                return this;
            }

            virtual bool isConstant() const override { return true; };

            virtual T const& value() const
            {
                return _value;
            }

        };

        template <class T>
        class LambdaValueInstance : public DynamicValueInstance<T>
        {
        protected:

            using LambdaType = std::function<T(void)>;

            LambdaType _lambda;

        public:

            //template <class Lambda>
            //LambdaValue(Lambda && l):
            //    DynamicValue(),
            //    _lambda(l)
            //{}

            LambdaValueInstance(LambdaType const& lambda) :
                DynamicValueInstance<T>(),
                _lambda(lambda)
            {}

            virtual bool isConstant() const override { return false; };

            virtual T const& value() const override
            {
                // TODO cache
                DynamicValueInstance<T>::_value = _lambda();
                return DynamicValueInstance<T>::_value;
            }


        };
    }

    template <class T>
    class DynamicValue
    {
    protected:

        std::shared_ptr<impl::DynamicValueInstance<T>> _inst = nullptr;

    public:

        using LambdaType = std::function<T(void)>;

        constexpr DynamicValue() = default;

        DynamicValue(T const& t):
            _inst(new impl::DynamicValueInstance<T>(t))
        {}

        DynamicValue(T && t) :
            _inst(new impl::DynamicValueInstance<T>(std::move(t)))
        {}

        DynamicValue(LambdaType const& lambda):
            _inst(new impl::LambdaValueInstance<T>(lambda))
        {}

        DynamicValue(DynamicValue const& other) :
            _inst(other.instance())
        {}

        DynamicValue(DynamicValue&& other) :
            _inst(std::move(other._inst))
        {}

        DynamicValue& operator=(DynamicValue const& other)
        {
            _inst = other._inst;
            return *this;
        }
        DynamicValue& operator=(DynamicValue&& other)
        {
            _inst.swap(other._inst);
            return *this;
        }

        DynamicValue& operator=(T const& t)
        {
            _inst = std::make_shared<impl::DynamicValueInstance<T>>(impl::DynamicValueInstance<T>(t));
            return *this;
        }

        DynamicValue& operator=(T & t)
        {
            _inst = std::make_shared<impl::DynamicValueInstance<T>>(impl::DynamicValueInstance<T>(std::move(t)));
            return *this;
        }

        DynamicValue& operator=(LambdaType const & l)
        {
            _inst = std::make_shared<impl::LambdaValueInstance<T>>(impl::LambdaValueInstance<T>(l));
            return *this;
        }


        T const& value()const
        {
            assert(hasValue());
            return _inst->value();
        }

        operator T()const
        {
            return value();
        }

        auto instance()const
        {
            return _inst;
        }

        constexpr bool hasValue() const
        {
            return _inst.operator bool();
        }

    };

}