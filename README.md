# ExecutionComparator

Utility that can be used to compare results of executions of any amount functions checking for all combinations of bool inputs in any amount (assuming that each function has the same amount of bool params) completly at compile time

## Usage
Include the header and use `ExecutionComparator` function like so

```cpp
bool res = ExecutionComparator(...);
```

Where you function names are parameters. 

The function can be used both at runtime and at compile time. To use at compile time all functions which executions will be compared has to be `constexpr`

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

constexpr bool func4(bool a, bool b, bool c)
{
	return a && b && c;
}

constexpr bool func5(bool a, bool b, bool c)
{
	return a || b || c;
}

int main()
{
	constexpr bool b  = ExecutionComparator(func1, func2, func3); // true
	constexpr bool b2 = ExecutionComparator(func4, func5);        // false
}

```
