/**
 * @file        unittest_gdbus_util.cc
 * @date        2 May 2023
 * @brief       Unit test for GDBus Utility functions
 * @see         https://github.com/nnstreamer/api
 * @author      Wook Song <wook16.song@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>

#include "dbus-interface.h"
#include "gdbus-util.h"

/**
 * @brief Negative test for the gdbus helper function, gdbus_export_interface
 */
TEST (gdbusInstanceNotInitialized, export_interface_n)
{
  int ret;

  ret = gdbus_export_interface (nullptr, DBUS_PIPELINE_PATH);
  EXPECT_EQ (-ENOSYS, ret);

  ret = gdbus_export_interface (nullptr, DBUS_MODEL_PATH);
  EXPECT_EQ (-ENOSYS, ret);

  ret = gdbus_get_name (DBUS_ML_BUS_NAME);
  EXPECT_EQ (-ENOSYS, ret);
}

/**
 * @brief Negative test for the gdbus helper function, get_system_connection
 */
TEST (gdbusInstanceNotInitialized, get_system_connection_n)
{
  gboolean is_session = true;
  int ret;

  ret = gdbus_get_system_connection (is_session);
  EXPECT_EQ (-ENOSYS, ret);

  ret = gdbus_get_system_connection (!is_session);
  EXPECT_EQ (-ENOSYS, ret);
}

/**
 * @brief Main gtest
 */
int
main (int argc, char **argv)
{
  int result = -1;

  try {
    testing::InitGoogleTest (&argc, argv);
  } catch (...) {
    g_warning ("catch 'testing::internal::<unnamed>::ClassUniqueToAlwaysTrue'");
  }

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  return result;
}
