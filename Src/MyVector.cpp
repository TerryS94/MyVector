#include <memory>
#include <string>
#include <random> //for .shuffle() method
#include <iostream>

namespace MyVector
{
	template<typename T>
	class vector
	{
	private:
		size_t totalMemoryInUse = 0u;
		size_t blockSize = 0u;
		size_t elementCount = 0u;
		T* block = nullptr;

		inline void grow_if_needed()
		{
			if (elementCount >= blockSize)
			{
				size_t oldCap = blockSize;
				size_t newCap = blockSize ? (blockSize + (blockSize / 2u)) : 1u;
				if (newCap == oldCap) newCap = oldCap + 1u;
				size_t addedElems = newCap - oldCap;
				reserve(newCap);
				//printf("reserved %zu more elements. current elementCount = %zu\n", addedElems, elementCount);
			}
		}

		inline void swap(vector& other) noexcept
		{
			std::swap(totalMemoryInUse, other.totalMemoryInUse);
			std::swap(blockSize, other.blockSize);
			std::swap(elementCount, other.elementCount);
			swap(block, other.block);
		}

	public:
		inline vector() noexcept
		{
			blockSize = 4u;
			block = static_cast<T*>(::operator new(blockSize * sizeof(T)));
			totalMemoryInUse += blockSize * sizeof(T);
			elementCount = 0u;
		}
		inline ~vector() { clear(); }

		inline vector& operator=(const vector& other)
		{
			if (this == &other) return *this;
			vector tmp(other);
			swap(tmp);
			return *this;
		}

		inline vector(vector&& other) noexcept
			: totalMemoryInUse(other.totalMemoryInUse), blockSize(other.blockSize), elementCount(other.elementCount), block(other.block)
		{
			other.totalMemoryInUse = 0u;
			other.blockSize = 0u;
			other.elementCount = 0u;
			other.block = nullptr;
		}

		inline vector& operator=(vector&& other) noexcept
		{
			if (this == &other) return *this;
			clear();
			totalMemoryInUse = other.totalMemoryInUse;
			blockSize = other.blockSize;
			elementCount = other.elementCount;
			block = other.block;
			other.totalMemoryInUse = 0u;
			other.blockSize = 0u;
			other.elementCount = 0u;
			other.block = nullptr;
			return *this;
		}

		inline vector(const vector& other) : totalMemoryInUse(0u), blockSize(0u), elementCount(0u), block(nullptr)
		{
			if (other.blockSize == 0u) return;
			T* newStorage = static_cast<T*>(::operator new(other.blockSize * sizeof(T)));
			size_t i = 0u;
			try
			{
				for (; i < other.elementCount; ++i) new (static_cast<void*>(newStorage + i)) T(other.block[i]);
				block = newStorage;
				blockSize = other.blockSize;
				elementCount = other.elementCount;
				totalMemoryInUse = blockSize * sizeof(T);
			}
			catch (...)
			{
				for (size_t j = 0u; j < i; ++j) newStorage[j].~T();
				::operator delete(static_cast<void*>(newStorage));
				throw;
			}
		}


		[[nodiscard]] inline size_t size() const noexcept
		{
			return elementCount;
		}

		//total memory allocated for this container
		[[nodiscard]] inline size_t total_memory() noexcept
		{
			return totalMemoryInUse;
		}

		//remove the extra allocated memory that was tagged onto the end when the container grew by 50%
		inline void shrink_to_fit()
		{
			if (elementCount == 0u)
			{
				::operator delete(static_cast<void*>(block));
				block = nullptr;
				blockSize = 0u;
				totalMemoryInUse = 0u;
				return;
			}

			T* newStorage = static_cast<T*>(::operator new(elementCount * sizeof(T)));
			for (size_t i = 0u; i < elementCount; ++i)
			{
				new (newStorage + i) T(std::move(block[i]));
				block[i].~T();
			}
			::operator delete(static_cast<void*>(block));
			block = newStorage;
			blockSize = elementCount;
			totalMemoryInUse = blockSize * sizeof(T);
		}

		inline void reserve(size_t newCapacity)
		{
			if (newCapacity <= blockSize) return;
			if (newCapacity < elementCount) newCapacity = elementCount;
			T* newStorage = static_cast<T*>(::operator new(newCapacity * sizeof(T)));
			for (size_t i = 0u; i < elementCount; ++i)//move our old elements into the new storage
			{
				new(static_cast<void*>(newStorage + i)) T(std::move(block[i]));
				block[i].~T();//now destroy the old object
			}
			::operator delete(static_cast<void*>(block));
			block = newStorage;
			blockSize = newCapacity;
			totalMemoryInUse = blockSize * sizeof(T);
		}

		inline void push_back(const T& data)
		{
			grow_if_needed();
			new(static_cast<void*>(block + elementCount)) T(data);
			++elementCount;
		}

