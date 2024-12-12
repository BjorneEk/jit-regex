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
#include <string.h>
#include <ctype.h>

typedef struct dla {
	uint8_t	*data;
	size_t	allocated;	/* total allocated bytes */
	size_t	length;		/* number of elements */
} dla_t;

static inline
size_t	DLA_length(dla_t *self)
{
	return self->length;
}

static inline
void	DLA_init(dla_t *res, size_t width, size_t len)
{
	res->allocated	= len * width;
	res->length	= 0;
	res->data	= (uint8_t*)calloc(len, width);
}

static inline
dla_t	*DLA_new(size_t width, size_t len)
{
	dla_t	*res;

	res = (dla_t*)malloc(sizeof(dla_t));
	DLA_init(res, width, len);
	return res;
}

static inline
void	DLA_expandn(dla_t *self, size_t bytes)
{
	self->allocated += bytes;
	self->data = (uint8_t*)realloc(self->data, self->allocated);
}

static inline
void	DLA_expand(dla_t *self, size_t width)
{
	DLA_expandn(self, self->allocated > width ? self->allocated : width);
}

static inline
void	DLA_insert(dla_t *self, size_t width, uint64_t index, uint8_t *data)
{
	if (self->length * width == self->allocated)
		DLA_expand(self, width);

	/* move elements one step back in array to make room for the new one */
	memmove(
		self->data + (index + 1)	* width,	/*	dest	*/
		self->data + index		* width,	/*	source	*/
		self->length - index * width			/*	size	*/
	);

	memcpy(self->data + (index * width), data, width);
	++self->length;
}

static inline
void	DLA_append(dla_t *self, size_t width, uint8_t *data)
{
	if (self->length * width == self->allocated)
		DLA_expand(self, width);

	memcpy(self->data + self->length * width, data, width);
	++self->length;
}
static inline
void DLA_next(dla_t *self, size_t width, uint32_t *index)
{
	if (self->length * width == self->allocated)
		DLA_expand(self, width);
	*index = self->length;
	++self->length;
}

static inline
void	DLA_appendn(dla_t *self, size_t width, uint8_t *data, size_t count)
{
	if ((self->length + count) * width > self->allocated)
		DLA_expand(self, count * width - (self->allocated - self->length * width));

	memcpy(self->data + self->length * width, data, width * count);
	self->length += count;
}

static inline
void	DLA_append_dla(dla_t *self, dla_t *other, size_t width)
{
	size_t	other_bc;
	size_t	self_bc;

	other_bc = other->length * width;
	self_bc = self->length * width;

	if (self->allocated - self_bc < other_bc)
		DLA_expandn(self, other_bc - (self->allocated - self_bc));

	memcpy(self->data + self_bc, other->data, other_bc);
	self->length += other->length;
}

static inline
void	*DLA_get(dla_t *self, size_t width, size_t index)
{
	return self->data + width * index;
}

#define CODEGEN_DLA(value_type, prefix)						\
static										\
size_t	prefix##_length(dla_t *self)						\
{										\
	return DLA_length(self);						\
}										\
										\
static										\
void	prefix##_init(dla_t *self, size_t len)					\
{										\
	DLA_init(self, sizeof(value_type), len);				\
}										\
										\
static										\
dla_t	*prefix##_new(size_t len)						\
{										\
	return DLA_new(sizeof(value_type), len);				\
}										\
										\
static										\
void	prefix##_expandn(dla_t *self, size_t count)				\
{										\
	DLA_expandn(self, sizeof(value_type) * count);				\
}										\
										\
static										\
void	prefix##_expand(dla_t *self, size_t width)				\
{										\
	DLA_expand(self, sizeof(value_type));					\
}										\
										\
static										\
void	prefix##_insert(dla_t *self, uint64_t index, value_type val)		\
{										\
	DLA_insert(self, sizeof(value_type), index, (uint8_t*)&val);		\
}										\
										\
