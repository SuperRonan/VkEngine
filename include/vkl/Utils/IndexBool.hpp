#include <concepts>

namespace vkl
{
	template <std::unsigned_integral Index = uint32_t>
	class IndexBool
	{
	protected:
		Index _data;
		static constexpr Index BoolBit = (Index(1) << Index(sizeof(Index) * 8 - 1));
	public:

		constexpr IndexBool(Index index = 0, bool boolean = false) :
			_data(index | (boolean ? BoolBit : Index(0)))
		{
		}

		constexpr Index index() const
		{
			return _data & ~BoolBit;
		}

		constexpr bool boolean() const
		{
			return bool(_data & BoolBit);
		}

		constexpr void setIndex(Index index)
		{
			_data = index | (_data & BoolBit);
		}

		constexpr void setBoolean(bool value)
		{
			if (value)
			{
				_data |= BoolBit;
			}
			else
			{
				_data &= ~BoolBit;
			}
		}
	};
}