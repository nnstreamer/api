/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 */

package org.nnsuite.nnstreamer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

/**
 * Provides interfaces to handle tensor data frame.
 */
public final class TensorsData implements AutoCloseable {
    private TensorsInfo mInfo = null;
    private ArrayList<ByteBuffer> mDataList = new ArrayList<>();

    /**
     * Internal method to allocate a new direct byte buffer with the native byte order.
     */
    private static ByteBuffer allocateBuffer(int size) {
        return ByteBuffer.allocateDirect(size).order(ByteOrder.nativeOrder());
    }

    /**
     * Allocates a new direct byte buffer with the native byte order.
     *
     * @param size The byte size of the buffer
     *
     * @return The new byte buffer
     *
     * @throws IllegalArgumentException if given size is invalid
     */
    public static ByteBuffer allocateByteBuffer(int size) {
        if (size <= 0) {
            throw new IllegalArgumentException("Given size is invalid");
        }

        return allocateBuffer(size);
    }

    /**
     * Allocates a new direct byte buffer with the native byte order and sets given byte array.
     *
     * @param bytes The contents of the buffer
     *
     * @return The new byte buffer
     *
     * @throws IllegalArgumentException if given data is invalid
     */
    public static ByteBuffer allocateByteBuffer(byte[] bytes) {
        if (bytes == null || bytes.length == 0) {
            throw new IllegalArgumentException("Given data is invalid");
        }

        return allocateBuffer(bytes.length).put(bytes);
    }

    /**
     * Allocates a new direct byte buffer with the native byte order and sets given string.
     *
     * @param text The string data of the buffer
     *
     * @return The new byte buffer
     *
     * @throws IllegalArgumentException if given data is invalid
     */
    public static ByteBuffer allocateByteBuffer(String text) {
        if (text == null || text.isEmpty()) {
            throw new IllegalArgumentException("Given data is invalid");
        }

        return allocateByteBuffer(text.getBytes(StandardCharsets.UTF_8));
    }

    /**
     * Allocates a new {@link TensorsData} instance with the given tensors information.
     *
     * @param info The tensors information
     *
     * @return {@link TensorsData} instance
     *
     * @throws IllegalArgumentException if given tensors information is invalid
     */
    public static TensorsData allocate(TensorsInfo info) {
        TensorsData data = new TensorsData(info);
        int count = info.getTensorsCount();

        for (int i = 0; i < count; i++) {
            /* If tensor format is flexible, data size would be 0. */
            data.addTensorData(allocateBuffer(info.getTensorSize(i)));
        }

        return data;
    }

    /**
     * Gets the tensors information.
     *
     * @return {@link TensorsInfo} instance cloned from current tensors information.
     */
    public TensorsInfo getTensorsInfo() {
        return mInfo.clone();
    }

    /**
     * Sets the tensors information.
     *
     * @param info The tensors information
     *
     * @throws IllegalArgumentException if given tensors information is invalid
     */
    private void setTensorsInfo(TensorsInfo info) {
        if (info == null || info.getTensorsCount() == 0) {
            throw new IllegalArgumentException("Given tensors information is invalid");
        }

        mInfo = info.clone();
    }

    /**
     * Gets the number of tensors in tensors data.
     *
     * @return The number of tensors
     */
    public int getTensorsCount() {
        return mDataList.size();
    }

    /**
     * Adds a new tensor data.
     *
     * @param data The tensor data to be added
     *
     * @throws IllegalArgumentException if given data is invalid
     * @throws IndexOutOfBoundsException when the maximum number of tensors in the list
     */
    private void addTensorData(ByteBuffer data) {
        int index = getTensorsCount();

        checkByteBuffer(index, data);
        mDataList.add(data);
    }

    /**
     * Gets a tensor data of given index.
     *
     * @param index The index of the tensor in the list
     *
     * @return The tensor data
     *
     * @throws IndexOutOfBoundsException if the given index is invalid
     */
    public ByteBuffer getTensorData(int index) {
        checkIndexBounds(index);
        return mDataList.get(index);
    }

    /**
     * Sets a tensor data.
     *
     * @param index The index of the tensor in the list
     * @param data  The tensor data
     *
     * @throws IndexOutOfBoundsException if the given index is invalid
     * @throws IllegalArgumentException if given data is invalid
     */
    public void setTensorData(int index, ByteBuffer data) {
        checkIndexBounds(index);
        checkByteBuffer(index, data);

        mDataList.set(index, data);
    }

    /**
     * Internal method called from native to get the array of tensor data.
     */
    private Object[] getDataArray() {
        return mDataList.toArray();
    }

    /**
     * Internal method called from native to reallocate buffer.
     */
    private void updateData(int index, int size) {
        checkIndexBounds(index);

        mDataList.set(index, allocateByteBuffer(size));
    }

    /**
     * Internal method to check the index.
     *
     * @throws IndexOutOfBoundsException if the given index is invalid
     */
    private void checkIndexBounds(int index) {
        if (index < 0 || index >= getTensorsCount()) {
            throw new IndexOutOfBoundsException("Invalid index [" + index + "] of the tensors");
        }
    }

    /**
     * Internal method to check byte buffer.
     *
     * @throws IllegalArgumentException if given data is invalid
     * @throws IndexOutOfBoundsException if the given index is invalid
     */
    private void checkByteBuffer(int index, ByteBuffer data) {
        if (data == null) {
            throw new IllegalArgumentException("Given data is null");
        }

        if (!data.isDirect()) {
            throw new IllegalArgumentException("Given data is not a direct buffer");
        }

        if (data.order() != ByteOrder.nativeOrder()) {
            /* Default byte order of ByteBuffer in java is big-endian, it should be a little-endian. */
            throw new IllegalArgumentException("Given data has invalid byte order");
        }

        if (index >= NNStreamer.TENSOR_SIZE_LIMIT) {
            throw new IndexOutOfBoundsException("Max size of the tensors is " + NNStreamer.TENSOR_SIZE_LIMIT);
        }

        /* compare to tensors info */
        if (mInfo != null) {
            int count = mInfo.getTensorsCount();

            if (index >= count) {
                throw new IndexOutOfBoundsException("Current information has " + count + " tensors");
            }

            NNStreamer.TensorFormat format = mInfo.getFormat();

            /* The size of input buffer should be matched if data format is static. */
            if (format == NNStreamer.TensorFormat.STATIC) {
                int size = mInfo.getTensorSize(index);

                if (data.capacity() != size) {
                    throw new IllegalArgumentException("Invalid buffer size, required size is " + size);
                }
            }
        }
    }

    @Override
    public void close() {
        mDataList.clear();
        mInfo = null;
    }

    /**
     * Private constructor to prevent the instantiation.
     */
    private TensorsData(TensorsInfo info) {
        setTensorsInfo(info);
    }
}
