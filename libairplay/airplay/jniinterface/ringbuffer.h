#ifndef __RINGBUFFER_H
#define	__RINGBUFFER_H
#if defined (__cplusplus)
extern "C" {
#endif

typedef struct {
    char *buf;
    size_t len;
}
ringbuffer_data_t ;

typedef struct {
    char	*buf;
    size_t	write_ptr;
    size_t	read_ptr;
    size_t	size;
    size_t	size_mask;
    int	mlocked;
}
ringbuffer_t ;

ringbuffer_t *ringbuffer_create(size_t sz);
void ringbuffer_destroy(ringbuffer_t *rb);
void ringbuffer_get_read_vector(const ringbuffer_t *rb,
                                         ringbuffer_data_t *vec);
void ringbuffer_get_write_vector(const ringbuffer_t *rb,
                                          ringbuffer_data_t *vec);
size_t ringbuffer_get(ringbuffer_t *rb, char *dest, size_t cnt);
size_t ringbuffer_peek(ringbuffer_t *rb, char *dest, size_t cnt);
void ringbuffer_read_advance(ringbuffer_t *rb, size_t cnt);
size_t ringbuffer_read_space(const ringbuffer_t *rb);
int ringbuffer_mlock(ringbuffer_t *rb);
void ringbuffer_reset(ringbuffer_t *rb);
void ringbuffer_reset_size (ringbuffer_t * rb, size_t sz);
size_t ringbuffer_put(ringbuffer_t *rb, const char *src,
                                 size_t cnt);
void ringbuffer_write_advance(ringbuffer_t *rb, size_t cnt);
size_t ringbuffer_write_space(const ringbuffer_t *rb);
int ringbuffer_is_empty(const ringbuffer_t *ring_buf);

 

#if defined (__cplusplus)
}
#endif
#endif
