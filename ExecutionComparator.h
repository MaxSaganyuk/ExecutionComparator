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

	template<typename Type>
	struct FunctionTraits;

	template<typename... Bools>
	struct FunctionTraits<bool(*)(Bools...)>
	{
		constexpr static size_t boolAmount = sizeof...(Bools);
	};

	template<size_t boolAmount, typename Func>
	constexpr void AssertCurrentBoolAmount(Func func)
	{
		static_assert(
			(boolAmount == FunctionTraits<Func>::boolAmount) &&
			"Boolean amount is not the same for all functions"
		);
	}

	template<size_t boolAmount, typename Func>
	constexpr void AssertBoolAmountImpl(Func func)
	{
		AssertCurrentBoolAmount<boolAmount>(func);
	}

	template<size_t boolAmount, typename Func, typename... Funcs>
	constexpr void AssertBoolAmountImpl(Func func, Funcs... funcs)
	{
		AssertCurrentBoolAmount<boolAmount>(func);
		AssertBoolAmountImpl<boolAmount>(funcs...);
	}

	template<size_t boolAmount, typename... Funcs>
	constexpr void AssertBoolAmount(Funcs... funcs)
	{
		AssertBoolAmountImpl<boolAmount>(funcs...);
	}

	template<typename Func, typename Array, size_t... Indices>
	constexpr bool CheckWithBoolSet(Func func, Array array, std::index_sequence<Indices...>)
	{
		return func(array[Indices]...);
	}

	template<size_t boolAmount, typename Array, typename Func>
	constexpr size_t CallCheckWithBoolSet(Array array, Func func)
	{
		return CheckWithBoolSet(func, array, std::make_index_sequence<boolAmount>{});
	}

	template<size_t boolAmount, typename Array, typename Func, typename... Funcs>
	constexpr size_t CallCheckWithBoolSet(Array array, Func func, Funcs... funcs)
	{
		return CheckWithBoolSet(func, array, std::make_index_sequence<boolAmount>{}) + 
			   CallCheckWithBoolSet<boolAmount>(array, funcs...);
	}

	template<size_t funcAmount>
	constexpr bool CheckPureEquivalecy(size_t counted)
	{
		return counted == 0 || counted == funcAmount;
	}
}

template<typename Func, typename... Funcs>
constexpr bool ExecutionComparator(Func func, Funcs... funcs)
{
	using namespace _ExecutionComparatorHelpers;

	constexpr size_t funcAmount = sizeof...(Funcs) + 1;
	constexpr size_t boolAmount = FunctionTraits<Func>::boolAmount;
	AssertBoolAmount<boolAmount>(funcs...);
	constexpr size_t pow = Pow(2, boolAmount);
	constexpr auto allBoolCombs = GetAllBoolCombs<boolAmount, pow>();

	for (size_t i = 0; i < pow; ++i)
	{
		if (!CheckPureEquivalecy<funcAmount>(CallCheckWithBoolSet<boolAmount>(allBoolCombs[i], func, funcs...)))
		{
			return false;
		}
	}

	return true;
}