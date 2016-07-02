#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#define report_exceptional_condition() abort()

struct ring_buffer  
{ 
    unsigned long count_bytes;
    unsigned long write_offset_bytes;
    unsigned long read_offset_bytes;
    void *address;
};


void ring_buffer_create(struct ring_buffer *buffer, unsigned long order);

void ring_buffer_free(struct ring_buffer *buffer);

void *ring_buffer_write_address(struct ring_buffer *buffer);

void ring_buffer_write_advance(struct ring_buffer *buffer,
                           unsigned long count_bytes);

void *ring_buffer_read_address(struct ring_buffer *buffer);

void ring_buffer_read_advance(struct ring_buffer *buffer,
                          unsigned long count_bytes);

unsigned long ring_buffer_count_bytes(struct ring_buffer *buffer);

unsigned long ring_buffer_count_free_bytes(struct ring_buffer *buffer);

void ring_buffer_clear(struct ring_buffer *buffer);


#endif

