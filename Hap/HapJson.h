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

#ifndef _HAP_JSON_H_
#define _HAP_JSON_H_

#include "jsmn.h"
#include <type_traits>
#include <limits>

namespace Hap
{
	namespace Json
	{
		struct member
		{
			const char* key;	// key
			uint8_t type;		// expected type mask, may include JSMN_UNDEFINED if key is optional

			// parse results:
			int i = 0;			// i>0 - key is present, i = index of value; i<=0 - key not present
		};

		class Obj
		{
		protected:
			const char* _js = nullptr;
			uint16_t _len = 0;
			jsmntok_t* _tk;
			int _cnt = 0;

		public:
			Obj(jsmntok_t* tk)
			: _tk(tk)
			{
			}

			// token pointer
			const jsmntok_t* tk(int i) const
			{
				if (i < 0 || i >= _cnt)
					return nullptr;
				return &_tk[i];
			}

			// token type
			int8_t type(int i)
			{
				auto t = tk(i);
				if (t == nullptr)
					return int8_t(JSMN_UNDEFINED);

				return t->type;
			}

			// token start
			const char* start(int i) const
			{
				auto t = tk(i);
				if (t == nullptr)
					return nullptr;

				return _js + t->start;
			}

			// token length
			int length(int i) const
			{
				auto t = tk(i);
				if (t == nullptr)
					return 0;

				return t->end - t->start;
			}

			// object/array size
			int size(int i) const
			{
				auto t = tk(i);
				if (t == nullptr)
					return 0;

				return t->size;
			}

			// is this is the 'null' token?
			bool is_null(int i) const
			{
				auto t = tk(i);
				if (t == nullptr)
					return false;

				if (cmp(t, "null", JSMN_PRIMITIVE))
					return true;

				return false;
			}

			// is this is the 'bool' token?
			bool is_bool(int i, bool& value) const
			{
				auto t = tk(i);
				if (t == nullptr)
					return false;

				if (cmp(t, "true", JSMN_PRIMITIVE))
				{
					value = true;
					return true;
				}

				if (cmp(t, "1", JSMN_PRIMITIVE))
				{
					value = true;
					return true;
				}

				if (cmp(t, "false", JSMN_PRIMITIVE))
				{
					value = false;
					return true;
				}

				if (cmp(t, "0", JSMN_PRIMITIVE))
				{
					value = false;
					return true;
				}

				return false;
			}

			template<typename T> bool is_number(int i, T& value) const
			{
				Log::Msg("unknown number\n");

				return false;
			}
			template<typename T> bool is_number(int i, std::enable_if_t<std::is_floating_point<T>::value, T&> value) const
			{
				char s[24];
				auto t = tk(i);
				if (t == nullptr)
					return false;

				double v = atof(copy(i, s, sizeofarr(s)));
				value = v;
				return true;
			}
			template<typename T> bool is_number(int i, std::enable_if_t<std::is_integral<T>::value && std::numeric_limits<T>::is_signed, T&> value) const
			{
				char s[24];
				auto t = tk(i);
				if (t == nullptr)
					return false;

				int64_t v = atoll(copy(i, s, sizeofarr(s)));

				Log::Dbg("signed int '%s' -> %lld\n", s, v);

				if (v >= std::numeric_limits<T>::min() && v <= std::numeric_limits<T>::max())
				{
					value = static_cast<T>(v);
					return true;
				}
				return false;
			}
			template<typename T> bool is_number(int i, std::enable_if_t<std::is_integral<T>::value && !std::numeric_limits<T>::is_signed, T&> value) const
			{
				char s[24];
				auto t = tk(i);
				if (t == nullptr)
					return false;

				uint64_t v = atoll(copy(i, s, sizeofarr(s)));

				Log::Dbg("unsigned int '%s' -> %llu\n", s, v);

				if (v >= std::numeric_limits<T>::min() && v <= std::numeric_limits<T>::max())
				{
					value = static_cast<T>(v);
					return true;
				}
				return false;
			}

			// copy token i into buffer, zero terminate the buffer
			char* copy(int i, char* buf, uint32_t size) const
			{
				uint32_t l = 0;
				const jsmntok_t* t = tk(i);
				if (t != nullptr)
				{
					l = t->end - t->start;
					if (l > size - 1)
						l = size - 1;
					strncpy(buf, _js + t->start, l);
				}
				buf[l] = 0;
				return buf;
			}

