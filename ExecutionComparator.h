#include <array>

#ifdef DISABLE_STRICT_BOOL_ARGS
#define STRICT_BOOL_ARGS 0
#else
#define STRICT_BOOL_ARGS 1
#endif

namespace _ExecutionComparatorHelpers
{
	constexpr size_t Pow2(size_t k)
	{
		size_t res = 1;

		res <<= (k - 1);

		return res;
	}

	template<size_t n, size_t pow>
	constexpr auto GetAllBoolCombs()
	{
		std::array<std::array<bool, n>, pow> allBoolCombs{};

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

	template<typename Arg>
	constexpr void AssertCurrentArgIsBool()
	{
		static_assert(std::is_same_v<Arg, bool> && "Not all arguments are bools");
	}

	template<typename Arg, typename... Args>
	constexpr void AssertArgsAreBools()
	{
		AssertCurrentArgIsBool<Arg>();

		if constexpr (sizeof...(Args) != 0)
		{
			AssertArgsAreBools<Args...>();
		}
	}

	template<typename... Bools>
	constexpr size_t GetAmountOfBoolArgs()
	{
		if constexpr (STRICT_BOOL_ARGS)
		{
			AssertArgsAreBools<Bools...>();
		}

		return sizeof...(Bools);
	}

	template<typename Type>
	struct FunctionTraits : FunctionTraits<decltype(&Type::operator())>{};

	template<typename CallableStructType, typename... Bools>
	struct FunctionTraits<bool(CallableStructType::*)(Bools...) const>
	{
		constexpr static size_t boolAmount = GetAmountOfBoolArgs<Bools...>();
	};

	template<typename CallableStructType, typename... Bools>
	struct FunctionTraits<bool(CallableStructType::*)(Bools...)>
	{
		constexpr static size_t boolAmount = GetAmountOfBoolArgs<Bools...>();
	};

	template<typename... Bools>
	struct FunctionTraits<bool(*)(Bools...)>
	{
		constexpr static size_t boolAmount = GetAmountOfBoolArgs<Bools...>();
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
		static_assert(boolAmount < 64 && "Algorithm limits amount of bool params to 64");
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
	constexpr size_t pow = Pow2(boolAmount);
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