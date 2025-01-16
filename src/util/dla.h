/*==========================================================*
 *
 * @author Gustaf Franzén :: https://github.com/BjorneEk;
 *
 * dynamic length array
 *
 *==========================================================*/

#ifndef _DLA_H_
#define _DLA_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct dla {void *data; size_t allocated; size_t length;} dla_t;
#define _GEN_get(ts, ty, name)		ts ty name ## _get(dla_t *dla, size_t i) {return ((ty*)dla->data)[i];}
#define _GEN_set(ts, ty, name)		ts void name ## _set(dla_t *dla, size_t i, ty val) { ((ty*)dla->data)[i] = val;}
#define _GEN_getp(ts, ty, name)		ts ty * name ## _getp(dla_t *dla, size_t i) {return &((ty*)dla->data)[i];}
#define _GEN_pop(ts, ty, name)		ts ty name ## _pop(dla_t *dla) { return ((ty*)dla->data)[dla->length--]; }
#define _GEN_len(ts, ty, name)		ts size_t name ## _len(dla_t *dla) { return dla->length; }
#define _GEN_deinit(ts, ty, name)	ts void name ## _deinit(dla_t *dla) { free(dla->data); dla->data = NULL; dla->length = 0;}
#define _GEN_clear(ts, ty, name)	ts void name ## _clear(dla_t *dla) { dla->length = 0; }

#define _GEN_init(ts, ty, name)										\
ts void name ## _init(dla_t *dla, size_t isz)								\
{													\
	dla->data = NULL;										\
	if (isz == 0)											\
		goto end;										\
	dla->data = malloc(isz * sizeof(ty));								\
	if (!dla->data) {										\
		fprintf(stderr, "dla: malloc failed, Requested size: %zu\n", isz * sizeof(ty));		\
		exit(-1);										\
	}												\
end:													\
	dla->allocated = isz;										\
	dla->length = 0;										\
}

#define _GEN_push(ts, ty, name)												\
ts void name ## _push(dla_t *dla, ty d)											\
{															\
	void *newd;													\
	if (dla->allocated <= dla->length + 1) {									\
		dla->allocated *= 2;											\
		newd = realloc(dla->data, dla->allocated * sizeof(ty));							\
		if (!newd) {												\
			fprintf(stderr, "dla: realloc failed, Requested size: %zu\n", dla->allocated * sizeof(ty));	\
			exit(-1);											\
		}													\
		dla->data = newd;											\
	}														\
	memmove(&((ty*)dla->data)[dla->length++], &d, sizeof(ty));							\
}

#define _GEN_new(ts, ty, name)												\
ts ty * name ## _new(dla_t *dla)											\
{															\
	void *newd;													\
	if (dla->allocated <= dla->length + 1) {									\
		dla->allocated *= 2;											\
		newd = realloc(dla->data, dla->allocated * sizeof(ty));							\
		if (!newd) {												\
			fprintf(stderr, "dla: realloc failed, Requested size: %zu\n", dla->allocated * sizeof(ty));	\
			exit(-1);											\
		}													\
		dla->data = newd;											\
	}														\
	return &((ty*)dla->data)[dla->length++];									\
}

#define _GEN_alloc(ts, ty, name)											\
ts void name ## _alloc(dla_t *dla, size_t n)										\
{															\
	void *newd;													\
	if (dla->allocated <= dla->length + n) {									\
		dla->allocated += n;											\
		newd = realloc(dla->data, dla->allocated * sizeof(ty));							\
		if (!newd) {												\
			fprintf(stderr, "dla: realloc failed, Requested size: %zu\n", dla->allocated * sizeof(ty));	\
			exit(-1);											\
		}													\
		dla->data = newd;											\
	}														\
}


#define DLA_FOREACHP_ITER(_dla, _type, _name,_iname, ...) do {	\
	size_t _iname;						\
	_type *_data__ = (_type*)(_dla)->data;			\
	for (_iname = 0;					\
		_iname < (_dla)->length;			\
		_iname++) {					\
		_type * _name = &_data__[_iname];		\
		__VA_ARGS__;					\
	}							\
}while(0);

