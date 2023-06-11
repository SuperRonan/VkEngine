#pragma once
#include <functional>
#include <memory>
#include <cassert>
#include <type_traits>

template <class Q, class T>
concept TLike = std::is_convertible<Q, T>::value;

template<class T>
using LambdaType = std::function<T(void)>;

template <class Q, class T>
concept LambdaLike = std::is_convertible<Q, LambdaType<T>>::value;

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

			virtual T const& value() const
			{
				return _value;
			}

			void setValue(T const& t)
			{
				_value = t;
			}

			void setValue(T && t)
			{
				_value = std::move(t);
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

		using LambdaType = LambdaType<T>;

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

		DynamicValue(const T * ptr):
			_inst(new impl::PointedValueInstance<T>(ptr))
		{}

		DynamicValue(T* ptr) :
			_inst(new impl::PointedValueInstance<T>(ptr))
		{}

		template <TLike<T> Q>
		DynamicValue(DynamicValue<Q> const& q) :
			_inst(new impl::LambdaValueInstance<T>([=]() {return T(q.value()); }))
		{}

		template <TLike<T> Q>
		DynamicValue(Q const& q):
			_inst(std::make_shared<impl::DynamicValueInstance<T>>(T(q)))
		{}

		template <LambdaLike<T> L>
		DynamicValue(L const& l):
			_inst(std::make_shared<impl::LambdaValueInstance<T>>(l))
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

	};

#define DECLARE_DYNAMIC_OP_DD(op) \
		template <class T, class Q = T> \
		auto operator op(DynamicValue<T> const& t, DynamicValue<Q> const& q) \
		{ \
			using _op = decltype([](T const& t, Q const& q){return t op q;}); \
			using RetType = std::invoke_result<_op, T, Q>::type; \
			return DynamicValue<RetType>([=]() -> RetType {return t.value() op q.value(); }); \
		}

#define DECLARE_DYNAMIC_OP_DS(op) \
		template <class T, class Q = T> \
		auto operator op(DynamicValue<T> const& t, Q const& q) \
		{ \
			using _op = decltype([](T const& t, Q const& q) {return t op q; }); \
			using RetType = std::invoke_result<_op, T, Q>::type; \
			return DynamicValue<RetType>([=]() -> RetType {return t.value() op q; }); \
		}

#define DECLARE_DYNAMIC_OP_SD(op) \
		template <class Q, class T = Q> \
		auto operator op(T const& t, DynamicValue<Q> const& q) \
		{ \
			using _op = decltype([](Q const& q, T const& t) {return q op t; }); \
			using RetType = std::invoke_result<_op, Q, T>::type; \
			return DynamicValue<RetType>([=]() -> RetType {return t op q.value(); }); \
		}

#define DECLARE_DYNAMIC_OP(op) DECLARE_DYNAMIC_OP_DD(op) DECLARE_DYNAMIC_OP_SD(op) DECLARE_DYNAMIC_OP_DS(op)


	DECLARE_DYNAMIC_OP(+)
	DECLARE_DYNAMIC_OP(-)
	DECLARE_DYNAMIC_OP(*)
	DECLARE_DYNAMIC_OP(/ )

	DECLARE_DYNAMIC_OP(== )
	DECLARE_DYNAMIC_OP(!= )
	DECLARE_DYNAMIC_OP(< )
	DECLARE_DYNAMIC_OP(<= )
	DECLARE_DYNAMIC_OP(> )
	DECLARE_DYNAMIC_OP(>= )

#undef DECLARE_DYNAMIC_OP
#undef DECLARE_DYNAMIC_OP_DD
#undef DECLARE_DYNAMIC_OP_DS
#undef DECLARE_DYNAMIC_OP_SD

	template <class T>
	using dv_ = DynamicValue<T>;

}