/******************************************************************************
 * @file Streaming
 * @brief Streaming data through network
 *
 *  Streaming channel
 *  This structure manage a ring buffer as a streaming channel.
 *
 *   |--------------------- ring_buffer_size --------------------|
 *   |*****************|...............|                         |
 *   ^                 ^               ^                         ^
 * ring_buffer    sample_ptr       data_ptr               end_ring_buffer
 *
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef STREAMING_H
#define STREAMING_H

#include <stdint.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct
{
    void *ring_buffer;     // Begin ring buffer pointer
    void *end_ring_buffer; // End of the ring buffer
    void *sample_ptr;      // Current sample pointer (pointer always point a fresh data)
    void *data_ptr;        // Current pointer of data
    uint8_t data_size;     // Size granulariry of the data contained on the ring buffer
} streaming_channel_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
streaming_channel_t Stream_CreateStreamingChannel(const void *ring_buffer, uint16_t ring_buffer_size, uint8_t data_size);
void Stream_ResetStreamingChannel(streaming_channel_t *stream);
uint8_t Stream_PutSample(streaming_channel_t *stream, const void *data, uint16_t size);
uint8_t Stream_GetSample(streaming_channel_t *stream, void *data, uint16_t size);
uint8_t Stream_GetAvailableSampleNB(streaming_channel_t *stream);
uint8_t Stream_GetAvailableSampleNBUntilEndBuffer(streaming_channel_t *stream);
uint8_t Stream_AddAvailableSampleNB(streaming_channel_t *stream, uint16_t size);
uint8_t Stream_RmvAvailableSampleNB(streaming_channel_t *stream, uint16_t size);

#endif /* LUOS_H */
