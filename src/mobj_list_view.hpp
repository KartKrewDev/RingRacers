// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef mobj_list_view_hpp
#define mobj_list_view_hpp

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "p_mobj.h"

namespace srb2
{

// for (T* ptr : MobjList(hnext(), [](T* ptr) { return ptr->hnext(); }))
template <typename T, typename F>
struct MobjListView
{
	static_assert(std::is_convertible_v<T, mobj_t>);

	struct Iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = T*;
		using pointer = value_type;
		using reference = value_type;

		Iterator(pointer ptr, F adv) : ptr_(deref(ptr)), adv_(adv) {}
		Iterator& operator=(const Iterator& b)
		{
			// adv_ may be a lambda. However, lambdas are not
			// copy assignable. Therefore, perform copy
			// construction instead!
			this->~Iterator();
			return *new(this) Iterator {b};
		}

		bool operator==(const Iterator& b) const { return ptr_ == b.ptr_; };
		bool operator!=(const Iterator& b) const { return ptr_ != b.ptr_; };

		reference operator*() const { return ptr_; }
		pointer operator->() { return ptr_; }

		Iterator& operator++()
		{
			ptr_ = deref(adv_(ptr_));
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator prev = *this;
			++(*this);
			return prev;
		}

	private:
		pointer ptr_;
		F adv_;

		static T* deref(T* ptr) { return !P_MobjWasRemoved(ptr) ? ptr : nullptr; }
	};

	MobjListView(T* ptr, F adv) : ptr_(ptr), adv_(adv) {}

	Iterator begin() const { return {ptr_, adv_}; }
	Iterator end() const { return {nullptr, adv_}; }

private:
	T* ptr_;
	F adv_;
};

}; // namespace srb2

#endif/*mobj_list_view_hpp*/
