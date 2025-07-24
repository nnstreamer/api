package org.nnsuite.nnstreamer;

import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

import java.util.Set;

/**
 * Testcases for MLInformation.
 */
@RunWith(AndroidJUnit4.class)
public class APITestMLInformation {
    private MLInformation mInfo;

    @Before
    public void setUp() {
        APITestCommon.initNNStreamer();
        mInfo = new MLInformation();
    }

    @After
    public void tearDown() {
        mInfo.close();
    }

    @Test
    public void testSetInformation() {
        try {
            mInfo.set("test-key1", "test-value1");
            assertEquals(1, mInfo.size());

            mInfo.set("test-key2", "test-value2");
            assertEquals(2, mInfo.size());

            mInfo.set("test-key2", "test-value2-update");
            assertEquals(2, mInfo.size());

            mInfo.set("test-key3", "test-value3");
            assertEquals(3, mInfo.size());
        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testGetInformation() {
        try {
            testSetInformation();

            assertEquals("test-value1", mInfo.get("test-key1"));
            assertEquals("test-value2-update", mInfo.get("test-key2"));
            assertEquals("test-value3", mInfo.get("test-key3"));
        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testKeys() {
        try {
            testSetInformation();

            Set<String> keys = mInfo.keys();

            assertEquals(3, keys.size());
            assertTrue(keys.contains("test-key1"));
            assertTrue(keys.contains("test-key2"));
            assertTrue(keys.contains("test-key3"));
        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testSetNullKey_n() {
        try {
            mInfo.set(null, "test-value");
            fail();
        } catch (Exception e) {
            /* expected */
        }

        assertEquals(0, mInfo.size());
    }

    @Test
    public void testSetEmptyKey_n() {
        try {
            mInfo.set("", "test-value");
            fail();
        } catch (Exception e) {
            /* expected */
        }

        assertEquals(0, mInfo.size());
    }

    @Test
    public void testSetNullValue_n() {
        try {
            mInfo.set("test-key", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }

        assertEquals(0, mInfo.size());
    }

    @Test
    public void testSetEmptyValue_n() {
        try {
            mInfo.set("test-key", "");
            fail();
        } catch (Exception e) {
            /* expected */
        }

        assertEquals(0, mInfo.size());
    }

    @Test
    public void testGetNullKey_n() {
        try {
            mInfo.get(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetEmptyKey_n() {
        try {
            mInfo.get("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInvalidKey_n() {
        try {
            mInfo.get("unknown-key");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }
}
