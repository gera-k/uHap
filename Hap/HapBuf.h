#ifndef _HAP_BUFFER_H_
#define _HAP_BUFFER_H_

namespace Hap
{
	class Tlv;

	// generic data buffer pointer
	//	consists of:
	//	 p	pointer to data buffer
	//	 s	allocation size of the buffer
	//	 l	length of valid data
	class Buf
	{
	public:
		using T = uint8_t;
		using S = uint16_t;
		using L = uint16_t;

		// empty memory-less buffer
		Buf()
		: _s(0), _l(0), _p(nullptr)
		{}

		// copy pointer (doesn't copy content)
		Buf(const Buf& b)
		: _s(b._s), _l(b._l), _p(b._p)
		{}

		// attach to memory area, set initial length
		Buf(const void* p, S s, L l)
		: _s(s), _l(l), _p((T*)p)
		{}

		// attach to memory area, set initial length equal to size
		Buf(const void* p, S s)
		: _s(s), _l(s), _p((T*)p)
		{}

		~Buf()
		{}

        // attach to external buffer
        void attach(const Buf& b)
        {
			_s = b._s;
			_l = b._l; 
			_p = b._p;
		}

		// pointer to memory offset
		const T* p(S off = 0) const
		{
            if (off >= _s)
                return nullptr;
			return _p + off;
		}

		T* p(S off = 0)
		{
            if (off >= _s)
                return nullptr;
			return _p + off;
		}

		S s() const
		{
			return _s;
		}

		L l() const
		{
			return _l;
		}

		// update valid length, truncate if overflow
		//	returns resulting length
		L l(L l)
		{
			if (l > _s)
				l = _s;
			_l = l;
			return _l;
		}

		// reset length to 0
		void init()
		{
			_l = 0;
		}

		// append (copy) data to buffer memory
		bool append(const void* p, L l);

		// append another Buffer
		bool append(const Buf& buf);

		// append TLV prefix and data
		bool append(const Tlv& tlv, bool copy = true);

	protected:
		S _s;				// size of buffer memory
		L _l;				// length of valid data
		T* _p = nullptr;	// points to memory
	};

	// Buffer with memory
	template<Buf::S size>
	class Mem : public Buf
	{
	public:
		Mem()
		: Buf(_buf, size)
		{}

		void zero()
		{
			memset(_buf, 0, size);
		}

		void copy(const Buf& buf)
		{
			L l = buf.l();
			if (l > _s)
				l = _s;
			memcpy(_p, buf.p(), l);
			_l = l;
		}
	private:
		Buf::T _buf[size];
	};

	// platform-dependent memory allocation
	Buf* BufAlloc();
	void BufFree(Buf* buf);

	// temp buffer
	class Tmp : public Buf
	{
	public:
		Tmp();
		~Tmp();
	private:
		Buf* _mem = nullptr;
	};
}

#endif /*_HAP_BUFFER_H_*/