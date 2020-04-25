/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Crypto.h"
#include "MD.h"

static constexpr unsigned D_BITS = 32;				// bits per digit
static constexpr unsigned D_BYTES = D_BITS / 8;		// bytes per digit
static constexpr MD::D D_HIBIT = 1UL << (D_BITS - 1);	// highest bit of a digit

uint32_t MD::cntMul;
uint32_t MD::cntMulS;
uint32_t MD::cntMulK;

static inline MD::D lowDigit(MD::DD dd)
{
	return dd & 0xFFFFFFFF;
}

static inline MD::D highDigit(MD::DD dd)
{
	return (MD::D)(dd >> 32);
}

// create MD on provided buffer
MD::MD(D* d, uint32_t digits)
	: _d(d), _s(digits)
{
}

// create MD on provided buffer and init from given octets
// the size of the buffer must be big enough to fit all octets
MD::MD(D* d, const uint8_t *val, uint32_t len)
	: _d(d), _s(len / D_BYTES + ((len % D_BYTES ? 1 : 0)))
{
	init(val, len);
}

void MD::zero()
{
	memset(_d, 0, _s * D_BYTES);
}

void MD::random()
{
	Crypto::rnd_data((uint8_t*)_d, _s * D_BYTES);
}

// init MD from bytes
bool MD::init(const uint8_t *val, uint32_t len)
{
	uint32_t i, j, k;

	zero();

	// read input value starting from the last (less significant) byte,
	//	convert into D, and store in _d less significant digits first
	for (i = 0, j = len; i < _s && j > 0; i++)
	{
		D t = 0;
		for (k = 0; j > 0 && k < D_BITS; k += 8)
			t |= ((D)val[--j]) << k;
		_d[i] = t;
	}

	return true;
}

// convert MD to octets
//	convert digits to octets and store them
//	in provided buffer starting from the end
//	pad result with zeros to the left
void MD::val(uint8_t* val, uint32_t len) const
{
	uint32_t i, j, k;

	if (len == 0)
		len = byteLength();

	for (i = 0, j = len; i < _s && j > 0; i++)
	{
		D t = _d[i];
		for (k = 0; j > 0 && k < D_BITS; k += 8)
			val[--j] = (t >> k) & 0xFF;
	}

	while (j > 0)
		val[--j] = 0;
}

#if defined(_INC_STDIO)
void MD::print() const
{
	uint32_t len = digitLength();

	if (len == 0)
		len = 1;

	printf("%x", _d[--len]);
	while (len--)
		printf("%08x", _d[len]);
}
#endif

// Returns number of significant digits
uint32_t MD::digitLength() const
{
	uint32_t ndigits = _s;
	while (ndigits--)
	{
		if (_d[ndigits] != 0)
			return (++ndigits);
	}
	return 0;
}

bool MD::isZero()
{
	return digitLength() == 0;
}


// Returns number of significant bytes
uint32_t MD::byteLength() const
{
	return (bitLength() + 7) / 8;
}

// Returns number of significant bits
uint32_t MD::bitLength() const
{
	uint32_t n, i, bits;
	D bit;

	n = digitLength();
	if (n == 0)
		return 0;

	for (i = 0, bit = D_HIBIT; bit > 0; bit >>= 1, i++)
	{
		if (_d[n - 1] & bit)
			break;
	}

	bits = n * D_BITS - i;

	return bits;
}

// this = x
void MD::copy(uint32_t x)
{
	zero();
	_d[0] = x;
}

// this = x
void MD::copy(const MD& x)
{
	uint32_t l = x.digitLength();
	for (uint32_t i = 0; i < _s; i++)
	{
		if (i < l)
			_d[i] = x._d[i];
		else
			_d[i] = 0;
	}
}

void MD::copy(const MD& x, uint32_t start, uint32_t length)
{
	uint32_t max = start + length;
	if (max > x._s)
		max = x._s;
	for (uint32_t i = 0; i < _s; i++)
	{
		if (start < max)
			_d[i] = x._d[start++];
		else
			_d[i] = 0;
	}
}

// this = x + y
//	14.7 Algorithm Multiple-precision addition
//	returns carry from highest digit (0 or 1)
MD::D MD::add(const MD& x, const MD& y)
{
	uint32_t m = x.digitLength();	// num of significant digits in x
	uint32_t n = y.digitLength();	// num of significant digits in y

	D c = 0;
	D* pd = _d;
	D* px = x._d;
	D* py = y._d;

	for (uint32_t i = 0; i < _s; i++)
	{
		// d_i = x_i + y_i + c
		DD t = c;

		if (i < m)
			t += *px++; // x._d[i];

		if (i < n)
			t += *py++; // y._d[i];

		*pd++ = lowDigit(t);
		c = highDigit(t);
	}

	return c;
}

// this += (x << n)
MD::D MD::add(const MD& x, uint32_t n)
{
	uint32_t m = x.digitLength();	// num of significant digits in x

	D c = 0;
	D* pd = _d + n;

	for (uint32_t i = n, j = 0; i < _s; i++, j++)
	{
		// d_i = d_i + x_i + c
		DD t = c;

		t += *pd;

		if (j < m)
			t += x._d[j];

		*pd++ = lowDigit(t);
		c = highDigit(t);
	}

	return c;
}

// this = x - y (assume x >= y)
//	returns carry from highest digit (0 or FFFFFFFF)
MD::D MD::sub(const MD& x, const MD& y)
{
	uint32_t m = x.digitLength();	// num of significant digits in x
	uint32_t n = y.digitLength();	// num of significant digits in y

	D c = 0;   // c is 0 or FFFFFFFF

	for (uint32_t i = 0; i < _s; i++)
	{
		// d_i = x_i - y_i + k
		DD t = c ? DD(-1) : 0;

		if (i < m)
			t += x._d[i];

		if (i < n)
			t -= y._d[i];

		_d[i] = lowDigit(t);
		c = highDigit(t);
	}

	return c;
}