		protected:
			bool cmp(const jsmntok_t *tk, const char *s, uint8_t type = JSMN_STRING) const
			{
				int l = tk->end - tk->start;
				return tk->type & type
					&& (int)strlen(s) == l
					&& strncmp(_js + tk->start, s, l) == 0;
			}

			int dump(const jsmntok_t *t, uint32_t count, int ind) const
			{
				char indent[64];
				int i, j;
				if (count == 0)
				{
					return 0;
				}
				i = 0;
				j = ind;
				while (j-- > 0)
				{
					indent[i++] = ' ';
					indent[i++] = ' ';
				}
				indent[i++] = 0;

				if (t->type == JSMN_PRIMITIVE)
				{
					Log::Dbg("%sP(%d-%d-%d)%.*s\n", indent, t->start, t->end, t->parent, t->end - t->start, _js + t->start);
					return 1;
				}
				else if (t->type == JSMN_STRING)
				{
					Log::Dbg("%sS(%d-%d-%d)'%.*s'\n", indent, t->start, t->end, t->parent, t->end - t->start, _js + t->start);
					return 1;
				}
				else if (t->type == JSMN_OBJECT)
				{
					Log::Dbg("%sO(%d-%d-%d %d)%.*s {\n", indent, t->start, t->end, t->parent, t->size, t->end - t->start, _js + t->start);
					j = 0;

					for (i = 0; i < t->size; i++)
					{
						j += dump(t + 1 + j, count - j, ind + 1);
						Log::Dbg("%s  :\n", indent);
						j += dump(t + 1 + j, count - j, ind + 1);
					}

					Log::Dbg("%s}\n", indent);
					return j + 1;
				}
				else if (t->type == JSMN_ARRAY)
				{
					j = 0;
					Log::Dbg("%sA(%d-%d-%d %d)%.*s [\n", indent, t->start, t->end, t->parent, t->size, t->end - t->start, _js + t->start);

					for (i = 0; i < t->size; i++)
					{
						j += dump(t + 1 + j, count - j, ind + 1);
					}
					Log::Dbg("%s]\n", indent);
					return j + 1;
				}
				return 0;
			}
		};

		template<int TokenCount = 64>
		class Parser : public Obj
		{
		private:
			jsmn_parser _ps;
			jsmntok_t _tk[TokenCount];

		public:
			Parser() : Obj(_tk)
			{
				jsmn_init(&_ps);
			}

			// parse JSON-formatted string
			bool parse(const char* js, uint16_t len)
			{
				_js = js;
				_len = len;

				_cnt = jsmn_parse(&_ps, _js, _len, _tk, sizeofarr(_tk));

				return _cnt > 0;
			}

			// parse object
			// returns -1 when parsed ok, or index of first invalid or missing parameter
			int parse(int obj, member* om, int om_cnt) const
			{
				int i;

				for (i = 0; i < om_cnt; i++)
				{
					member* m = &om[i];

					// find token that matches the requested key
					m->i = find(obj, m->key);

					if (m->i > 0)
					{
						m->i++;

						// verify that member type is valid
						if (!(m->type & _tk[m->i].type))
						{
							m->i = -1;
							return i;  // invalid type of key i
						}
					}
				}

				for (i = 0; i < om_cnt; i++)
				{
					member* m = &om[i];

					if (!(m->type & JSMN_UNDEFINED) && (m->i < 0))
						return i;	// missing mandatory key i 
				}

				return -1;
			}

			// find object <key> in json object <obj>
			//	returns token index or -1
			int find(int obj, const char* key) const
			{
				if (_tk[obj].type != JSMN_OBJECT)
					return -1;

				int i = obj + 1;
				int cnt = _tk[obj].size;		// number of object members
				while (cnt)
				{
					// find next member which parent is obj
					if (_tk[i].parent == obj)
					{
						if (cmp(&_tk[i], key))
							return i;
						cnt--;
					}
					i++;  //assume t[obj].size is correct so we never go beyond the token array
				}
				return -1;
			}

			// find element <ind> of array <arr>
			//	returns token index or -1
			int find(int arr, int ind) const
			{
				if (_tk[arr].type != JSMN_ARRAY)
					return -1;

				int i = arr + 1;
				int cnt = _tk[arr].size;		// number of array elements
				while (cnt)
				{
					// find next element which parent is arr
					if (_tk[i].parent == arr)
					{
						// if requested element is reached, return its index
						if (ind-- == 0)
						{
							return i;
						}
						cnt--;
					}
					i++;
				}
				return -1;
			}

			void dump() const
			{
				Obj::dump(_tk, _cnt, 0);
			}
		};

		using ParserDef = Parser<>;
	}
}

#endif
