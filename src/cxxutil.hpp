#ifndef __SRB2_CXXUTIL_HPP__
#define __SRB2_CXXUTIL_HPP__

#include <type_traits>
#include <utility>

namespace srb2 {

template <class F>
class Finally {
public:
	explicit Finally(const F& f) noexcept : f_(f) {}
	explicit Finally(F&& f) noexcept : f_(f) {}

	Finally(Finally&& from) noexcept : f_(std::move(from.f_)), call_(std::exchange(from.call_, false)) {}

	Finally(const Finally& from) = delete;
	void operator=(const Finally& from) = delete;
	void operator=(Finally&& from) = delete;

	~Finally() noexcept {
		f_();
	}

private:
	F f_;
	bool call_ = true;
};

template <class F>
Finally<std::decay_t<F>> finally(F&& f) noexcept {
	return Finally {std::forward<F>(f)};
}

}

#endif // __SRB2_CXXUTIL_HPP__
