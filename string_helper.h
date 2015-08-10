#pragma once

//#include <string>

template<typename T>
static void StringReplace(T& str, const T& from, const T& to)
{
	size_t pos = 0;
	while ((pos = str.find(from, pos)) != T::npos)
	{
		if (from.length() < to.length())
		{
			str.erase(pos, from.length());
			str.insert(pos, to);
		}
		else
		{
			str.replace(pos, to.length(), to);
			if (from.length() > to.length())
			{
				str.erase(pos + to.length(), from.length() - to.length());
			}
		}
	}
}
