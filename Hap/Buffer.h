/*
MIT License

Copyright (c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _HAP_BUFFER_H_
#define _HAP_BUFFER_H_

namespace Hap
{
	// generic buffer (pointer/length pair)
	template<typename T, typename L = uint32_t>
	class Buf
	{
	public:
		Buf() : _p(nullptr), _l(0) {}
		Buf(T* p, L l) : _p(p), _l(l) {}
		auto p() const { return _p; }
		auto l() const { return _l; }
		auto& p() { return _p; }
		auto& l() { return _l; }

		T& operator[](int i) { return p[i]; }
//	private:
		T* _p;
		L _l;
	};


	// buffer with internally allocated memory
	template<typename T, uint32_t s>
	class BufStatic : public Buf<T>
	{
	private:
		T _b[s];

	public:
		BufStatic() : Buf<T>(_b, s) {}
	};

	// make buffer on passed-in memory
	template<typename T, typename S = uint32_t>
	static inline Buf<T,S> makeBuf(T* p, S s) 
	{ 
		return Buf<T,S>(p, s);
	}
}

#endif /*_HAP_BUFFER_H_*/