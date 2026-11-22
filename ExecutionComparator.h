#ifndef EXECUTION_COMPARATOR
#define EXECUTION_COMPARATOR

#include <utility>
#include <array>

#ifdef DISABLE_STRICT_BOOL_ARGS
#define STRICT_BOOL_ARGS 0
#else
#define STRICT_BOOL_ARGS 1
#endif

namespace _ExecutionComparatorHelpers
{
	template<size_t n>
	class Bitset // std::bitset is not constexpr in C++20
	{
		template<size_t n, typename Type, typename... Types>
		struct GeneralizedConditional : GeneralizedConditional<n - 1, Types...>
		{
			static_assert((n > 0 && n < sizeof...(Types) + 1), "Invalid index for the amount of listed types");
		};

		template<typename Type, typename... Types>
		struct GeneralizedConditional<0, Type, Types...>
		{
			using TypeToUse = Type;
		};

		constexpr static unsigned short bitInByte = 8;
		constexpr static unsigned short cppIntTypeAmount = 4;
		constexpr static unsigned short maxBitAmount = (bitInByte << (cppIntTypeAmount - 1));

		// Even 32 will be more than enough, no need to extend to larger sizes
		static_assert(n <= maxBitAmount, "Invalid bit size");

		template<size_t n>
		consteval static unsigned short GetBitValueTypeIndex()
		{
			for (unsigned short i = 0; i < cppIntTypeAmount - 1; ++i)
			{
				if (n < (bitInByte << i))
				{
					return i;
				}
			}
		}

		GeneralizedConditional<
			GetBitValueTypeIndex<n>(),
			unsigned char,     // 8 bits or less
			unsigned short,    // 16 bits or less
			unsigned int,      // 32 bits or less
			unsigned long long // 64 bits or less
		>::TypeToUse bits;

	public:
		constexpr Bitset(size_t number)
		{
			bits = static_cast<decltype(bits)>(number);
		}

		constexpr bool operator[](unsigned short index) const
		{
			if (index < 0 || index > n)
			{
				return false;
			}

			return (bits & (1ull << index));
		}
	};

	constexpr size_t Pow2(size_t k)
	{
		size_t res = 1;

		res <<= k;

		return res;
	}

	template<typename Arg>
	constexpr void AssertCurrentArgIsBool()
	{
		static_assert(std::is_same_v<Arg, bool>, "Not all arguments are bools");
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
	struct FunctionTraits : FunctionTraits<decltype(&std::remove_reference_t<Type>::operator())>{};

	template<std::equality_comparable RetType, typename CallableStructType, typename... Bools>
	struct FunctionTraits<RetType(CallableStructType::*)(Bools...) const>
	{
		using ReturnType = RetType;
		constexpr static size_t boolAmount = GetAmountOfBoolArgs<Bools...>();
	};

	template<std::equality_comparable RetType, typename CallableStructType, typename... Bools>
	struct FunctionTraits<RetType(CallableStructType::*)(Bools...)>
	{
		using ReturnType = RetType;
		constexpr static size_t boolAmount = GetAmountOfBoolArgs<Bools...>();
	};

	template<std::equality_comparable RetType, typename... Bools>
	struct FunctionTraits<RetType(&)(Bools...)>
	{
		using ReturnType = RetType;
		constexpr static size_t boolAmount = GetAmountOfBoolArgs<Bools...>();
	};

	template<size_t boolAmount, typename Func>
	constexpr void AssertCurrentBoolAmount(Func&& func)
	{
		static_assert(
			(boolAmount == FunctionTraits<Func>::boolAmount),
			"Boolean amount is not the same for all functions"
		);
	}

	template<size_t boolAmount, typename Func>
	constexpr void AssertBoolAmountImpl(Func&& func)
	{
		AssertCurrentBoolAmount<boolAmount>(std::forward<Func>(func));
	}

	template<size_t boolAmount, typename Func, typename... Funcs>
	constexpr void AssertBoolAmountImpl(Func&& func, Funcs&&... funcs)
	{
		AssertCurrentBoolAmount<boolAmount>(std::forward<Func>(func));
		AssertBoolAmountImpl<boolAmount>(std::forward<Funcs>(funcs)...);
	}

	template<size_t boolAmount, typename... Funcs>
	constexpr void AssertBoolAmount(Funcs&&... funcs)
	{
		static_assert(boolAmount <= 64, "Algorithm limits amount of bool params to 64");
		AssertBoolAmountImpl<boolAmount>(std::forward<Funcs>(funcs)...);
	}

	template<size_t boolAmount, size_t funcAmount, typename Func, size_t... Indices>
	constexpr void CheckWithBoolSet(
		Func&& func, Bitset<boolAmount> bitset, 
		std::array<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets, size_t i, 
		std::index_sequence<Indices...>
	)
	{
		executionRets[i] = std::forward<Func>(func)(bitset[Indices]...);
	}

	template<size_t boolAmount, size_t funcAmount, typename Func>
	constexpr void CallCheckWithBoolSet(
		Bitset<boolAmount> bitset, std::array<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets, 
		size_t i, Func&& func
	)
	{
		CheckWithBoolSet(std::forward<Func>(func), bitset, executionRets, i, std::make_index_sequence<boolAmount>{});
	}

	template<size_t boolAmount, size_t funcAmount, typename Func, typename... Funcs>
	constexpr void CallCheckWithBoolSet(
		Bitset<boolAmount> bitset, std::array<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets, 
		size_t i, Func&& func, Funcs&&... funcs
	)
	{
		CheckWithBoolSet(std::forward<Func>(func), bitset, executionRets, i, std::make_index_sequence<boolAmount>{});
		CallCheckWithBoolSet<boolAmount>(bitset, executionRets, i + 1, std::forward<Funcs>(funcs)...);
	}

	/*
	* The reason simple == 0 to == funcAmount cannot be reused for new "non bool return type" idea
	* is simply because it was dependent on sum of bool returns and since we can return size_t values now,
	* a simple sum of two max bound values will cause an overflow.
	* Storing each return for biset fixes the issue. I will miss that pretty solution.
	*/
	template<typename Func, size_t funcAmount>
	constexpr bool CheckPureEquivalecy(std::array<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets)
	{
		if (executionRets.size() > 1)
		{
			typename FunctionTraits<Func>::ReturnType firstValue = executionRets[0];

			for (size_t i = 1; i < executionRets.size(); ++i)
			{
				if (firstValue != executionRets[i]) return false;
			}
		}

		return true;
	}
}

template<typename Func, typename... Funcs>
constexpr bool ExecutionComparator(Func&& func, Funcs&&... funcs)
{
	using namespace _ExecutionComparatorHelpers;

	constexpr size_t funcAmount = sizeof...(Funcs) + 1;
	constexpr size_t boolAmount = FunctionTraits<Func>::boolAmount;
	AssertBoolAmount<boolAmount>(std::forward<Funcs>(funcs)...);
	constexpr size_t pow = Pow2(boolAmount);

	std::array<typename FunctionTraits<Func>::ReturnType, funcAmount> executionRets{};

	for (size_t i = 0; i < pow; ++i)
	{
		CallCheckWithBoolSet(Bitset<boolAmount>(i), executionRets, 0, std::forward<Func>(func), std::forward<Funcs>(funcs)...);

		if (!CheckPureEquivalecy<Func>(executionRets)) return false;
	}

	return true;
}

#endif 