		inline void push_back(T&& data)
		{
			grow_if_needed();
			new(static_cast<void*>(block + elementCount)) T(std::move(data));
			++elementCount;
		}

		template<typename... Args>
		inline void emplace_back(Args&&... args)
		{
			grow_if_needed();
			new(static_cast<void*>(block + elementCount)) T(std::forward<Args>(args)...);
			++elementCount;
		}

		inline void pop_back()
		{
			if (elementCount == 0u) return;
			--elementCount;
			block[elementCount].~T();
		}

		inline void erase(size_t index)
		{
			if (elementCount == 0u || index >= elementCount)
			{
				//MessageBox(nullptr, "Tried to access an out of bounds element!", "F", MB_OK);
				//ExitProcess(EXIT_FAILURE);
			}
			block[index].~T();
			for (size_t i = index; i + 1u < elementCount; ++i)
			{
				new(static_cast<void*>(block + i)) T(std::move(block[i + 1u]));
				block[i + 1u].~T();
			}
			--elementCount;
		}

		inline void erase(size_t start, size_t end)
		{
			if (elementCount == 0u || start >= end || end > elementCount)
			{
				//MessageBox(nullptr, "Tried to access an out of bounds element!", "F", MB_OK);
				//ExitProcess(EXIT_FAILURE);
			}

			size_t count = end - start;
			size_t tail = elementCount - end;

			if (tail > 0u)
			{
				for (size_t i = 0; i < tail; ++i)
				{
					block[start + i].~T();
					new (static_cast<void*>(block + start + i)) T(std::move(block[end + i]));
					block[end + i].~T();
				}
			}
			else
				for (size_t i = start; i < elementCount; ++i) block[i].~T();

			elementCount -= count;
		}

		inline void shuffle()
		{
			if (elementCount <= 1u) return;
			static thread_local std::mt19937 rng(std::random_device {}());
			for (size_t i = elementCount - 1u; i > 0u; --i)
			{
				std::uniform_int_distribution<size_t> dist(0, i);
				const size_t j = dist(rng);
				std::swap(block[i], block[j]);
			}
		}

		inline void reverse()
		{
			if (elementCount <= 1u) return;
			for (size_t left = 0u, right = elementCount - 1u; left < right;)
			{
				std::swap(block[left], block[right]);
				++left; --right;
			}
		}

		[[nodiscard]] inline T& operator[](size_t index) noexcept
		{
			if (index >= elementCount)
			{
				//MessageBox(nullptr, "Tried to access an out of bounds element!", "F", MB_OK);
				//ExitProcess(EXIT_FAILURE);
			}
			return block[index];
		}

		[[nodiscard]] inline T& at(size_t index) const
		{
			if (index >= elementCount)
			{
				//MessageBox(nullptr, "Tried to access an out of bounds element!", "F", MB_OK);
				//ExitProcess(EXIT_FAILURE);
			}
			return block[index];
		}

		inline void clear()
		{
			for (size_t i = 0u; i < elementCount; ++i) block[i].~T();
			::operator delete(static_cast<void*>(block));
			block = nullptr;
			elementCount = 0u;
			blockSize = 0u;
		}

		[[nodiscard]] inline bool empty()
		{
			return elementCount == 0;
		}
	};
}

struct data
{
	size_t var1 {};
	std::string var2 {};
	int var3 {};
	int var4 {};
	int var5 {};
};

int main()
{
	std::cout << "starting..." << "\n";
	{
		MyVector::vector<data> container {};
		const bool print_container = true, reserve_memory = false;
		const size_t reserve_capacity = 13u;
		if (reserve_memory) container.reserve(reserve_capacity);

		for (size_t i = 0u; i < reserve_capacity; ++i) container.emplace_back(i, "hello", 420, 69, 666);
		if (!container.empty())
		{
			for (size_t i = 0u; i < 5u; ++i) container.pop_back();

			data d = data(100, "push_back copy", 420, 69, 666);
			container.push_back(d);
			container.push_back(data(101, "push_back move", 420, 69, 666));

			container.shuffle();
			container.reverse();

			std::cout << "Total memory used for container: " << container.total_memory() << std::endl;
			if (!reserve_memory)
			{
				container.shrink_to_fit(); std::cout << "-Shrink to Fit called-" << "\n";
				std::cout << "Total memory used for container: " << container.total_memory() << std::endl;
			}
			if (print_container)
			{
				MyVector::vector<data> container_copy = container;
				for (size_t i = 0u; i < container_copy.size(); ++i)
				{
					const data& d = container_copy[i];
					std::cout << "var1: " << d.var1 << "   -   var2: " << d.var2 << "   -   var3: " << d.var3 << "   -   var4: " << d.var4 << "   -   var5: " << d.var5 << "\n";
				}
			}
		}
		else std::cout << "container was empty!" << "\n";
	}

	std::cout << "\npress Enter key to exit the main function" << "\n";
	std::cin.get();
	return 0;
}