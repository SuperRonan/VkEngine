#pragma once

#include <utility>
#include <iterator>


namespace std
{
	namespace concepts
	{
		// from https://stackoverflow.com/questions/60449592/how-do-you-define-a-c-concept-for-the-standard-library-containers
		template <class ContainerType>
		concept GenericContainer = requires(ContainerType a, const ContainerType b)
		{
			requires std::regular<ContainerType>;
			requires std::swappable<ContainerType>;
			requires std::destructible<typename ContainerType::value_type>;
			requires std::same_as<typename ContainerType::reference, typename ContainerType::value_type&>;
			requires std::same_as<typename ContainerType::const_reference, const typename ContainerType::value_type&>;
			requires std::forward_iterator<typename ContainerType::iterator>;
			requires std::forward_iterator<typename ContainerType::const_iterator>;
			requires std::signed_integral<typename ContainerType::difference_type>;
			requires std::same_as<typename ContainerType::difference_type, typename std::iterator_traits<typename ContainerType::iterator>::difference_type>;
			requires std::same_as<typename ContainerType::difference_type, typename std::iterator_traits<typename ContainerType::const_iterator>::difference_type>;
			{ a.begin() } -> std::same_as<typename ContainerType::iterator>;
			{ a.end() } -> std::same_as<typename ContainerType::iterator>;
			{ b.begin() } -> std::same_as<typename ContainerType::const_iterator>;
			{ b.end() } -> std::same_as<typename ContainerType::const_iterator>;
			{ a.cbegin() } -> std::same_as<typename ContainerType::const_iterator>;
			{ a.cend() } -> std::same_as<typename ContainerType::const_iterator>;
			{ a.size() } -> std::same_as<typename ContainerType::size_type>;
			{ a.max_size() } -> std::same_as<typename ContainerType::size_type>;
			{ a.empty() } -> std::same_as<bool>;
		};
		template <class ContainerTypeMaybeRef>
		concept GenericContainerMaybeRef = GenericContainer<typename std::remove_reference<ContainerTypeMaybeRef>::type>;

		template <class ContainerType, class T>
		concept Container = requires(ContainerType c)
		{
			requires GenericContainer<ContainerType>;
			requires std::is_same<T, typename ContainerType::value_type>::value;
		};
		template <class ContainerTypeMaybeRef, class T>
		concept ContainerMaybeRef = Container<typename std::remove_reference<ContainerTypeMaybeRef>::type, T>;

		template <class ContainerType, class ReferenceContainerType>
		concept ConvertibleContainer = requires
		{
			requires GenericContainer<ReferenceContainerType>;
			requires GenericContainer<ContainerType>;
			requires std::convertible_to<typename ContainerType::value_type, typename ReferenceContainerType::value_type>;
		};
		template <class ContainerTypeMaybeRef, class ReferenceContainerType>
		concept ConvertibleContainerMaybeRef = ConvertibleContainer<typename std::remove_reference<ContainerTypeMaybeRef>::type, ReferenceContainerType>;

		template <class ContainerType>
		concept GenericGrowableContainer = requires(ContainerType c, typename ContainerType::value_type v)
		{
			requires GenericContainer<ContainerType>;
			{std::back_inserter(c)};
			{c.push_back(v)};
			{c.push_back(std::move(v))};
			//{c.emplace_back()}; // Assume this one, don't know how to express it right now
		};
		template <class ContainerType>
		concept GenericGrowableContainerMaybeRef = GenericGrowableContainer<typename std::remove_reference<ContainerType>::type>;

		template <class ContainerType, class T>
		concept GrowableContainer = requires(ContainerType c)
		{
			requires GenericGrowableContainer<ContainerType>;
			requires std::is_same<T, typename ContainerType::value_type>::value;
		//requires Container<ContainerType, T>;
		};
		template <class ContainerType, class T>
		concept GrowableContainerMaybeRef = GrowableContainer<typename std::remove_reference<ContainerType>::type, T>;
	}
	

	// TODO some tests
}