static										\
void	prefix##_append(dla_t *self, value_type val)				\
{										\
	DLA_append(self, sizeof(value_type), (uint8_t*)&val);			\
}										\
										\
static										\
void	prefix##_next(dla_t *self, uint32_t *index)				\
{										\
	DLA_next(self, sizeof(value_type), index);				\
}										\
										\
static										\
void	prefix##_appendn(dla_t *self, value_type *val, size_t count)		\
{										\
	DLA_appendn(self, sizeof(value_type), (uint8_t*)val, count);		\
}										\
										\
static										\
value_type	prefix##_get(dla_t *self, size_t index)				\
{										\
	return *(value_type*)DLA_get(self, sizeof(value_type), index);		\
}										\
static										\
value_type*	prefix##_getptr(dla_t *self, size_t index)			\
{										\
	return (value_type*)DLA_get(self, sizeof(value_type), index);		\
}										\
										\
static	inline									\
void	prefix##_deinit(dla_t *self)						\
{										\
	free(self->data);							\
	self->length = self->allocated = 0;					\
}										\
static inline									\
void	prefix##_swp(dla_t *self, size_t i1, size_t i2)				\
{										\
	value_type tmp;								\
	tmp = prefix##_get(self, i1);						\
	((value_type*)self->data)[i1] = prefix##_get(self, i2);			\
	((value_type*)self->data)[i2] = tmp;					\
}										\
static	inline									\
void	prefix##_flush(dla_t *self)						\
{										\
	self->length = 0;							\
}

#define DLA_FOREACH(_dla, _type, _name, __stmt__) do {				\
	size_t FOREACH_DLA_iter_##_name;					\
	_type *_data__ = (_type*)(_dla)->data;					\
	for (FOREACH_DLA_iter_##_name = 0;					\
		FOREACH_DLA_iter_##_name < (_dla)->length;			\
		FOREACH_DLA_iter_##_name++) {					\
		_type _name = _data__[FOREACH_DLA_iter_##_name];		\
		__stmt__;							\
	}									\
}while(0);

#define DLA_FOREACH_IDX(_dla, _idx, _type, _name, __stmt__) do {		\
	size_t _idx;								\
	_type *_data__ = (_type*)(_dla)->data;					\
	for (_idx = 0; _idx < (_dla)->length; _idx++) {				\
		_type _name = _data__[_idx];					\
		__stmt__;							\
	}									\
}while(0);

#define DLA_FOREACH_PTR_IDX(_dla, _idx, _type, _name, __stmt__) do {		\
	size_t _idx;								\
	_type *_data__ = (_type *)(_dla)->data;					\
	for (_idx = 0; _idx < (_dla)->length; _idx++) {				\
		_type *_name = &_data__[_idx];					\
		__stmt__;							\
	}									\
} while(0);

#define DLA_FOREACH_PTR(_dla, _type, _name, __stmt__) do {			\
	size_t FOREACH_DLA_iter_ptr_##_name;					\
	_type *_data__ = (_type *)(_dla)->data;					\
	for (FOREACH_DLA_iter_ptr_##_name = 0;					\
		FOREACH_DLA_iter_ptr_##_name < (_dla)->length;			\
		FOREACH_DLA_iter_ptr_##_name++) {				\
		_type *_name = &_data__[FOREACH_DLA_iter_ptr_##_name];		\
		__stmt__;							\
	}									\
}while(0);

#define DLA_MAP(_dla, _type, _name, _restype, _resname, __stmt__) 	\
	({								\
		_restype	*result_arr;				\
	do {								\
	size_t		_idx;						\
	_type		*_data__;					\
	result_arr = malloc((_dla)->length * sizeof(_restype));		\
	_data__ = (_type *)(_dla)->data;				\
	for (_idx = 0; _idx < (_dla)->length; _idx++) {		\
		_type		_name = _data__[_idx];			\
		_restype	_resname;				\
		__stmt__;						\
		result_arr[_idx] = _resname;				\
	}								\
}while(0); result_arr;})

#endif /* _DLA_H_ */
