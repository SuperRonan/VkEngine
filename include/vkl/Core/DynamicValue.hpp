#pragma once
#include <functional>
#include <memory>
#include <cassert>
#include <type_traits>
#include <vkl/Utils/stl_extension.hpp>
#include <optional>

template <class Q, class T>
concept TLike = std::is_convertible<Q, T>::value;

template<class T>
using LambdaType = std::function<T(void)>;

template <class Q, class T>
concept LambdaLike = std::is_convertible<Q, LambdaType<T>>::value;

template <class Q, class T, class Op>
concept CombinableWith = requires(Q q, T t, Op op)
{
	op(t, q);
};

// names match std <functional> names
// Binary operators
#define OPERATOR_plus +
#define OPERATOR_minus -
#define OPERATOR_multiplies *
#define OPERATOR_divides /
#define OPERATOR_modulus %
#define OPERATOR_equal_to ==
#define OPERATOR_not_equal_to !=
#define OPERATOR_greater >
#define OPERATOR_less <
#define OPERATOR_greater_equal >=
#define OPERATOR_less_equal <=
#define OPERATOR_logical_and &&
#define OPERATOR_logical_or ||
#define OPERATOR_bit_and &
#define OPERATOR_bit_or |
#define OPERATOR_bit_xor ^
// Unary operators
#define OPERATOR_negate -
#define OPERATOR_logical_not !
#define OPERATOR_bit_not ~

namespace vkl
{
	namespace impl
	{
		class DynamicValueInstanceBase
		{
		public:


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

			T const& getCachedValue() const
			{
				return _value;
			}

			T & getCachedValueRef() 
			{
				return _value;
			}

			virtual T const& value() const
			{
				return getCachedValue();
			}

			void setValue(T const& t)
			{
				assert(canSetValue());
				_value = t;
			}

			void setValue(T && t)
			{
				assert(canSetValue());
				_value = std::move(t);
			}

			virtual bool canSetValue() const
			{
				return true;
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

			virtual T const& value() const override
			{
				DynamicValueInstance<T>::_value = *_ptr;
				return DynamicValueInstance<T>::_value;
			}

			// this one could be true (could be implemented), but it would be very dangerous
			virtual bool canSetValue() const override
			{
				return false;
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

			virtual T const& value() const override
			{
				DynamicValueInstance<T>::_value = _lambda();
				return DynamicValueInstance<T>::_value;
			}

			virtual bool canSetValue() const override
			{
				return false;
			}
		};

		template <class T>
		class LambdaValueInstanceByRef : public DynamicValueInstance<T>
		{
		public:
			using LambdaType = std::function<void(T&)>;

		protected:
			LambdaType _lambda;

		public:

			//template <class Lambda>
			//LambdaValue(Lambda && l):
			//    DynamicValue(),
			//    _lambda(l)
			//{}

			LambdaValueInstanceByRef(LambdaType const& lambda) :
				DynamicValueInstance<T>(),
				_lambda(lambda)
			{}

			virtual T const& value() const override
			{
				_lambda(DynamicValueInstance<T>::_value);
				return DynamicValueInstance<T>::_value;
			}

			virtual bool canSetValue() const override
			{
				return false;
			}
		};
	}

	template <class T>
	class DynamicValue
	{
	protected:

		std::shared_ptr<impl::DynamicValueInstance<T>> _inst = nullptr;

	public:

		using LambdaType = LambdaType<T>;

		constexpr DynamicValue() = default;

		DynamicValue(DynamicValue const& other) :
			_inst(other.instance())
		{}

		DynamicValue(DynamicValue&& other) noexcept :
			_inst(std::move(other._inst))
		{}

		DynamicValue(T const& t):
			_inst(std::make_shared<impl::DynamicValueInstance<T>>(t))
		{}

		DynamicValue(T && t) :
			_inst(std::make_shared<impl::DynamicValueInstance<T>>(std::move(t)))
		{}

