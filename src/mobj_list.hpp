// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef mobj_list_hpp
#define mobj_list_hpp

#include <type_traits>

#include "cxxutil.hpp"
#include "mobj.hpp"
#include "mobj_list_view.hpp"

namespace srb2
{

// Requires:
//   void T::next(T*)
//   T*   T::next() const
template <typename T, mobj_t*& Head>
struct MobjList
{
	static_assert(std::is_convertible_v<T, mobj_t>);

	MobjList() {}

	T* front() const { return static_cast<T*>(Head); }
	bool empty() const { return !front(); }

	void push_front(T* ptr)
	{
		ptr->next(front());
		front(ptr);
	}

	void erase(T* node)
	{
		if (front() == node)
		{
			front(node->next());
			node->next(nullptr);
			return;
		}

		auto view = this->view();
		auto end = view.end();
		auto it = view.begin();

		SRB2_ASSERT(it != end);

		for (;;)
		{
			T* prev = *it;

			it++;

			if (it == end)
			{
				break;
			}

			if (*it == node)
			{
				prev->next(node->next());
				node->next(nullptr);
				break;
			}
		}
	}

	void clear()
	{
		while (!empty())
		{
			erase(front());
		}
	}

	auto begin() const { return view().begin(); }
	auto end() const { return view().end(); }

private:
	void front(T* ptr) { Mobj::ManagedPtr {Head} = ptr; }
	auto view() const { return MobjListView(front(), [](T* node) { return node->next(); }); }
};

}; // namespace srb2

#endif/*mobj_list_hpp*/
