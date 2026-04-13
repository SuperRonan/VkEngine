#pragma once

#include <vkl/GUI/ObjectInlineInspector.hpp>

namespace vkl::GUI
{
	// Specialized ObjectInlineInspector with type checking
	template <std::derived_from<VkObject> Target>
	class TypedInlineInspector : public ObjectInlineInspector
	{
	public:

		using AcceptTypedFnPtr = AcceptObjectResult(*)(const Target* current, const Target* incoming, const void* data);

	protected:
		AcceptTypedFnPtr _typed_accept_fn = nullptr;
		const void* _typed_accept_data = nullptr;

		void setTypeCheckAcceptFunction()
		{
			AcceptObjectFnPtr to_set = [](const VkObject* current, const VkObject* incoming, const void* data)
				{
					AcceptObjectResult res = {};
					res.accept = true;
					const Target* casted = nullptr;
					if (incoming)
					{
						casted = dynamic_cast<const Target*>(incoming);
						if (!casted)
						{
							res.accept = false;
							res.invalid_type = false;
							res.reason = "Incompatible type";
						}
					}
					if (res.accept)
					{
						const TypedInlineInspector* that = reinterpret_cast<const TypedInlineInspector*>(data);
						if (that->_typed_accept_fn)
						{
							res = that->_typed_accept_fn(static_cast<const Target*>(current), casted, that->_typed_accept_data);
						}
					}
					return res;
				};
			ObjectInlineInspector::setAcceptFunction(to_set, this);
		}
	public:

		TypedInlineInspector(std::string_view label = {}):
			ObjectInlineInspector(label)
		{
			setTypeCheckAcceptFunction();
		}

		TypedInlineInspector(TypedInlineInspector const& other):
			ObjectInlineInspector(other),
			_typed_accept_fn(other._typed_accept_fn),
			_typed_accept_data(other._typed_accept_data)
		{
			setTypeCheckAcceptFunction();
		}

		TypedInlineInspector(TypedInlineInspector&& other) noexcept :
			ObjectInlineInspector(std::move(other)),
			_typed_accept_fn(other._typed_accept_fn),
			_typed_accept_data(other._typed_accept_data)
		{
			setTypeCheckAcceptFunction();
		}

		~TypedInlineInspector() = default;

		void setAcceptFunction(AcceptTypedFnPtr fn, const void* data = nullptr)
		{
			_typed_accept_fn = fn;
			_typed_accept_data = data;
		}
	};
}