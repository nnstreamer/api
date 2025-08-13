/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API
 * Copyright (C) 2025 Samsung Electronics Co., Ltd.
 */

package org.nnsuite.nnstreamer;

import java.util.Hashtable;
import java.util.Set;

/**
 * Provides interfaces to handle machine-learning information, a set of string key-value pairs.
 */
public final class MLInformation implements AutoCloseable {
    private Hashtable<String, String> mInfoTable = new Hashtable<>();

    /**
     * Gets the number of machine-learning information.
     *
     * @return The number of machine-learning information.
     */
    public int size() {
        return mInfoTable.size();
    }

    /**
     * Adds a new information.
     *
     * @param key The key to set the corresponding value.
     * @param value The value of the key.
     *
     * @throws IllegalArgumentException if given param is invalid.
     */
    public void set(String key, String value) {
        if (!isValidString(key)) {
            throw new IllegalArgumentException("Given information key is invalid.");
        }

        if (!isValidString(value)) {
            throw new IllegalArgumentException("Given information value is invalid.");
        }

        mInfoTable.put(key, value);
    }

    /**
     * Gets an information value of key.
     *
     * @param key The key to get the corresponding value.
     *
     * @return The value of the key.
     *
     * @throws IllegalArgumentException if given param is invalid.
     * @throws IllegalStateException if failed to get the information value of the key.
     */
    public String get(String key) {
        if (!isValidString(key)) {
            throw new IllegalArgumentException("Given information key is invalid.");
        }

        String value = mInfoTable.get(key);

        if (value == null) {
            throw new IllegalStateException("Failed to get the information value of the key.");
        }

        return value;
    }

    /**
     * Gets the keys contained in the machine-learning information.
     *
     * @return A set of the keys.
     */
    public Set<String> keys() {
        return mInfoTable.keySet();
    }

    /**
     * Internal method to validate string.
     */
    private static boolean isValidString(String str) {
        return (str != null && !str.trim().isEmpty());
    }

    @Override
    public void close() {
        mInfoTable.clear();
    }
}
