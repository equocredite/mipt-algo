#include <iostream>
#include <vector>
#include <string>

class BigInteger
{
private:
	static const int BASE_ = 10;
	std::vector<int> num_; //least to most significant digit
	int sign_;

public:
	BigInteger() : num_({0}), sign_(1) {}
	BigInteger(int);
	BigInteger(const std::string &);
	
	BigInteger & normalize();
	
	int sign() const;
	BigInteger & invert_sign();
	
	explicit operator bool() const;
	explicit operator double() const;
	
	bool abs_equal(const BigInteger &other) const;
	bool abs_less(const BigInteger &other) const;
	bool abs_greater(const BigInteger &other) const;
	
	BigInteger & abs_add(const BigInteger &other);
	BigInteger & abs_substract(const BigInteger &other);
	BigInteger & abs_multiply(const BigInteger &other);
	
	BigInteger power_of_ten(int pow) const;
	BigInteger & abs_divide(const BigInteger &other);
	
	friend void swap(BigInteger &, BigInteger &);
	
	friend std::istream & operator >> (std::istream & in, BigInteger & x);
	
	std::string toString() const;
};

BigInteger & operator *= (BigInteger &, const BigInteger &);
BigInteger operator + (const BigInteger &, const BigInteger &);
BigInteger operator * (const BigInteger &, const BigInteger &);

int BigInteger::sign() const
{
	return sign_;
}

BigInteger & BigInteger::invert_sign()
{
	sign_ *= (-1);
	return normalize();
}

BigInteger & BigInteger::normalize()
{
	while (num_.size() > 1 && num_.back() == 0)
		num_.pop_back();
	if (num_.back() == 0)
		sign_ = 1;
	return *this;
}

BigInteger::BigInteger(int x) : sign_(1)
{
	if (x < 0)
	{
		sign_ = -1;
		x *= -1;
	}
	if (x == 0) num_ = {0};
	while (x != 0)
	{
		num_.push_back(x % 10);
		x /= 10;
	}
}

BigInteger::BigInteger(const std::string & s) : sign_(1)
{
	if (s.front() == '-')
		sign_ = -1;
	for (size_t i = s.size(); i > 0 && s[i - 1] != '-'; --i)
		num_.push_back(s[i - 1] - '0');
	normalize();
}

BigInteger::operator bool() const
{
	return num_.back() != 0;
}

BigInteger::operator double() const
{
	double result = 0;
	double coef = 1.0;
	for (size_t i = 0; i < num_.size(); ++i)
	{
		result += coef * num_[i];
		coef *= 10.0;
	}
	return result;
}

bool BigInteger::abs_equal(const BigInteger &other) const
{
	return num_ == other.num_;
}

bool BigInteger::abs_less(const BigInteger &other) const
{
	if (num_.size() != other.num_.size())
		return num_.size() < other.num_.size();
	for (size_t i = num_.size(); i > 0; --i)
		if (num_[i - 1] != other.num_[i - 1])
			return num_[i - 1] < other.num_[i - 1];
	return false;
}

bool BigInteger::abs_greater(const BigInteger &other) const
{
	return !(abs_less(other) || abs_equal(other));
}

BigInteger & BigInteger::abs_add(const BigInteger &other)
{
	int carry = 0;
	for (size_t i = 0; i < std::max(num_.size(), other.num_.size()) || carry; ++i)
	{
		if (i == num_.size())
			num_.push_back(0);
		num_[i] += (i < other.num_.size() ? other.num_[i] : 0) + carry;
		if (num_[i] >= BASE_)
		{
			carry = 1;
			num_[i] -= BASE_;
		} else
			carry = 0;
	}
	return normalize();
}

BigInteger & BigInteger::abs_substract(const BigInteger &other)
{
	int carry = 0;
	for (size_t i = 0; i < std::max(num_.size(), other.num_.size()); ++i)
	{
		num_[i] -= (i < other.num_.size() ? other.num_[i] : 0) + carry;
		if (num_[i] < 0)
		{
			carry = 1;
			num_[i] += BASE_;
		} else
			carry = 0;
	}
	return normalize();
}

BigInteger & BigInteger::abs_multiply(const BigInteger &other)
{
	BigInteger result;
	result.num_.resize(num_.size() + other.num_.size() + 7);
	for (size_t i = 0; i < num_.size(); ++i)
	{
		int carry = 0;
		for (size_t j = 0; j < other.num_.size() || carry > 0; ++j)
		{
			result.num_[i + j] += num_[i] * (j < other.num_.size() ? other.num_[j] : 0) + carry;
			carry = result.num_[i + j] / BASE_;
			result.num_[i + j] %= BASE_;
		}
	}
	num_ = result.num_;
	return normalize();
}

