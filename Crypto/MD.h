/*
MIT License

Copyright(c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _CRYPTO_BD_H_
#define _CRYPTO_BD_H_

// Subset of bignum math required for HAP
//	Based on algorithms from Handbook of Applied
//	Cryptography http://cacr.uwaterloo.ca/hac/

// Multi-digit integer
class MD
{
public:
	using D = uint32_t;			// base digit - 32 bit machine word
	using DD = uint64_t;		// double digit
	static constexpr unsigned D_BITS = 32;				// bits per digit
	static constexpr unsigned D_BYTES = D_BITS / 8;		// bytes per digit

	static uint32_t cntMul;
	static uint32_t cntMulS;
	static uint32_t cntMulK;

	// create MD on provided buffer
	MD(D* d, uint32_t digits);

	// create MD on provided buffer and init from given octets
	// the size of the buffer must be big enough to fit all octets
	MD(D* d, const uint8_t *val, uint32_t len);

	// fill with zeros
	void zero();

	// fill with random data
	void random();

	bool isZero();

	// init MD from bytes
	bool init(const uint8_t *val, uint32_t len);

	// convert MD to octets
	//	convert digits to octets and store them
	//	in provided buffer starting from the end
	//	pad result with zeros to the left
	void val(uint8_t* val, uint32_t len = 0) const;

#if defined(_INC_STDIO)
	void print() const;
#endif

	// Returns number of significant digits
	uint32_t digitLength() const;

	// Returns number of significant bytes
	uint32_t byteLength() const;

	// Returns number of significant bits
	uint32_t bitLength() const;

	// this = x
	void copy(uint32_t x);

	// this = x
	void copy(const MD& x);

	// this = digits of x
	void copy(const MD& x, uint32_t start, uint32_t length);

	// this = x + y
	//	returns carry from highest digit (0 or 1)
	D add(const MD& x, const MD& y);

	// this += (x << n)
	//	returns carry from highest digit (0 or 1)
	D add(const MD& x, uint32_t n = 0);

	// this = x - y (assume x >= y)
	//	returns carry from highest digit (0 or FFFFFFFF)
	D sub(const MD& x, const MD& y);

	// this -= (x << n)
	//	returns carry from highest digit (0 or FFFFFFFF)
	D sub(const MD& x, uint32_t n = 0);

	// returns sign of this-y (1, 0, -1)
	int cmp(const MD& y) const;

	// this = x * y
	bool mul(const MD& x, const MD& y, bool kar = true);

	// this = x * y
	//	this - >= 192D, x, y <= 96D 
	//bool mul_k(const MD& x, const MD& y);

	// this = v mod N  result: k bits (k - number of bits in N)
	// use Barrett reduction with pre-calculated R
	bool Mod(
		const MD& v					// v: k * 2 bits
	);

	// this = x+y mod N  result: k bits (k - number of bits in N)
	bool addMod(
		const MD& x, const MD& y	// x, y: k bits
	);

	// this = x*y mod N  result: k bits (k - number of bits in N)
	bool mulMod(
		const MD& x, const MD& y	// x, y: k bits
	);

	// this = x^e mod N		result: k bits (k - number of bits in N)
	bool expMod(
		const MD& x, const MD& e	// x, y: k bits
	);

private:
	D* _d;				// external storage for digits
	uint32_t _s;		// max size in digits
	static MD N;
	static MD R;
	static uint32_t K;	// number of digits in N
	static D* t;		// min size [N_DIGITS * 6 + 2];
};

// MDl - the storage is allocated in the object itself
template <uint32_t S>		// S - size in bytes
class MDl : public MD
{
private:
	static constexpr uint32_t Sd = S / D_BYTES + ((S % D_BYTES) ? 1 : 0);	// size in digits
	D _d[Sd];
public:
	MDl() : MD(_d, Sd) {}
	MDl(const uint8_t *val, uint32_t len = S) : MD(_d, val, len) {}
};



#endif /*_CRYPTO_BD_H_*/

