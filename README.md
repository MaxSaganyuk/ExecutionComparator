# ExecutionComparator

Utility that can be used to compare results of executions of any amount functions (callables) checking for all combinations of bool inputs (assuming that each function has the same amount of bool params and bool parameter amount is not exceeding 64, which is extreme amount anyway) completly at compile time. Requires C++20 at least

## Usage
Include the header and use `ExecutionComparator` function like so

```cpp
bool res = ExecutionComparator(...);
```

Where your functions to check (or lambdas) are parameters. 

The function can be used both at runtime and at compile time. To use at compile time all callables which executions will be compared have to be `constexpr`

```cpp
constexpr bool res = ExecutionComparator(...);
```

Concrete example:

```cpp
constexpr bool func1(bool a, bool b)
{
	if (a)
	{
		if (b)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

constexpr bool func2(bool a, bool b)
{
	return (!a || b);
}

constexpr bool func3(bool a, bool b)
{
	return !(a && !b);
}

constexpr bool func4(bool a, bool b, bool c)
{
	return a && b && c;
}

constexpr bool func5(bool a, bool b, bool c)
{
	return a || b || c;
}

struct CustomCallable
{
	constexpr bool operator()(bool a, bool b)
	{
		if (a == b)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
};

int main()
{
	constexpr bool b  = ExecutionComparator(func1, func2, func3); // true
	constexpr bool b2 = ExecutionComparator(func4, func5);        // false

	// You can use lambdas and custom callables as well, mixed together, but not member functions 
	auto lamb = [](bool a,  bool b) { return !(a == b); };

	constexpr bool b3 = ExecutionComparator(lamb, CustomCallable{}, [](bool a, bool b) -> bool { return a ^ b; }); // true
}

```