		DynamicValue(const T * ptr):
			_inst(std::make_shared<impl::PointedValueInstance<T>>(ptr))
		{}

		DynamicValue(T* ptr) :
			_inst(std::make_shared<impl::PointedValueInstance<T>>(ptr))
		{}

		template <TLike<T> Q>
		DynamicValue(DynamicValue<Q> const& q) :
			_inst(std::make_shared<impl::LambdaValueInstance<T>>([=]() {return T(q.value()); }))
		{}

		template <TLike<T> Q>
		DynamicValue(Q const& q):
			_inst(std::make_shared<impl::DynamicValueInstance<T>>(T(q)))
		{}

		template <LambdaLike<T> L>
		DynamicValue(L const& l):
			_inst(std::make_shared<impl::LambdaValueInstance<T>>(l))
		{}

		template <std::convertible_to<typename impl::LambdaValueInstanceByRef<T>::LambdaType> L>
		DynamicValue(L const& l):
			_inst(std::make_shared<impl::LambdaValueInstanceByRef<T>>(l))
		{}

		template <TLike<T> Q, LambdaLike<Q> L>
		explicit DynamicValue(L const l):
			_inst(std::make_shared<impl::LambdaValueInstance<T>>([=](){ return T(l()); }))
		{}

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

		template <TLike<T> Q>
		DynamicValue& operator=(DynamicValue<Q> const& o)
		{
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

		template <TLike<T> Q>
		DynamicValue& operator=(Q const& q)
		{
			_inst = std::make_shared<impl::DynamicValueInstance<T>>(T(q));
			return *this;
		}

		template <LambdaLike<T> L>
		DynamicValue& operator=(L const& l)
		{
			_inst = std::make_shared<impl::LambdaValueInstance<T>>(l);
			return *this;
		}

		template <std::convertible_to<typename impl::LambdaValueInstanceByRef<T>::LambdaType> L>
		DynamicValue& operator=(L const& l)
		{
			_inst = std::make_shared<impl::LambdaValueInstanceByRef<T>>(l);
			return *this;
		}

		T const& getCachedValue()const
		{
			assert(hasValue());
			return _inst->getCachedValue();
		}

		T& getCachedValueRef()
		{
			assert(hasValue());
			return _inst->getCachedValueRef();
		}

		T const& value()const
		{
			assert(hasValue());
			return _inst->value();
		}

		T const& operator*()const
		{
			return value();
		}

		T const& valueOr(T const& other) const
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

		constexpr operator bool() const
		{
			return hasValue();
		}

		void setValue(T const& t)
		{
			assert(hasValue());
			_inst->setValue(t);
		}

		void setValue(T&& t)
		{
			assert(hasValue());
			_inst->setValue(std::move(t));
		}

		bool canSetValue() const
		{
			return _inst ? _inst->canSetValue() : false;
		}
		
		std::optional<T> optionalValue()const
		{
			return hasValue() ? value() : std::optional<T>{};
		}

		operator std::optional<T>() const
		{
			return optionalValue();
		}
	};

#define DECLARE_BINARY_OPERATOR_WRAPPER(op_name) \
		template <class T, class Q> \
		struct BinaryOperator_ ## op_name \
		{ \
			auto operator()(T && t, Q && q) \
			{ \
				return std::forward<T>(t) OPERATOR_ ## op_name std::forward<Q>(q); \
			} \
			using Type = typename std::invoke_result<BinaryOperator_ ## op_name, T, Q>::type; \
		};

#define DECLARE_UNARY_OPERATOR_WRAPPER(op_name) \
		template <class T> \
		struct UnaryOperator_ ## op_name \
		{ \
			auto operator()(T && t) \
			{ \
				return OPERATOR_ ## op_name std::forward<T>(t); \
			}\
			using Type = typename std::invoke_result<UnaryOperator_ ## op_name, T>::type; \
		};

#define DECLARE_DYNAMIC_BINRARY_OP_DD(op_name) \
		template <class T, class Q> \
		auto operator OPERATOR_##op_name(DynamicValue<T> const& t, DynamicValue<Q> const& q) \
		{ \
			using RetType = typename BinaryOperator_ ## op_name <T, Q> :: Type; \
			return DynamicValue<RetType>([=]() -> RetType {return t.value() OPERATOR_##op_name q.value(); }); \
		}

#define DECLARE_DYNAMIC_BINRARY_OP_DS(op_name) \
		template <class T, class Q = T> \
		auto operator OPERATOR_##op_name(DynamicValue<T> const& t, Q const& q) \
		{ \
			using RetType = typename BinaryOperator_ ## op_name <T, Q> :: Type; \
			return DynamicValue<RetType>([=]() -> RetType {return t.value() OPERATOR_##op_name q; }); \
		}

#define DECLARE_UNARY_OP_D(op_name) \
		template<class T> \
		auto operator OPERATOR_ ## op_name(DynamicValue<T> const& t) \
		{ \
			using RetType = typename UnaryOperator_ ## op_name <T> :: Type; \
			return DynamicValue<RetType>([=]() -> RetType {return OPERATOR_ ## op_name t.value(); }); \
		}

/*
#define DECLARE_DYNAMIC_BINRARY_OP_SD(op_name) \
		template <class Q, class T = Q> \
		auto operator OPERATOR_##op_name(T const& t, DynamicValue<Q> const& q) \
		{ \
			using RetType = typename std::invoke_result<std::op_name, Q, T>::type; \
			return DynamicValue<RetType>([=]() -> RetType {return t OPERATOR_##op_name q.value(); }); \
		}
*/

#define DECLARE_DYNAMIC_BINRARY_OP(op_name) DECLARE_BINARY_OPERATOR_WRAPPER(op_name) DECLARE_DYNAMIC_BINRARY_OP_DD(op_name) DECLARE_DYNAMIC_BINRARY_OP_DS(op_name) //DECLARE_DYNAMIC_OP_SD(op_name) 

#define DECLARE_DYNAMIC_UNARY_OP(op_name) DECLARE_UNARY_OPERATOR_WRAPPER(op_name) DECLARE_UNARY_OP_D(op_name)

	DECLARE_DYNAMIC_BINRARY_OP(plus)
	DECLARE_DYNAMIC_BINRARY_OP(minus)
	DECLARE_DYNAMIC_BINRARY_OP(multiplies)
	DECLARE_DYNAMIC_BINRARY_OP(divides)
	DECLARE_DYNAMIC_BINRARY_OP(modulus)

	DECLARE_DYNAMIC_BINRARY_OP(equal_to)
	DECLARE_DYNAMIC_BINRARY_OP(not_equal_to)
	DECLARE_DYNAMIC_BINRARY_OP(less)
	DECLARE_DYNAMIC_BINRARY_OP(less_equal)
	DECLARE_DYNAMIC_BINRARY_OP(greater)
	DECLARE_DYNAMIC_BINRARY_OP(greater_equal)

	DECLARE_DYNAMIC_BINRARY_OP(logical_and)
	DECLARE_DYNAMIC_BINRARY_OP(logical_or)
	
	DECLARE_DYNAMIC_BINRARY_OP(bit_and)
	DECLARE_DYNAMIC_BINRARY_OP(bit_or)
	DECLARE_DYNAMIC_BINRARY_OP(bit_xor)

	DECLARE_DYNAMIC_UNARY_OP(negate)
	DECLARE_DYNAMIC_UNARY_OP(logical_not)
	DECLARE_DYNAMIC_UNARY_OP(bit_not)


//#undef DECLARE_DYNAMIC_OP
//#undef DECLARE_DYNAMIC_OP_DD
//#undef DECLARE_DYNAMIC_OP_DS
//#undef DECLARE_DYNAMIC_OP_SD

	template <class T>
	using dv_ = DynamicValue<T>;

	template <class T>
	using Dyn = DynamicValue<T>;

}