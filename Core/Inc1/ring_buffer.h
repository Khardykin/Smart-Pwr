//-----------------------------------------------------------------------------
//
//  Модуль функций обслуживания кольцевого буфера
//
//  ring_buffer.h                                                    22.04.2019
//
//-----------------------------------------------------------------------------

#ifndef RING_BUFFER_H_INCLUDED
#define RING_BUFFER_H_INCLUDED

typedef struct {
  volatile uint16_t write_offset;
  volatile uint16_t read_offset;
  uint16_t size;
  uint8_t *buffer;
}ring_buffer;

static inline uint16_t get_next(uint16_t cur_offset, uint16_t max_offset)
{
  return (cur_offset == max_offset-1 ? 0 : cur_offset + 1);
}

static inline uint16_t ring_buffer_get_next_write(const ring_buffer *ring)
{
  return get_next(ring->write_offset, ring->size);
}

static inline uint16_t ring_buffer_get_next_read(const ring_buffer *ring)
{
  return get_next(ring->read_offset, ring->size);
}

static inline uint8_t ring_buffer_is_full(const ring_buffer *ring)
{
  return (ring->read_offset == ring_buffer_get_next_write(ring));
}

static inline uint8_t ring_buffer_is_empty(const ring_buffer *ring)
{
  return (ring->read_offset == ring->write_offset);
}

static inline ring_buffer ring_buffer_init(uint8_t *buffer, uint16_t size)
{
  ring_buffer ring;
  ring.write_offset = 0;
  ring.read_offset = 0;
  ring.size = size;
  ring.buffer = buffer;
  return ring;
}

static inline uint8_t ring_buffer_get(ring_buffer *ring)
{
  uint8_t data = ring->buffer[ring->read_offset];
  ring->read_offset = ring_buffer_get_next_read(ring);
  return data;
}

static inline void ring_buffer_put(ring_buffer *ring, uint8_t data)
{
  ring->buffer[ring->write_offset] = data;
  ring->write_offset = ring_buffer_get_next_write(ring);
}



#endif /* RING_BUFFER_H_INCLUDED */
