#ifndef EXECUTION_COMPARATOR
#define EXECUTION_COMPARATOR

#include <utility>
#include <array>
#include <algorithm>
#include <iostream>

#ifdef DISABLE_STRICT_BOOL_ARGS
#define STRICT_BOOL_ARGS 0
#else
#define STRICT_BOOL_ARGS 1
#endif

namespace _ExecutionComparatorHelpers
{
	template<typename Type>
	struct FunctionTraits : FunctionTraits<decltype(&std::remove_reference_t<Type>::operator())> {};

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

}

template<typename Type, size_t funcAmount>
struct FuncsReturns : public std::array<Type, funcAmount>
{
	/*
	* The reason simple == 0 to == funcAmount cannot be reused for new "non bool return type" idea
	* is simply because it was dependent on sum of bool returns and since we can return size_t values now,
	* a simple sum of two max bound values will cause an overflow.
	* Storing each return for biset fixes the issue. I will miss that pretty solution.
	*/
	constexpr operator bool() const
	{
		if (this->size() > 1)
		{
			auto& firstValue = this->operator[](0);

			for (size_t i = 1; i < this->size(); ++i)
			{
				if (firstValue != this->operator[](i)) return false;
			}
		}

		return true;
	}
};

// Note that resulting array has size of 2^boolAmount since the intent is to hold all combinations under the bool amount
template<typename Type, size_t funcAmount, size_t boolAmount>
struct ExecutionResult : 
	public std::array<FuncsReturns<Type, funcAmount>, _ExecutionComparatorHelpers::Pow2(boolAmount)>
{
	constexpr operator bool() const
	{
		return !std::any_of(this->begin(), this->end(), [](bool value) { return !value; });
	}
};


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
	public:
		using UnderlyingType =
		typename GeneralizedConditional<
			GetBitValueTypeIndex<n>(),
			unsigned char,     // 8 bits or less
			unsigned short,    // 16 bits or less
			unsigned int,      // 32 bits or less
			unsigned long long // 64 bits or less
		>::TypeToUse;
	private:
		UnderlyingType bits;
	public:
		constexpr Bitset(size_t number)
		{
			bits = static_cast<decltype(bits)>(number);
		}

		constexpr bool operator[](unsigned short index) const
		{
			if (index < 0 || index >= n) return false;

			return (bits & ((1ull << (n - 1)) >> index));
		}

		constexpr operator auto() const
		{
			return bits;
		}

		constexpr auto& operator++()
		{
			++bits;
			return *this;
		}

		constexpr auto operator++(int)
		{
			bits++;
			return this;
		}

		constexpr auto& operator--()
		{
			--bits;
			return *this;
		}

		constexpr auto operator--(int)
		{
			bits--;
			return this;
		}
	};

	template<size_t boolAmount, size_t funcAmount, typename Func, size_t... Indices>
	constexpr void CheckWithBoolSet(
		Func&& func, Bitset<boolAmount> bitset, 
		FuncsReturns<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets, size_t i,
		std::index_sequence<Indices...>
	)
	{
		executionRets[i] = std::forward<Func>(func)(bitset[Indices]...);
	}

	template<size_t boolAmount, size_t funcAmount, typename Func>
	constexpr void CallCheckWithBoolSet(
		Bitset<boolAmount> bitset, FuncsReturns<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets,
		size_t i, Func&& func
	)
	{
		CheckWithBoolSet(std::forward<Func>(func), bitset, executionRets, i, std::make_index_sequence<boolAmount>{});
	}

	template<size_t boolAmount, size_t funcAmount, typename Func, typename... Funcs>
	constexpr void CallCheckWithBoolSet(
		Bitset<boolAmount> bitset, FuncsReturns<typename FunctionTraits<Func>::ReturnType, funcAmount>& executionRets,
		size_t i, Func&& func, Funcs&&... funcs
	)
	{
		CheckWithBoolSet(std::forward<Func>(func), bitset, executionRets, i, std::make_index_sequence<boolAmount>{});
		CallCheckWithBoolSet<boolAmount>(bitset, executionRets, i + 1, std::forward<Funcs>(funcs)...);
	}
}

template<bool evaluateAll = false, typename Func, typename... Funcs>
constexpr auto ExecutionComparator(Func&& func, Funcs&&... funcs)
{
	using namespace _ExecutionComparatorHelpers;

	constexpr size_t funcAmount = sizeof...(Funcs) + 1;
	constexpr size_t boolAmount = FunctionTraits<Func>::boolAmount;
	AssertBoolAmount<boolAmount>(std::forward<Funcs>(funcs)...);

	ExecutionResult<typename FunctionTraits<Func>::ReturnType, funcAmount, boolAmount> executionRes{};

	for (size_t i = 0; i < executionRes.size(); ++i)
	{
		CallCheckWithBoolSet(
			Bitset<boolAmount>(i), executionRes[i], 0, std::forward<Func>(func), std::forward<Funcs>(funcs)...
		);

		if constexpr (!evaluateAll)
		{
			if (!executionRes[i]) return false;
		}
	}

	if constexpr (evaluateAll)
	{
		return executionRes;
	}
	else
	{
		return true;
	}
}

/*
* For now only runtime version of ExecutionResult processing for std::cout out of the box.
* Alter. version would include fixed string collecting the result but that seems only interesting to do
* once C++26 comes along so it can be displayed in static_assert and would also require
* adding my FixedString implementation (or different one) from another repo, which is also an overkill, I feel.
*/
template<typename Type, size_t funcAmount, size_t boolAmount>
void DisplayExecutionResult(const ExecutionResult<Type, funcAmount, boolAmount>& executionRes)
{
	using namespace _ExecutionComparatorHelpers;

	for (Bitset<boolAmount> i{0}; i < executionRes.size(); ++i)
	{
		std::cout << static_cast<size_t>(i) << " - ";

		for (typename Bitset<boolAmount>::UnderlyingType j = 0; j < boolAmount; ++j)
		{
			std::cout << (i[j] ? '1' : '0') << ' ';
		}

		std::cout << "- ";

		for (size_t j = 0; j < executionRes[i].size(); ++j)
		{
			std::cout << executionRes[i][j] << ' ';
		}

		std::cout << '\n';
	}
}

#endif 