
#include <Core/VulkanCommons.hpp>

#include <unordered_set>
#include <set>

template <class T> 
void TestContainerForType()
{
	static_assert(::std::concepts::GenericContainer<std::vector<T>>);
	static_assert(::std::concepts::GenericContainer<std::deque<T>>);
	static_assert(::std::concepts::GenericContainer<std::list<T>>);
	static_assert(::std::concepts::GenericContainer<std::set<T>>);
	static_assert(::std::concepts::GenericContainer<std::unordered_set<T>>);

	static_assert(::std::concepts::Container<std::vector<T>, T>);
	
	static_assert(::std::concepts::GenericSet<std::set<T>>);
	static_assert(::std::concepts::GenericGrowableSet<std::set<T>>);
	static_assert(::std::concepts::GenericSet<std::unordered_set<T>>);
	
}


void LaunchTests()
{
	TestContainerForType<int>();

	static_assert(::std::concepts::StringLike<std::string>);
	static_assert(::std::concepts::StringLike<std::string_view>);
	static_assert(::std::concepts::StringLike<const char *>);
	static_assert(::std::concepts::StringLike<char *>);

	static_assert(::std::concepts::ConvertibleContainer<std::vector<uint32_t>, std::vector<uint64_t>>);
}