BigInteger BigInteger::power_of_ten(int pow) const
{
	BigInteger result(1);
	static BigInteger ten(10);
	while (pow--)
		result.abs_multiply(ten);
	return result;
}

BigInteger & BigInteger::abs_divide(const BigInteger &other)
{
	if (abs_less(other))
	{
		num_ = {0};
		return *this;
	}
	if (abs_equal(other))
	{
		num_ = {1};
		return *this;
	}
	BigInteger dividend = *this, result;
	result.num_.resize(dividend.num_.size());
	for (size_t i = result.num_.size(); i > 0; --i)
	{
		int digit;
		BigInteger pow = power_of_ten(i - 1);
		for (digit = 9; dividend.abs_less((pow * digit).abs_multiply(other)); --digit) {}
		result.num_[i - 1] = digit;
		dividend.abs_substract((pow * digit).abs_multiply(other));
	}
	num_ = result.num_;
	return normalize();
}

bool operator == (const BigInteger & a, const BigInteger & b)
{
	return a.sign() == b.sign() && a.abs_equal(b);
}

bool operator != (const BigInteger & a, const BigInteger & b)
{
	return !(a == b);
}

bool operator < (const BigInteger & a, const BigInteger & b)
{
	if (a.sign() != b.sign())
		return a.sign() < b.sign();
	if (a.sign() == 1)
		return a.abs_less(b);
	return a.abs_greater(b);
}

bool operator <= (const BigInteger & a, const BigInteger & b)
{
	return a == b || a < b;
}

bool operator > (const BigInteger & a, const BigInteger & b)
{
	return !(a <= b);
}

bool operator >= (const BigInteger & a, const BigInteger & b)
{
	return !(a < b);
}

BigInteger & operator += (BigInteger & a, const BigInteger & b)
{
	if (a.sign() == b.sign())
		a.abs_add(b);
	else if (!a.abs_less(b))
		a.abs_substract(b);
	else a = b + a;
	return a.normalize();
}

BigInteger & operator -= (BigInteger & a, const BigInteger & b)
{
	a *= (-1);
	a += b;
	a *= (-1);
	return a.normalize();
}

BigInteger & operator *= (BigInteger & a, const BigInteger & b)
{
	a.abs_multiply(b);
	if (b.sign() == -1)
		a.invert_sign();
	return a.normalize();
}

BigInteger & operator /= (BigInteger & a, const BigInteger & b)
{
	a.abs_divide(b);
	if (b.sign() == -1)
		a.invert_sign();
	return a.normalize();
}

BigInteger & operator %= (BigInteger & a, const BigInteger & b)
{
	BigInteger aux = a;
	aux /= b;
	aux *= b;
	a -= aux;
	return a.normalize();
}

BigInteger operator + (const BigInteger & a, const BigInteger & b)
{
	BigInteger result = a;
	return result += b;
}

BigInteger operator - (const BigInteger & a, const BigInteger & b)
{
	BigInteger result = a;
	return result -= b;
}

BigInteger operator * (const BigInteger & a, const BigInteger & b)
{
	BigInteger result = a;
	return result *= b;
}

BigInteger operator / (const BigInteger & a, const BigInteger & b)
{
	BigInteger result = a;
	return result /= b;
}

BigInteger operator % (const BigInteger & a, const BigInteger & b)
{
	BigInteger result = a;
	return result %= b;
}

BigInteger operator -(const BigInteger & x)
{
	BigInteger result = x;
	result.invert_sign();
	return result.normalize();
}

BigInteger & operator ++(BigInteger & x)
{
	return x += BigInteger(1);
}

BigInteger operator ++(BigInteger & x, int)
{
	BigInteger result = x;
	x += BigInteger(1);
	return result;
}

BigInteger & operator --(BigInteger & x)
{
	return x -= BigInteger(1);
}

BigInteger operator --(BigInteger & x, int)
{
	BigInteger result = x;
	x -= BigInteger(1);
	return result;
}

std::string BigInteger::toString() const
{
	std::string result;
	if (sign() == -1)
		result = "-";
	for (size_t i = num_.size(); i > 0; --i)
		result.push_back(num_[i - 1] + '0');
	return result;
}

std::ostream & operator << (std::ostream & out, const BigInteger & x)
{
	out << x.toString();
	return out;
}

std::istream & operator >> (std::istream & in, BigInteger & x)
{
	std::string str;
	in >> str;
	x = BigInteger(str);
	return in;
}

