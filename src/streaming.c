/******************************************************************************
 * @file Streaming
 * @brief Streaming data through network
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <string.h>
#include "streaming.h"
#include "luos_utils.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief Initialisation of a streaming channel.
 * @param ring_buffer Pointer to a data table
 * @param ring_buffer_size size of the buffer in number of values.
 * @param data_size values size.
 * @return streaming channel
 ******************************************************************************/
streaming_channel_t Stream_CreateStreamingChannel(const void *ring_buffer, uint16_t ring_buffer_size, uint8_t data_size)
{
    streaming_channel_t stream;
    LUOS_ASSERT((ring_buffer != NULL) || (ring_buffer_size > 0) || (data_size > 0));
    // Save ring buffer informations
    stream.ring_buffer     = (void *)ring_buffer;
    stream.data_size       = data_size;
    stream.end_ring_buffer = (void *)ring_buffer + (stream.data_size * ring_buffer_size);

    // Set data pointers to 0
    stream.data_ptr   = stream.ring_buffer;
    stream.sample_ptr = stream.ring_buffer;
    return stream;
}
/******************************************************************************
 * @brief re initialize a streaming channel.
 * @param stream streaming channel pointer
 * @return None
 ******************************************************************************/
void Stream_ResetStreamingChannel(streaming_channel_t *stream)
{
    stream->data_ptr   = stream->ring_buffer;
    stream->sample_ptr = stream->ring_buffer;
}
/******************************************************************************
 * @brief set data into ring buffer.
 * @param stream streaming channel pointer
 * @param data a pointer to the data table
 * @param size The number of data to copy
 * @return number of available samples
 ******************************************************************************/
uint8_t Stream_PutSample(streaming_channel_t *stream, const void *data, uint16_t size)
{
    if (((size * stream->data_size) + stream->data_ptr) > stream->end_ring_buffer)
    {
        // our data exceeds ring buffer end, cut it and copy.
        uint16_t chunk1 = stream->end_ring_buffer - stream->data_ptr;
        uint16_t chunk2 = (size * stream->data_size) - chunk1;
        // check if we exceed ring buffer capacity
        LUOS_ASSERT(stream->sample_ptr >= (stream->ring_buffer + chunk2));
        // Everything good copy datas.
        memcpy(stream->data_ptr, data, chunk1);
        memcpy(stream->ring_buffer, (char *)data + chunk1, chunk2);
        // Set the new data pointer
        stream->data_ptr = stream->ring_buffer + chunk2;
    }
    else
    {
        // our data fit before ring buffer end
        // check if we exceed ring buffer capacity
        LUOS_ASSERT((stream->data_ptr >= stream->sample_ptr) && ((stream->data_ptr + (size * stream->data_size)) <= stream->end_ring_buffer));
        memcpy(stream->data_ptr, data, (size * stream->data_size));
        // Set the new data pointer
        stream->data_ptr = stream->data_ptr + (size * stream->data_size);
    }
    return Stream_GetAvailableSampleNB(stream);
}
/******************************************************************************
 * @brief copy a sample from ring buffer to a data.
 * @param stream streaming channel pointer
 * @param data a pointer of data
 * @param size data
 * @return None
 ******************************************************************************/
uint8_t Stream_GetSample(streaming_channel_t *stream, void *data, uint16_t size)
{
    uint16_t nb_available_samples = Stream_GetAvailableSampleNB(stream);
    if (nb_available_samples >= size)
    {
        // check if we need to loop in ring buffer
        if ((stream->sample_ptr + (size * stream->data_size)) > stream->end_ring_buffer)
        {
            // requested data exceeds ring buffer end, cut it and copy.
            int chunk1 = stream->end_ring_buffer - stream->sample_ptr;
            int chunk2 = (size * stream->data_size) - chunk1;
            memcpy(data, stream->sample_ptr, chunk1);
            memcpy((char *)data + chunk1, stream->ring_buffer, chunk2);
            // Set the new sample pointer
            stream->sample_ptr = stream->ring_buffer + chunk2;
        }
        else
        {
            data = memcpy(data, stream->sample_ptr, (size * stream->data_size));
            // Set the new sample pointer
            stream->sample_ptr = stream->sample_ptr + (size * stream->data_size);
        }

        nb_available_samples -= size;
    }
    else
    {
        // no more data
        return 0;
    }
    return nb_available_samples;
}
/******************************************************************************
 * @brief return the number of available samples
 * @param stream streaming channel pointer
 * @return number of available samples
 ******************************************************************************/
uint8_t Stream_GetAvailableSampleNB(streaming_channel_t *stream)
{
    uint16_t nb_available_sample = (stream->data_ptr - stream->sample_ptr) / stream->data_size;
    if (nb_available_sample < 0)
    {
        // The buffer have looped
        nb_available_sample = ((stream->end_ring_buffer - stream->sample_ptr) + (stream->data_ptr - stream->ring_buffer)) / stream->data_size;
    }
    return nb_available_sample;
}
/******************************************************************************
 * @brief set a number of sample available in buffer
 * @param stream streaming channel pointer
 * @param size The number of data to copy
 * @return number of available samples
 ******************************************************************************/
uint8_t Stream_SetAvailableSampleNB(streaming_channel_t *stream, uint16_t size)
{
    if (((size * stream->data_size) + stream->data_ptr) > stream->end_ring_buffer)
    {
        uint16_t chunk1 = stream->end_ring_buffer - stream->data_ptr;
        uint16_t chunk2 = (size * stream->data_size) - chunk1;
        stream->data_ptr = stream->ring_buffer + chunk2;
    }
    else
    {
    	stream->data_ptr = stream->data_ptr + (size * stream->data_size);
    }
	return Stream_GetAvailableSampleNB(stream);
}