#define DLA_FOREACH(_dla, _type, _name, ...)			DLA_FOREACHP_ITER(_dla, _type, _name ## _ptr, FOREACH_DLA_iter_##_name, {_type _name = * _name ## _ptr; __VA_ARGS__ ;})
#define DLA_FOREACHP(_dla, _type, _name, ...)			DLA_FOREACHP_ITER(_dla, _type, _name, FOREACH_DLA_iter_##_name, __VA_ARGS__)
#define DLA_FOREACH_ITER(_dla, _type, _name, _iname, ...)	DLA_FOREACHP_ITER(_dla, _type, _name ## _ptr, _iname, {_type _name = * _name ## _ptr; __VA_ARGS__ ;})

#define DLA_WHERE_UNSAFE(_dla, _type, _name, _expr, ...)		DLA_FOREACHP_ITER(_dla, _type, _name, WHERE_DLA_iter_##_name, if (_expr) __VA_ARGS__)
#define DLA_WHERE_ITER_UNSAFE(_dla, _type, _name, _iname, _expr, ...)	DLA_FOREACHP_ITER(_dla, _type, _name, _iname, if (_expr) __VA_ARGS__)
#define DLA_WHERE(_dla, _type, _name, _expr, ...)			DLA_WHERE_UNSAFE(_dla, _type, _name, _expr , {__VA_ARGS__})
#define DLA_WHERE_ITER(_dla, _type, _name, _iname, _expr, ...)		DLA_WHERE_ITER_UNSAFE(_dla, _type, _name, _iname, _expr , {__VA_ARGS__})

#define DLA_FILTER(_dla, _type, _name, _expr, ...) do{					\
	size_t FILTER_DLA_len_ ## _name = 0;						\
	DLA_WHERE_ITER_UNSAFE((_dla), _type, _name, FILTER_DLA_iter_ ## _name, _expr, {	\
		if (FILTER_DLA_len_ ## _name++ == FILTER_DLA_iter_ ## _name)		\
			continue;							\
		((_type*)((_dla)->data))[FILTER_DLA_len_ ## _name - 1] = *_name;	\
	} __VA_OPT__(else {__VA_ARGS__;}))						\
	(_dla)->length = FILTER_DLA_len_ ## _name;					\
} while (0);

#define DLA_FOLD(_dla, _type, _name, _acc_type, _accname, _accinit, ...) ({		\
	_acc_type _accname = _accinit;							\
	DLA_FOREACH((_dla), _type, _name, __VA_ARGS__);					\
	_accname;									\
})

#define _CAT(x, y) x ## y
#define _XCAT(x, y) _CAT(x, y)

#define _VA_LEN(...) _XVA_LEN(__VA_ARGS__ __VA_OPT__(,) 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define _XVA_LEN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

#define _GEN(f, ty, name) _CAT(_GEN_, f) (, ty, name)
#define _GENstatic(f, ty, name) _CAT(_GEN_, f) (static, ty, name)

#define _DLA_DECL_0(ts, ty, name)
#define _DLA_DECL_1(ts, ty, name, f1) _GEN ## ts (f1, ty, name)
#define _DLA_DECL_2(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_1(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_3(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_2(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_4(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_3(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_5(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_4(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_6(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_5(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_7(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_6(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_8(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_7(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_9(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_8(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_10(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_9(ts, ty, name, __VA_ARGS__)
#define _DLA_DECL_11(ts, ty, name, f1, ...) _GEN ## ts (f1, ty, name) _DLA_DECL_10(ts, ty, name, __VA_ARGS__)

#define DLA_GEN(ts, ty, name, ...) _XCAT(_DLA_DECL_, _VA_LEN(__VA_ARGS__)) (ts, ty, name __VA_OPT__(,) __VA_ARGS__)

#define _DLA_POP_0(_dla, _ty, len, e1, ...) (_dla)->length = len; if ((_dla)->allocated < (_dla)->length) {exit(-1);}
#define _DLA_POP_1(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_0(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_2(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_1(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_3(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_2(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_4(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_3(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_5(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_4(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_6(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_5(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_7(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_6(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_8(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_7(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_9(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_8(_dla, _ty, len, __VA_ARGS__)
#define _DLA_POP_10(_dla, _ty, len, e1, ...) ((_ty*)(_dla)->data)[len - _VA_LEN(__VA_ARGS__) - 1] = e1; _DLA_POP_9(_dla, _ty, len, __VA_ARGS__)

#define DLA_FILL(_dla, ty, ...) _XCAT(_DLA_POP_, _VA_LEN(__VA_ARGS__)) ((_dla), ty, _VA_LEN(__VA_ARGS__), __VA_ARGS__);


#endif /* _DLA_H_ */