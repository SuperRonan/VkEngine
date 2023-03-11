
#include <functional>

namespace vkl
{
    template <class T>
    class DynamicValue
    {
    protected:

        T _value = {};

    public:

        DynamicValue() = default;

        DynamicValue(T && t):
            _value(std::forward<T>(t))
        {}

        DynamicValue(DynamicValue const&) = delete;

        DynamicValue(DynamicValue && other):
            _value(std::move(other))
        {}

        DynamicValue& operator=(DynamicValue const&) = delete;

        DynamicValue& operator=(DynamicValue&& other)
        {
            std::swap(_value, other.value);
            return this;
        }

        virtual bool isConstant() const {return true;};

        virtual void eval() {};

        T const& value() const
        {
            return _value;
        }

        T const& operator T()const
        {
            return _value;
        }

    };

    template <class T>
    class LambdaValue : public DynamicValue<T>
    {
    protected:

        std::function<T(void)> _lambda;

    public:

        template <class Lambda>
        LambdaValue(Lambda && l):
            DynamicValue(),
            _lambda(l)
        {}

        virtual bool isConstant() const override { return false; };

        virtual void eval() override
        {
            _value = _lambda();
        }
    };



}