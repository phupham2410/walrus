#ifndef __STDMACRO_H__
#define __STDMACRO_H__

typedef char  S8;
typedef short S16;
typedef int   S32;

typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;
typedef unsigned long long U64;

#define MIN2(a, b) ( ((a) < (b))? (a) : (b))
#define MAX2(a, b) ( ((a) > (b))? (a) : (b))

#define MIN3(a,b,c) MIN2(a,MIN2(b,c))
#define MAX3(a,b,c) MAX2(a,MAX2(b,c))

#define INRANGE(beg, end, val) (((beg) <= (val)) && ((val) < (end)))

#define FORMAT_BUFFER(buff, ...) char buff[512]; sprintf(buff, __VA_ARGS__)
#define FORMAT_STRING(name, ...) do { FORMAT_BUFFER(__tmpbuff, __VA_ARGS__); name = string(__tmpbuff); } while(0)

#define FILL_REGION(name, value, size) do { for (U32 i = 0; i < size; i++) name[i] = value; } while(0)
#define FILL_ARRAY(name, value) do { U32 itemCount = sizeof (name) / sizeof (name[0]); FILL_REGION(name, value, itemCount); } while(0)
#define MAKE_ARRAY(name, cont) do { U32 size = sizeof (name) / sizeof (name[0]); for (U32 i = 0; i < size; i++) cont.push_back(name[i]); } while(0)

#define FRMT_DBL(w, val) setw(w) << val
#define FRMT_U32(w, val) setw(w) << setfill('0') << dec << (U32) val
#define FRMT_STR(w, str) setw(w) << setfill(' ') << str
#define FRMT_DEC(w) setw(w) << setfill('0') << dec
#define FRMT_HEX(w) setw(w) << setfill('0') << hex

#define ASSERT(x)  if (!(x)) { FORMAT_BUFFER(__tmpbuff, "expr(%s)\n         file(%s)\n         line(%d)", #x, __FILE__, __LINE__); printf("[assert] %s\n", __tmpbuff); while(1); }

#define GET_BIT(val, idx) (((val) >> (idx)) & 0x1)
#define GET_WORD(buff, idx) (U16) ((buff[(idx) * 2 + 1] << 8) | buff[(idx) * 2])
#define GET_DWORD(buff, idx) (U32) ((buff[(idx) * 4 + 3] << 24) | (buff[(idx) * 4 + 2] << 16) | (buff[(idx) * 4 + 1] << 8) | (buff[(idx) * 4 + 0]))

// -----------------------------------------------------------
// Extensions
// -----------------------------------------------------------

#define ALLOC_OBJ(type, name) name = new type
#define ALLOC_PTR(type, name) type* name = new type

#define DEF_TYPE02(name, type0, name0, type1, name1) \
struct name { \
    type0 name0; type1 name1; \
    name(type0 p0, type1 p1)  \
        : name0(p0), name1(p1) {} \
};

#define DEF_TYPE03(name, type0, name0, type1, name1, type2, name2) \
struct name { \
    type0 name0; type1 name1; type2 name2; \
    name(type0 p0, type1 p1, type2 p2)  \
        : name0(p0), name1(p1), name2(p2) {} \
};

#define DEF_TYPE04(name, type0, name0, type1, name1, type2, name2, type3, name3) \
struct name { \
    type0 name0; type1 name1; type2 name2; type3 name3; \
    name(type0 p0, type1 p1, type2 p2, type3 p3) \
        : name0(p0), name1(p1), name2(p2), name3(p3) {} \
};

#define DEF_TYPE05(name, type0, name0, type1, name1, type2, name2, type3, name3, type4, name4) \
struct name { \
    type0 name0; type1 name1; type2 name2; type3 name3; type4 name4; \
    name(type0 p0, type1 p1, type2 p2, type3 p3, type4 p4) \
        : name0(p0), name1(p1), name2(p2), name3(p3), name4(p4) {} \
};

#endif // __STDMACRO_H__
