#pragma once
#include <string_view>
#include <stdexcept>

template <size_t Size>
class FixedString
{
	static_assert(Size > 0, "FixedString size is not valid");

private:
	size_t size;
	char buffer[Size];

public:
	FixedString() : size(0), buffer{}
	{}

	FixedString(const FixedString& other)
	{
		size = other.size;
		memcpy(buffer, other.buffer, size + 1);
	}

	FixedString(FixedString&& other)
	{
		size = other.size;
		memcpy(buffer, other.buffer, size + 1);
		other.size = 0;
	}

	FixedString& operator=(const FixedString& other)
	{
		size = other.size;
		memcpy(buffer, other.buffer, size + 1);
		return *this;
	}

	FixedString& operator=(FixedString&& other)
	{
		size = other.size;
		memcpy(buffer, other.buffer, size + 1);
		other.size = 0;
		return *this;
	}

	FixedString(const std::string_view& other)
	{
		size = other.size();
		if (size >= Size)
			size = Size - 1;
		memcpy(buffer, other.data(), size);
		buffer[size] = 0;
	}

	FixedString& operator=(const std::string_view& other)
	{
		size = other.size();
		if (size >= Size)
			size = Size - 1;
		memcpy(buffer, other.data(), size);
		buffer[size] = 0;
		return *this;
	}

	char& operator[](size_t i)
	{
		if (i >= size)
			throw std::out_of_range("access out of range");

		return buffer[i];
	}

	char* operator&()
	{
		return buffer;
	}

	operator std::string_view()
	{
		return { buffer, size };
	}

	void clear()
	{
		size = 0;
		buffer[0] = 0;
	}
};
