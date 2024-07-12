#include <iostream>
#include <array>

namespace _ExecutionComparatorHelpers
{
	constexpr size_t Pow(size_t n, size_t k)
	{
		if (!k)
		{
			return 1;
		}

		size_t res = n;

		for (size_t i = 1; i < k; ++i)
		{
			res *= n;
		}

		return res;
	}

	template<size_t n, size_t pow>
	constexpr auto GetAllBoolCombs()
	{
		static_assert(n < 64 && "Algorithm limits amount of bool params to 64");

		std::array<std::array<bool, n>, pow> allBoolCombs;

		for (size_t comb = 0; comb < pow; ++comb)
		{
			size_t comparator = 1;
			for (size_t j = 0; j < n; ++j)
			{
				allBoolCombs[comb][j] = comb & comparator;
				comparator <<= 1;
			}
		}

		return allBoolCombs;
	}

	template<typename... Bools, typename Array, size_t... Indices>
	constexpr bool CheckWithBoolSet(bool(*func)(Bools...), Array array, std::index_sequence<Indices...>)
	{
		return func(array[Indices]...);
	}
}

template<typename... Bools>
constexpr bool CompareConditionals(bool(*func1)(Bools...), bool(*func2)(Bools...))
{
	using namespace _ExecutionComparatorHelpers;

	constexpr size_t n = sizeof...(Bools);
	constexpr size_t pow = Pow(2, n);
	constexpr auto allBoolCombs = GetAllBoolCombs<n, pow>();

	for (size_t i = 0; i < pow; ++i)
	{
		if (
			CheckWithBoolSet(func1, allBoolCombs[i], std::make_index_sequence<n>{}) !=
			CheckWithBoolSet(func2, allBoolCombs[i], std::make_index_sequence<n>{})
			)
		{
			return false;
		}
	}

	return true;
}