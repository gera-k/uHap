/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
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
		Buf()
		{}

		Buf(T* p, L l)
			: _p(p), _l(l)
		{}

		T* p() const
		{
			return _p;
		}

		L l() const
		{
			return _l;
		}

		void set(T* p, L l)
		{
			_p = p;
			_l = l;
		}

	private:
		T* _p = nullptr;
		L _l = 0;
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

}

#endif /*_HAP_BUFFER_H_*/