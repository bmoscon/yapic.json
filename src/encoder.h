#ifndef DEFBD7C4_2133_C63D_128D_F5D3D59111EF
#define DEFBD7C4_2133_C63D_128D_F5D3D59111EF


#include "../libs/double-conversion/double-conversion/double-conversion.h"
#include "config.h"
#include "buffer.h"
#include "globals.h"


namespace ZiboJson {
using namespace double_conversion;

#define Encoder_RETURN_TRUE return true

#define Encoder_RETURN_FALSE return false

#define Encoder_FN(name) inline bool name(PyObject* obj)

#define Encoder_AppendFast(chr) \
	( (assert(buffer.end - buffer.cursor >= 1)), (*(buffer.cursor++) = (chr)))

#define Encoder_EXTRA_CAPACITY 10

#define Encoder_EnsureCapacity(required) \
	if ((required) > buffer.end - buffer.cursor && buffer.EnsureCapacity(required) == false) { \
		Encoder_RETURN_FALSE; \
	}

#define Encoder_EnterRecursive() \
	if (++recursionDepth > maxRecursionDepth) { \
		Encoder_RETURN_FALSE; \
	}

#define Encoder_LeaveRecursive() \
	((assert(recursionDepth < maxRecursionDepth)), --recursionDepth)

#define Encoder_RecursionError(msg, ...) \
	PyErr_Format(EncodeError, ZiboJson_Err_MaxRecursion msg, __VA_ARGS__)

#define Encoder_RecursionOccured() \
	(recursionDepth > maxRecursionDepth && !PyErr_Occurred())

#define Encoder_HandleRecursion(msg, ...) \
	if (Encoder_RecursionOccured()) { \
		Encoder_RecursionError(msg, __VA_ARGS__); \
		Encoder_RETURN_FALSE;

// static const char* __hex_chars = "0123456789abcdef";
// #define HEX_CHAR(idx) (__hex_chars[(idx)])

#define HEX_CHAR(idx) ("0123456789abcdef"[(idx)])


// struct EncoderOptions {
// 	PyObject* defaultFn;
// 	PyObject* toJsonMethodName;
// 	int maxRecursionDepth;
// 	bool encodeDatetime;
// };


template<typename BUFF>
class Encoder {
	public:
		typedef typename BUFF::Char CHOUT;
		BUFF buffer;

		// const EncoderOptions* options;
		PyObject* defaultFn;
		PyObject* toJsonMethodName;
		int maxRecursionDepth;
		bool encodeDatetime;
		bool encodeHomogene;
		int recursionDepth;

		inline explicit Encoder()
			: recursionDepth(0) {
		}

		inline bool Encode(PyObject* obj) {
			if (PyUnicode_CheckExact(obj)) {
				Encoder_AppendFast('"');
				if (EXPECT_TRUE(EncodeString(obj))) {
					Encoder_AppendFast('"');
					Encoder_RETURN_TRUE;
				}
			} else if (PyDict_CheckExact(obj)) {
				return EncodeDict(obj);
			} else if (PyList_CheckExact(obj)) {
				return EncodeList(obj);
			} else if (PyTuple_CheckExact(obj)) {
				return EncodeTuple(obj);
			} else if (PyDate_CheckExact(obj)) {
				return EncodeDate(obj);
			} else if (PyTime_CheckExact(obj)) {
				return EncodeTime(obj);
			} else if (PyDateTime_CheckExact(obj)) {
				return EncodeDateTime(obj);
			} else if (PyIter_Check(obj)) {
				return EncodeIterable(obj);
			} else if (PyLong_CheckExact(obj)) {
				return EncodeLong(obj);
			} else if (PyFloat_CheckExact(obj)) {
				return EncodeFloat(obj);
			} else if (obj == Py_True) {
				Encoder_EnsureCapacity(4 + Encoder_EXTRA_CAPACITY);
				Encoder_AppendFast('t');
				Encoder_AppendFast('r');
				Encoder_AppendFast('u');
				Encoder_AppendFast('e');
				Encoder_RETURN_TRUE;
			} else if (obj == Py_False) {
				Encoder_EnsureCapacity(5 + Encoder_EXTRA_CAPACITY);
				Encoder_AppendFast('f');
				Encoder_AppendFast('a');
				Encoder_AppendFast('l');
				Encoder_AppendFast('s');
				Encoder_AppendFast('e');
				Encoder_RETURN_TRUE;
			} else if (obj == Py_None) {
				Encoder_EnsureCapacity(4 + Encoder_EXTRA_CAPACITY);
				Encoder_AppendFast('n');
				Encoder_AppendFast('u');
				Encoder_AppendFast('l');
				Encoder_AppendFast('l');
				Encoder_RETURN_TRUE;
			} else if (PyAnySet_Check(obj)) {
				return EncodeIterable(obj);
			} else if (PyObject_HasAttr(obj, toJsonMethodName)) {
				return EncodeWithJsonMethod<false>(obj);
			} else if (PyCallable_Check(defaultFn)) {
				return EncodeWithDefault<false>(obj);
			}

			PyErr_Format(EncodeError, ZiboJson_Err_NotSerializable, obj);
			Encoder_RETURN_FALSE;
		}

	private:
		Encoder_FN(__encode_dict_key) {
			if (PyUnicode_CheckExact(obj)) {
				return EncodeString(obj);
			} else if (PyLong_CheckExact(obj)) {
				return EncodeLong(obj);
			} else if (PyFloat_CheckExact(obj)) {
				return EncodeFloat(obj);
			} else if (obj == Py_True) {
				Encoder_EnsureCapacity(4 + Encoder_EXTRA_CAPACITY);
				Encoder_AppendFast('t');
				Encoder_AppendFast('r');
				Encoder_AppendFast('u');
				Encoder_AppendFast('e');
				Encoder_RETURN_TRUE;
			} else if (obj == Py_False) {
				Encoder_EnsureCapacity(5 + Encoder_EXTRA_CAPACITY);
				Encoder_AppendFast('f');
				Encoder_AppendFast('a');
				Encoder_AppendFast('l');
				Encoder_AppendFast('s');
				Encoder_AppendFast('e');
				Encoder_RETURN_TRUE;
			} else if (obj == Py_None) {
				Encoder_EnsureCapacity(4 + Encoder_EXTRA_CAPACITY);
				Encoder_AppendFast('n');
				Encoder_AppendFast('u');
				Encoder_AppendFast('l');
				Encoder_AppendFast('l');
				Encoder_RETURN_TRUE;
			} else if (PyObject_HasAttr(obj, toJsonMethodName)) {
				return EncodeWithJsonMethod<true>(obj);
			} else if (PyCallable_Check(defaultFn)) {
				return EncodeWithDefault<true>(obj);
			}

			PyErr_Format(EncodeError, ZiboJson_Err_InvalidDictKey, obj, toJsonMethodName);
			Encoder_RETURN_FALSE;
		}

		Encoder_FN(EncodeString) {
			Py_ssize_t length = PyUnicode_GET_LENGTH(obj);
			void* data = PyUnicode_1BYTE_DATA(obj);

			switch (PyUnicode_KIND(obj)) {
				case PyUnicode_1BYTE_KIND:
					Encoder_EnsureCapacity(length * 6 + Encoder_EXTRA_CAPACITY);
					__encode_string(reinterpret_cast<Py_UCS1*>(data), reinterpret_cast<Py_UCS1*>(data) + length);
				break;

				case PyUnicode_2BYTE_KIND:
					Encoder_EnsureCapacity(length * 6 + Encoder_EXTRA_CAPACITY);
					__encode_string(reinterpret_cast<Py_UCS2*>(data), reinterpret_cast<Py_UCS2*>(data) + length);
				break;

				case PyUnicode_4BYTE_KIND:
					if (sizeof(CHOUT) == 1) {
						Encoder_EnsureCapacity(length * 12 + Encoder_EXTRA_CAPACITY);
					} else {
						Encoder_EnsureCapacity(length * 6 + Encoder_EXTRA_CAPACITY);
					}
					__encode_string(reinterpret_cast<Py_UCS4*>(data), reinterpret_cast<Py_UCS4*>(data) + length);
				break;
			}

			Encoder_RETURN_TRUE;
		}

		#define StringEncoder_AppendChar(ch) \
				((assert(buffer.end - out >= 1)), (*(out++) = (ch)))

		template<typename CHIN>
		inline void __encode_string(const CHIN* input, const CHIN* end) {
			// printf("AAA %ld -> %ld\n", sizeof(CHIN), sizeof(CHOUT));
			register CHOUT* out = buffer.cursor;
			register CHOUT maxchar = buffer.maxchar;

			for (;;) {
				if (*input < 127) { // ASCII -> ASCII | UNICODE
					if (*input > 31) {
						if (*input == '\\') {
							StringEncoder_AppendChar('\\');
							StringEncoder_AppendChar('\\');
							++input;
						} else if (*input == '"') {
							StringEncoder_AppendChar('\\');
							StringEncoder_AppendChar('"');
							++input;
						} else {
							StringEncoder_AppendChar(*(input++));
						}
					} else {
						StringEncoder_AppendChar('\\');
						switch (*input) {
							case '\r': StringEncoder_AppendChar('r'); break;
							case '\n': StringEncoder_AppendChar('n'); break;
							case '\t': StringEncoder_AppendChar('t'); break;
							case '\b': StringEncoder_AppendChar('b'); break;
							case '\f': StringEncoder_AppendChar('f'); break;
							case 0:
								if (input >= end) {
									buffer.cursor = --out;
									buffer.maxchar = maxchar;
									return;
								}
							default:
								StringEncoder_AppendChar('u');
								StringEncoder_AppendChar('0');
								StringEncoder_AppendChar('0');
								StringEncoder_AppendChar(HEX_CHAR((*input & 0xF0) >> 4));
								StringEncoder_AppendChar(HEX_CHAR((*input & 0x0F)));
							break;
						}
						++input;
					}
				} else if (sizeof(CHIN) == 1 && sizeof(CHOUT) == 1) { // EXT ASCII -> ASCII
					StringEncoder_AppendChar('\\');
					StringEncoder_AppendChar('u');
					StringEncoder_AppendChar('0');
					StringEncoder_AppendChar('0');
					StringEncoder_AppendChar(HEX_CHAR((*input & 0xF0) >> 4));
					StringEncoder_AppendChar(HEX_CHAR((*(input++) & 0x0F))); // !POINTER INC
				} else if (sizeof(CHIN) == 2 && sizeof(CHOUT) == 1) { // 2byte UNICODE -> ASCII
					StringEncoder_AppendChar('\\');
					StringEncoder_AppendChar('u');
					StringEncoder_AppendChar(HEX_CHAR( (*input >> 12) ));
					StringEncoder_AppendChar(HEX_CHAR( (*input >> 8) & 0xF ));
					StringEncoder_AppendChar(HEX_CHAR( (*input >> 4) & 0xF ));
					StringEncoder_AppendChar(HEX_CHAR( *(input++) & 0xF ));
				} else if (sizeof(CHIN) == 4 && sizeof(CHOUT) == 1) { // 4 byte UNICODE -> ASCII
					StringEncoder_AppendChar('\\');
					StringEncoder_AppendChar('u');

					CHIN ch = *(input++);

					if (ch > 0xFFFF) {
						CHIN high = 0xD800 - (0x10000 >> 10) + (ch >> 10);
						StringEncoder_AppendChar('d');
						StringEncoder_AppendChar(HEX_CHAR( (high >> 8) & 0xF ));
						StringEncoder_AppendChar(HEX_CHAR( (high >> 4) & 0xF ));
						StringEncoder_AppendChar(HEX_CHAR( high & 0xF));

						StringEncoder_AppendChar('\\');
						StringEncoder_AppendChar('u');
						ch = 0xDC00 + (ch & 0x3FF);
					}

					StringEncoder_AppendChar(HEX_CHAR( (ch >> 12) ));
					StringEncoder_AppendChar(HEX_CHAR( (ch >> 8) & 0xF ));
					StringEncoder_AppendChar(HEX_CHAR( (ch >> 4) & 0xF ));
					StringEncoder_AppendChar(HEX_CHAR( ch & 0xF ));
				} else if (sizeof(CHOUT) > 1) { // UNICODE -> UNICODE
					maxchar |= *input;
					StringEncoder_AppendChar(*(input++));
				} else {
					assert(0);
				}
			}

			assert(0);
		}

		Encoder_FN(EncodeLong) {
			Encoder_EnsureCapacity(LONG_MAX_LENGTH_IN_CHR + Encoder_EXTRA_CAPACITY);
			register long value = PyLong_AS_LONG(obj);
			register unsigned long abs_value = value;

			if (value < 0) {
				abs_value = -value;
				Encoder_AppendFast('-');
			}

			register CHOUT *end_position = buffer.cursor + LONG_MAX_LENGTH_IN_CHR;
			register CHOUT *saved_end_position = end_position;

			do {
				*(--end_position) = (48 + (abs_value % 10));
			} while ((abs_value /= 10) > 0);

			abs_value = saved_end_position - end_position;

			memmove(buffer.cursor, end_position, sizeof(CHOUT) * abs_value);
			buffer.cursor += abs_value;

			Encoder_RETURN_TRUE;
		}

		Encoder_FN(EncodeFloat) {
			Encoder_EnsureCapacity(DOUBLE_MAX_LENGTH_IN_CHR + Encoder_EXTRA_CAPACITY);

			if (sizeof(CHOUT) == 1) {
				StringBuilder builder((char*) buffer.cursor, DOUBLE_MAX_LENGTH_IN_CHR);

				DoubleToStringConverter::EcmaScriptConverter().ToShortest(
					PyFloat_AS_DOUBLE(obj),
					&builder
				);

				buffer.cursor += builder.position();
			} else {
				char tmp[DOUBLE_MAX_LENGTH_IN_CHR];
				StringBuilder builder(tmp, DOUBLE_MAX_LENGTH_IN_CHR);

				DoubleToStringConverter::EcmaScriptConverter().ToShortest(
					PyFloat_AS_DOUBLE(obj),
					&builder
				);

				register int size = builder.position();
				if (size) {
					buffer.cursor += size;
					CHOUT* cursor = buffer.cursor - 1;
					do {
						*(cursor--) = tmp[--size];
					} while(size);
				}
			}
			Encoder_RETURN_TRUE;
		}

		#define EncodeDT_AppendInt2(value) \
			Encoder_AppendFast('0' + (value / 10)); \
			Encoder_AppendFast('0' + (value % 10));

		#define EncodeDT_AppendInt3(value) \
			Encoder_AppendFast('0' + (value / 100)); \
			Encoder_AppendFast('0' + ((value / 10) % 10)); \
			Encoder_AppendFast('0' + (value % 10));

		#define EncodeDT_AppendInt4(value) \
			Encoder_AppendFast('0' + (value / 1000)); \
			Encoder_AppendFast('0' + ((value / 100) % 10)); \
			Encoder_AppendFast('0' + ((value / 10) % 10)); \
			Encoder_AppendFast('0' + (value % 10));

		Encoder_FN(EncodeDate) {
			Encoder_EnsureCapacity(12 + Encoder_EXTRA_CAPACITY);

			int y = PyDateTime_GET_YEAR(obj);
			int m = PyDateTime_GET_MONTH(obj);
			int d = PyDateTime_GET_DAY(obj);

			Encoder_AppendFast('"');
			EncodeDT_AppendInt4(y);
			Encoder_AppendFast('-');
			EncodeDT_AppendInt2(m);
			Encoder_AppendFast('-');
			EncodeDT_AppendInt2(d);
			Encoder_AppendFast('"');

			Encoder_RETURN_TRUE;
		}

		// TODO: maybe tz info
		Encoder_FN(EncodeTime) {
			Encoder_EnsureCapacity(14 + Encoder_EXTRA_CAPACITY);

			int h = PyDateTime_TIME_GET_HOUR(obj);
			int m = PyDateTime_TIME_GET_MINUTE(obj);
			int s = PyDateTime_TIME_GET_SECOND(obj);
			int ms = PyDateTime_TIME_GET_MICROSECOND(obj);

			Encoder_AppendFast('"');
			EncodeDT_AppendInt2(h);
			Encoder_AppendFast(':');
			EncodeDT_AppendInt2(m);
			if (s > 0 || ms > 0) {
				Encoder_AppendFast(':');
				EncodeDT_AppendInt2(s);
			}
			if (ms > 0) {
				Encoder_AppendFast('.');
				EncodeDT_AppendInt3(ms);
			}
			Encoder_AppendFast('"');

			Encoder_RETURN_TRUE;
		}

		// "2017-04-02T22:54:12+01:00"
		Encoder_FN(EncodeDateTime) {
			Encoder_EnsureCapacity(27 + Encoder_EXTRA_CAPACITY);

			int dy = PyDateTime_GET_YEAR(obj);
			int dm = PyDateTime_GET_MONTH(obj);
			int dd = PyDateTime_GET_DAY(obj);
			int th = PyDateTime_DATE_GET_HOUR(obj);
			int tm = PyDateTime_DATE_GET_MINUTE(obj);
			int ts = PyDateTime_DATE_GET_SECOND(obj);
			int tms = PyDateTime_DATE_GET_MICROSECOND(obj);

			Encoder_AppendFast('"');
			EncodeDT_AppendInt4(dy);
			Encoder_AppendFast('-');
			EncodeDT_AppendInt2(dm);
			Encoder_AppendFast('-');
			EncodeDT_AppendInt2(dd);
			Encoder_AppendFast(' ');
			EncodeDT_AppendInt2(th);
			Encoder_AppendFast(':');
			EncodeDT_AppendInt2(tm);
			Encoder_AppendFast(':');
			EncodeDT_AppendInt2(ts);
			if (tms > 0) {
				Encoder_AppendFast('.');
				EncodeDT_AppendInt3(tms);
			}

			PyObject* tzinfo = PyObject_GetAttr(obj, TZINFO_NAME);
			if (tzinfo == NULL) {
				Encoder_RETURN_FALSE;
			} else if (tzinfo != Py_None) {
				PyObject *delta = PyObject_CallMethodObjArgs(tzinfo, UTCOFFSET_METHOD_NAME, obj, NULL);
				Py_DECREF(tzinfo);

				if (delta == NULL) {
					Encoder_RETURN_FALSE;
				} else if (PyDelta_Check(delta)) {
					int utcoffset = ((PyDateTime_Delta*)delta)->seconds + ((PyDateTime_Delta*)delta)->days * 86400;
					Py_DECREF(delta);

					if (tms > 0) {
						*(buffer.cursor - 13) = 'T';
					} else {
						*(buffer.cursor - 9) = 'T';
					}

					if (utcoffset == 0) {
						Encoder_AppendFast('Z');
					} else {
						if (utcoffset < 0) {
							utcoffset = -utcoffset;
							Encoder_AppendFast('-');
						} else {
							Encoder_AppendFast('+');
						}
						int tzm = utcoffset / 60;
						int tzh = (tzm / 60) % 24;
						tzm %= 60;
						EncodeDT_AppendInt2(tzh);
						Encoder_AppendFast(':');
						EncodeDT_AppendInt2(tzm);
					}
				} else {
					PyErr_Format(PyExc_TypeError, "tzinfo.utcoffset() must return None or timedelta, not '%s'", Py_TYPE(delta)->tp_name);
					Py_DECREF(delta);
					Encoder_RETURN_FALSE;
				}
			} else {
				Py_DECREF(tzinfo);
			}

			Encoder_AppendFast('"');

			Encoder_RETURN_TRUE;
		}

		Encoder_FN(EncodeDict) {
			Encoder_EnsureCapacity(Encoder_EXTRA_CAPACITY);
			Encoder_AppendFast('{');

			if (PyDict_Size(obj) == 0) {
				Encoder_AppendFast('}');
				Encoder_RETURN_TRUE;
			}

			Encoder_EnterRecursive();

			PyObject* key;
			PyObject* value;
			Py_ssize_t pos = 0;

			while (PyDict_Next(obj, &pos, &key, &value)) {
				Encoder_AppendFast('"');
				if (EXPECT_TRUE(__encode_dict_key(key))) {
					Encoder_AppendFast('"');
					Encoder_AppendFast(':');
					if (EXPECT_TRUE(Encode(value))) {
						Encoder_AppendFast(',');
					} else Encoder_HandleRecursion(ZiboJson_Err_MaxRecursion_DictValue, value, key)
					} else {
						Encoder_RETURN_FALSE;
					}
				} else Encoder_HandleRecursion(ZiboJson_Err_MaxRecursion_DictKey, key)
				} else {
					Encoder_RETURN_FALSE;
				}
			}

			--buffer.cursor; // overwrite last ','
			Encoder_AppendFast('}');
			Encoder_LeaveRecursive();
			Encoder_RETURN_TRUE;
		}

