#pragma once
#include <functional>
#include <memory>
#include <cassert>
#include <type_traits>

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
		class PointedValueInstance : public DynamicValueInstance<T>
		{
		protected:

			const T* _ptr;

		public:

			PointedValueInstance(const T* ptr):
				DynamicValueInstance<T>(),
				_ptr(ptr)
			{}

			virtual bool isConstant() const override
			{
				return false;
			}

			virtual T const& value() const override
			{
				DynamicValueInstance<T>::_value = *_ptr;
				return DynamicValueInstance<T>::_value;
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

		DynamicValue(DynamicValue const& other) :
			_inst(other.instance())
		{}

		DynamicValue(DynamicValue&& other) noexcept :
			_inst(std::move(other._inst))
		{}

		DynamicValue(T const& t):
			_inst(new impl::DynamicValueInstance<T>(t))
		{}

		DynamicValue(T && t) :
			_inst(new impl::DynamicValueInstance<T>(std::move(t)))
		{}

		DynamicValue(LambdaType const& lambda):
			_inst(new impl::LambdaValueInstance<T>(lambda))
		{}

		DynamicValue(const T * ptr):
			_inst(new impl::PointedValueInstance<T>(ptr))
		{}

		DynamicValue(T* ptr) :
			_inst(new impl::PointedValueInstance<T>(ptr))
		{}

		template <class Q>
		DynamicValue(DynamicValue<Q> const& q):
			_inst(new impl::LambdaValueInstance<T>([=]() {return T(q.value()); }))
		{
			static_assert(std::is_convertible<Q, T>::value);
		}

		template <class TLike>
		DynamicValue(TLike const& t):
			_inst(nullptr)
		{
			if constexpr (std::is_convertible<TLike, T>::value)
			{
				_inst = std::make_shared<impl::DynamicValueInstance<T>>(T(t));
			}
			else
			{
				_inst = std::make_shared<impl::LambdaValueInstance<T>>(t);
			}
		}

		DynamicValue& operator=(DynamicValue const& other)
		{
			_inst = other._inst;
			return *this;
		}
		DynamicValue& operator=(DynamicValue&& other) noexcept
		{
			_inst.swap(other._inst);
			return *this;
		}

		template <class Q>
		DynamicValue& operator=(DynamicValue<Q> const& o)
		{
			static_assert(std::is_convertible<Q, T>::value);
			_inst = std::make_shared<impl::LambdaValueInstance<T>>([=]() {return T(o.value()); });
			return *this;
		}

		DynamicValue& operator=(T const& t)
		{
			_inst = std::make_shared<impl::DynamicValueInstance<T>>(t);
			return *this;
		}

		DynamicValue& operator=(T & t)
		{
			_inst = std::make_shared<impl::DynamicValueInstance<T>>(std::move(t));
			return *this;
		}

		DynamicValue& operator=(const T* ptr)
		{
			_inst = std::make_shared<impl::PointedValueInstance<T>>(ptr);
			return *this;
		}

		DynamicValue& operator=(T* ptr)
		{
			_inst = std::make_shared<impl::PointedValueInstance<T>>(ptr);
			return *this;
		}

		template <class TLike>
		DynamicValue& operator=(TLike const & t)
		{
			if constexpr (std::is_convertible<TLike, T>::value)
			{
				_inst = std::make_shared<impl::DynamicValueInstance<T>>(T(t));
			}
			else
			{
				_inst = std::make_shared<impl::LambdaValueInstance<T>>(t);
			}
			return *this;
		}

#define DECLARE_DYNAMIC_OP_2(op) \
		template <class Q = T> \
		auto operator op(DynamicValue<Q> const& o)const \
		{ \
			using _op = decltype([](T const& t, Q const& q){return t op q;}); \
			using RetType = std::invoke_result<_op, T, Q>::type; \
			return DynamicValue<RetType>([=]() -> RetType {return _inst->value() op o.value(); }); \
		}

#define DECLARE_DYNAMIC_OP_1(op) \
		auto operator op(T const& o)const \
		{ \
			using _op = decltype([](T const& t, T const& q) {return t op q; }); \
			using RetType = std::invoke_result<_op, T, T>::type; \
			return DynamicValue<RetType>([=]() -> RetType {return _inst->value() op o; }); \
		}

#define DECLARE_DYNAMIC_OP(op) DECLARE_DYNAMIC_OP_1(op) DECLARE_DYNAMIC_OP_2(op)


		DECLARE_DYNAMIC_OP(+)
		DECLARE_DYNAMIC_OP(-)
		DECLARE_DYNAMIC_OP(*)
		DECLARE_DYNAMIC_OP(/)
		
		DECLARE_DYNAMIC_OP(==)
		DECLARE_DYNAMIC_OP(!=)
		DECLARE_DYNAMIC_OP(<)
		DECLARE_DYNAMIC_OP(<=)
		DECLARE_DYNAMIC_OP(>)
		DECLARE_DYNAMIC_OP(>=)

#undef DECLARE_DYNAMIC_OP
#undef DECLARE_DYNAMIC_OP_1
#undef DECLARE_DYNAMIC_OP_2

		T const& value()const
		{
			assert(hasValue());
			return _inst->value();
		}

		T const& operator*()const
		{
			return value();
		}

		T const& valueOr(T const& other)
		{
			return hasValue() ? value() : other;
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

	template <class T>
	using dv_ = DynamicValue<T>;

}