// this -= (x << n)
MD::D MD::sub(const MD& x, uint32_t n)
{
	uint32_t m = x.digitLength();	// num of significant digits in x

	D c = 0;

	for (uint32_t i = n, j = 0; i < _s; i++, j++)
	{
		// d_i = d_i - x_i + c
		DD t = c ? DD(-1) : 0;

		t += _d[i];

		if (j < m)
			t -= x._d[j];

		_d[i] = lowDigit(t);
		c = highDigit(t);
	}

	return c;
}

// returns sign of this-y (1, 0, -1)
int MD::cmp(const MD& y) const
{
	uint32_t m = digitLength();		// num of significant digits in this
	uint32_t n = y.digitLength();	// num of significant digits in y

	if (m > n)
		return 1;
	if (m < n)
		return -1;

	while (m--)
	{
		if (_d[m] > y._d[m])
			return 1;
		if (_d[m] < y._d[m])
			return -1;
	}

	return 0;
}

// this = x * y
//	INPUT: positive integers x and y having n + 1 and m + 1 base b digits, respectively.
//	OUTPUT: the product x*y = (w_n+t+1   w_1w_0)b in radix b representation.
//	1. For i from 0 to(n + t + 1) do: w_i = 0.
//	2. For i from 0 to t do the following :
//		2.1 c = 0.
//		2.2 For j from 0 to n do the following :
//			Compute(uv)b = w_i+j + x_j*y_i + c,
//			and set w_i+j = v, c = u.
//		2.3 w_i+n+1 = u.
bool MD::mul(const MD& x, const MD& y, bool kar)
{
	cntMulS++;

	// note that these n and m are greater by 1 that n and m in the algorithm above
	uint32_t n = x.digitLength();	// num of significant digits in x
	uint32_t m = y.digitLength();	// num of significant digits in y

	// zero result
	zero();

	// check if result will fit
	if (n + m > _s)
		return false;

	if (n == 0 || m == 0)
		return true;

	D* yp = y._d;
	D* dp = _d;
	// loop through y
	while (m-- > 0)
	{
		D c = 0;	// carry
		D yi = *yp++;
		D* xp = x._d;

		// loop through x
		for (uint32_t j = 0; j < n; j++)
		{
			// compute (uv) = x_j * y_i + w_i+j + c
			DD t = (DD)(*xp++) * yi + dp[j] + c;

			dp[j] = lowDigit(t);	// w_i+j = v
			c = highDigit(t);			// c = u
		}

		dp[n] = c;
		dp++;
	}

	return true;
}


// this = v mod N  result: k bits (k - number of bits in N)
// use Barrett reduction with pre-calculated R
//	t = v - (v*R/2^(2*k)) * N
// if (t > N) t -= N
// temporary storage >= N.digitLength()*3 + 2
bool MD::Mod(
	const MD& v						// v: k * 2 bits
)
{
	if (v.cmp(N) < 0)
	{
		copy(v);
		return true;
	}

	// t2 = v*R	size 3k+1 at 1
	MD t2(t + 1, 3 * K + 1);
	if (!t2.mul(v, R))
		return false;

	// t3 = t2 >>= k*2	size k+1 at 1+2*k
	//	assuming that k*2 is divisible by D_BYTES
	//	just create another MD at position t2.pos + 2*k
	MD t3(t + 1 + 2 * K, K + 1);

	// t4 = t3*N	size 2k+1 at 0
	MD t4(t, 2 * K + 1);
	if (!t4.mul(t3, N))
		return false;

	// t3 = v-t4	size k+1 at 1+2*k
	t3.sub(v, t4);

	// this = t3 > N ? t3 - N : t3  
	if (t3.cmp(N) >= 0)
		sub(t3, N);
	else
		copy(t3);

	return true;
}

// this = x+y mod N  result: k bits (k - number of bits in N)
// temporary storage >= N.digitLength()*5 + 2
bool MD::addMod(
	const MD& x, const MD& y		// x, y: k bits
)
{
	// v = x+y  size k+1
	MD v(t + K * 3 + 2, 2 * K);
	v.add(x, y);

	return Mod(v);
}

// this = x*y mod N  result: k bits (k - number of bits in N)
// temporary storage >= N.digitLength()*5 + 2
bool MD::mulMod(
	const MD& x, const MD& y		// x, y: k bits
)
{
	// v = x*y  size 2k
	MD v(t + K * 3 + 2, 2 * K);
	if (!v.mul(x, y))
		return false;

	return Mod(v);
}

#if 1
// this = x^e mod N		result: k bits (k - number of bits in N)
//	Left-to-right binary exponentiation:
//	A = 1
//	for bits in y from leftmost to 0:
//		A = A*A
//		if e.bit
//			A = A*x
// temporary storage >= N.digitLength()*6 + 2
bool MD::expMod(
	const MD& x, const MD& e		// x, y: k bits
)
{
	MD A(t + K*5 + 2, K);		// accumulator, up to k bits
	A.copy(1);

	uint32_t n = e.digitLength();
	for (uint32_t i = n; i > 0; i--)
	{
		D d = e._d[i - 1];
		D b = D_HIBIT;

		if (i == n)
			while (!(d & b))
				b >>= 1;

		while (b > 0)
		{
			if (!mulMod(A, A))		// A = A * A mod N - k bits
				return false;

			if (d & b)
			{
				if (!A.mulMod(*this, x))	// A = A * x mod N - k bits
					return false;
			}
			else
				A.copy(*this);

			b >>= 1;
		}
		copy(A);
	}

	return true;
}
#endif
