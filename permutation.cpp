#include <iostream>
#include <cstring>

class Permutation
{
private:
	size_t n_;
	int * p_;
	
public:
	Permutation();
	Permutation(unsigned int);
	Permutation(unsigned int, int *);
	Permutation(const Permutation &);
	~Permutation();
	
	Permutation & operator =(const Permutation &);
	const int & operator [](size_t);
	
	bool operator == (const Permutation &);
	bool operator != (const Permutation &);
	bool operator < (const Permutation &);
	bool operator <= (const Permutation &);
	bool operator > (const Permutation &);
	bool operator >= (const Permutation &);
	
	Permutation & operator *=(const Permutation &);
	Permutation operator *(const Permutation &);
	Permutation & operator ++();
	Permutation operator ++(int);
	Permutation & operator --();
	Permutation operator --(int);
	
	Permutation next();
	Permutation previous();
	Permutation inverse();
	
	void operator ()(int *);
};

Permutation::Permutation() : n_(0), p_(nullptr) {}

Permutation::Permutation(unsigned int n) : n_(n)
{
	p_ = new int[n_];
	for (size_t i = 0; i < n_; ++i)
		p_[i] = i;
}

Permutation::Permutation(unsigned int n, int * val) : n_(n)
{
	p_ = new int[n_];
	memcpy(p_, val, n_ * sizeof(int));
}

Permutation::Permutation(const Permutation & other)
{
	n_ = other.n_;
	p_ = new int[n_];
	memcpy(p_, other.p_, n_ * sizeof(int));
}

Permutation::~Permutation()
{
	delete[] p_;
}

const int & Permutation::operator [](size_t i)
{
	return p_[i];
}

bool Permutation::operator == (const Permutation & other)
{
	if (n_ != other.n_)
		return false;
	for (size_t i = 0; i < n_; ++i)
		if (p_[i] != other.p_[i])
			return false;
	return true;
}

bool Permutation::operator != (const Permutation & other)
{
	return !(*this == other);
}

bool Permutation::operator < (const Permutation & other)
{
	for (size_t i = 0; i < n_ || i < other.n_; ++i)
	{
		if (i >= n_ || i >= other.n_)
			return i >= n_;
		if (p_[i] != other.p_[i])
			return p_[i] < other.p_[i];
	}
	return false;
}

bool Permutation::operator <= (const Permutation & other)
{
	return (*this == other) || (*this < other);
}

bool Permutation::operator > (const Permutation & other)
{
	return !(*this <= other);
}

bool Permutation::operator >= (const Permutation & other)
{
	return !(*this < other);
}

Permutation & Permutation::operator = (const Permutation & other)
{
	if (*this == other)
		return *this;
	delete[] p_;
	n_ = other.n_;
	p_ = new int[n_];
	memcpy(p_, other.p_, n_ * sizeof(int));
	return *this;
}

Permutation & Permutation::operator *= (const Permutation & other)
{
	int * buf = new int[n_];
	for (size_t i = 0; i < n_; ++i)
		buf[i] = p_[other.p_[i]];
	memcpy(p_, buf, n_ * sizeof(int));
	delete[] buf;
	return *this;
}

Permutation Permutation::operator * (const Permutation & other)
{
	Permutation result = *this;
	return result *= other;
}

void swap(int & a, int & b)
{
	int tmp = a;
	a = b;
	b = tmp;
}

Permutation & Permutation::operator ++()
{
	size_t k;
	for (k = n_ - 1; k > 0 && p_[k - 1] > p_[k]; --k) {}
	if (k == 0)
	{
		for (size_t i = 0; i < n_; ++i)
			p_[i] = i;
		return *this;
	}
	--k;
	size_t min_greater = k + 1;
	for (size_t i = k + 1; i < n_; ++i)
		if (p_[i] > p_[k] && p_[i] < p_[min_greater])
			min_greater = i;
	swap(p_[k], p_[min_greater]);
	for (size_t left = k + 1, right = n_ - 1; left < right; ++left, --right)
		swap(p_[left], p_[right]);
	return *this;
}

Permutation Permutation::operator ++(int)
{
	Permutation result = *this;
	++(*this);
	return result;
}

Permutation & Permutation::operator --()
{
	size_t k;
	for (k = n_ - 1; k > 0 && p_[k - 1] < p_[k]; --k) {}
	if (k == 0)
	{
		for (size_t i = 0; i < n_; ++i)
			p_[i] = n_ - i - 1;
		return *this;
	}
	--k;
	size_t max_less = k + 1;
	for (size_t i = k + 1; i < n_; ++i)
		if (p_[i] < p_[k] && p_[i] > p_[max_less])
			max_less = i;
	swap(p_[k], p_[max_less]);
	for (size_t left = k + 1, right = n_ - 1; left < right; ++left, --right)
		swap(p_[left], p_[right]);
	return *this;
}

Permutation Permutation::operator --(int)
{
	Permutation result = *this;
	--(*this);
	return result;
}

Permutation Permutation::next()
{
	Permutation result = *this;
	++result;
	return result;
}

Permutation Permutation::previous()
{
	Permutation result = *this;
	--result;
	return result;
}

Permutation Permutation::inverse()
{
	Permutation result(n_);
	for (size_t i = 0; i < n_; ++i)
		result.p_[p_[i]] = i;
	return result;
}

void Permutation::operator ()(int *arr)
{
	int * buf = new int[n_];
	for (size_t i = 0; i < n_; ++i)
		buf[p_[i]] = arr[i];
	memcpy(arr, buf, n_ * sizeof(int));
	delete[] buf;
}