		Encoder_FN(EncodeList) {
			Encoder_EnterRecursive();
			Encoder_EnsureCapacity(Encoder_EXTRA_CAPACITY);
			Encoder_AppendFast('[');

			register Py_ssize_t length = PyList_GET_SIZE(obj);
			register Py_ssize_t i = 0;

			for (; i<length ; i++) {
				if (Encode(PyList_GET_ITEM(obj, i))) {
					Encoder_AppendFast(',');
				} else Encoder_HandleRecursion(ZiboJson_Err_MaxRecursion_ListValue, PyList_GET_ITEM(obj, i), i)
				} else {
					Encoder_RETURN_FALSE;
				}
			}

			if (length > 0) {
				--buffer.cursor; // overwrite last ','
			}

			Encoder_AppendFast(']');
			Encoder_LeaveRecursive();
			Encoder_RETURN_TRUE;
		}

		Encoder_FN(EncodeTuple) {
			Encoder_EnterRecursive();
			Encoder_EnsureCapacity(Encoder_EXTRA_CAPACITY);
			Encoder_AppendFast('[');

			register Py_ssize_t length = PyTuple_GET_SIZE(obj);
			register Py_ssize_t i = 0;

			for (; i<length ; i++) {
				if (Encode(PyTuple_GET_ITEM(obj, i))) {
					Encoder_AppendFast(',');
				} else Encoder_HandleRecursion(ZiboJson_Err_MaxRecursion_ListValue, PyTuple_GET_ITEM(obj, i), i)
				} else {
					Encoder_RETURN_FALSE;
				}
			}

			if (length > 0) {
				--buffer.cursor; // overwrite last ','
			}

			Encoder_AppendFast(']');
			Encoder_LeaveRecursive();
			Encoder_RETURN_TRUE;
		}

		Encoder_FN(EncodeIterable) {
			Encoder_EnsureCapacity(Encoder_EXTRA_CAPACITY);
			Encoder_AppendFast('[');
			Encoder_EnterRecursive();

			PyObject *iterator = PyObject_GetIter(obj);
			if (iterator == NULL) {
				Encoder_RETURN_FALSE;
			}

			PyObject *item;
			register Py_ssize_t length = 0;

			while ((item = PyIter_Next(iterator))) {
				if (EXPECT_TRUE(Encode(item))) {
					Py_DECREF(item);
					Encoder_AppendFast(',');
					++length;
				} else {
					if (Encoder_RecursionOccured()) {
						Encoder_RecursionError(ZiboJson_Err_MaxRecursion_IterValue, item, length);
					}
					Py_DECREF(iterator);
					Py_DECREF(item);
					Encoder_RETURN_FALSE;
				}
			}
			Py_DECREF(iterator);

			if (PyErr_Occurred()) {
				Encoder_RETURN_FALSE;
			}

			if (length > 0) {
				--buffer.cursor; // overwrite last ','
			}

			Encoder_AppendFast(']');
			Encoder_LeaveRecursive();
			Encoder_RETURN_TRUE;
		}

		template<bool isDictKey>
		Encoder_FN(EncodeWithDefault) {
			Encoder_EnterRecursive();
			PyObject *toJson = PyObject_CallFunctionObjArgs(defaultFn, obj, NULL);

			if (toJson == NULL) {
				Encoder_RETURN_FALSE;
			}

			if (EXPECT_TRUE((isDictKey ? __encode_dict_key(toJson) : Encode(toJson)))) {
				Py_DECREF(toJson);
				Encoder_LeaveRecursive();
				Encoder_RETURN_TRUE;
			} else {
				Py_DECREF(toJson);
				if (Encoder_RecursionOccured()) {
					Encoder_RecursionError(ZiboJson_Err_MaxRecursion_Default, obj);
				}
			}

			Encoder_RETURN_FALSE;
		}

		template<bool isDictKey>
		Encoder_FN(EncodeWithJsonMethod) {
			Encoder_EnterRecursive();
			PyObject *toJson = PyObject_CallMethodObjArgs(obj, toJsonMethodName, NULL);

			if (toJson == NULL) {
				Encoder_RETURN_FALSE;
			}

			if (EXPECT_TRUE((isDictKey ? __encode_dict_key(toJson) : Encode(toJson)))) {
				Py_DECREF(toJson);
				Encoder_LeaveRecursive();
				Encoder_RETURN_TRUE;
			} else {
				Py_DECREF(toJson);
				if (Encoder_RecursionOccured()) {
					Encoder_RecursionError(ZiboJson_Err_MaxRecursion_JsonMethod, obj, toJsonMethodName);
				}
			}

			Encoder_RETURN_FALSE;
		}
};


} /* namespace ZiboJson */

#endif /* DEFBD7C4_2133_C63D_128D_F5D3D59111EF */