void swap(BigInteger & a, BigInteger & b)
{
	int temp_s = a.sign_;
	a.sign_ = b.sign_;
	b.sign_ = temp_s;
	
	std::vector<int> temp_n = a.num_;
	a.num_ = b.num_;
	b.num_ = temp_n;
}

BigInteger greatest_common_divisor(BigInteger a, BigInteger b)
{
	if (a < 0)
		a.invert_sign();
	if (b < 0)
		b.invert_sign();
	while (b != 0)
	{
		a %= b;
		swap(a, b);
	}
	return a.normalize();
}


class Rational
{
private:
	BigInteger p_, q_;
public:
	Rational() : p_(0), q_(1) {}
	Rational(int x) : p_(x), q_(1) {}
	Rational(const BigInteger & x);
	
	explicit operator double();
	
	friend bool operator == (const Rational &, const Rational &);
	friend bool operator < (const Rational &, const Rational &);
	
	friend Rational & operator += (Rational &, const Rational &);
	friend Rational & operator *= (Rational &, const Rational &);
	friend Rational & operator /= (Rational &, const Rational &);
	friend Rational operator -(const Rational &);
	
	Rational & normalize();
	
	std::string toString();
	std::string asDecimal(size_t) const;
};

Rational::Rational(const BigInteger &x) : p_(x), q_(1) {}

Rational & Rational::normalize()
{
	BigInteger gcd = greatest_common_divisor(p_, q_);
	p_ /= gcd;
	q_ /= gcd;
	if (q_.sign() == -1)
	{
		p_.invert_sign();
		q_.invert_sign();
	}
	p_.normalize();
	q_.normalize();
	return *this;
}

bool operator == (const Rational & a, const Rational & b)
{
	return a.p_ == b.p_ && a.q_ == b.q_;
}

bool operator != (const Rational & a, const Rational & b)
{
	return !(a == b);
}

bool operator < (const Rational & a, const Rational & b)
{
	return a.p_ * b.q_ < b.p_ *a.q_;
}

bool operator <= (const Rational & a, const Rational & b)
{
	return a == b || a < b;
}

bool operator > (const Rational & a, const Rational & b)
{
	return !(a <= b);
}

bool operator >= (const Rational & a, const Rational & b)
{
	return !(a < b);
}

Rational & operator += (Rational & a, const Rational & b)
{
	if (a.q_ == b.q_)
		a.p_ += b.p_;
	else
	{
		a.p_ *= b.q_;
		a.p_ += b.p_ * a.q_;
		a.q_ *= b.q_;
	}
	return a.normalize();
}

Rational & operator -= (Rational & a, const Rational & b)
{
	if (a == b)
	{
		a = 0;
		return a;
	}
	a *= (-1);
	a += b;
	a *= (-1);
	return a.normalize();
}

Rational & operator *= (Rational & a, const Rational & b)
{
	a.p_ *= b.p_;
	a.q_ *= b.q_;
	return a.normalize();
}

Rational & operator /= (Rational & a, const Rational & b)
{
	a.p_ *= b.q_;
	a.q_ *= b.p_;
	return a.normalize();
}

Rational operator + (const Rational & a, const Rational & b)
{
	Rational result = a;
	return result += b;
}

Rational operator - (const Rational & a, const Rational & b)
{
	Rational result = a;
	return result -= b;
}

Rational operator * (const Rational & a, const Rational & b)
{
	Rational result = a;
	return result *= b;
}

Rational operator / (const Rational & a, const Rational & b)
{
	Rational result = a;
	return result /= b;
}

Rational operator -(const Rational & x)
{
	Rational result = x;
	result.p_.invert_sign();
	return result.normalize();
}

std::string Rational::toString()
{
	std::string result = p_.toString();
	if (q_ != BigInteger(1))
		result += "/" + q_.toString();
	return result;
}

std::string Rational::asDecimal(size_t precision = 0) const
{
	std::string result;
	if (p_.sign() == -1)
		result += "-";
	BigInteger pow = p_.power_of_ten(precision);
	BigInteger before_dot = p_;
	before_dot *= pow;
	before_dot /= q_;
	std::string after_dot = ((before_dot * (before_dot < 0 ? -1 : 1)) % pow).toString();
	before_dot /= pow;
	result += before_dot.toString();
	if (precision > 0)
	{
		result += ".";
		for (size_t i = 0; i + after_dot.size() < precision; ++i)
			result += '0';
		result += after_dot;
	}
	return result;
}

Rational::operator double()
{
	return double(p_) / double(q_);
}
