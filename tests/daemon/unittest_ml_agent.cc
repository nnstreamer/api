/**
 * @file        unittest_ml_agent.cc
 * @date        16 Jul 2022
 * @brief       Unit test for ML Agent itself
 * @see         https://github.com/nnstreamer/api
 * @author      Sangjung Woo <sangjung.woo@samsung.com>
 * @bug         No known bugs
 */

#include <gio/gio.h>
#include <gtest/gtest.h>

#include "test-dbus.h"
#include "dbus-interface.h"
#include "../dbus/test-dbus-interface.h"

/**
 * @brief Test base class for ML Agent Daemon
 */
class MLAgentTest : public::testing::Test
{
protected:
  GTestDBus *dbus;
  GDBusProxy *proxy;

public:
  /**
   * @brief Setup method for each test case.
   */
  void SetUp() override
  {
    gchar *services_dir = g_build_filename (g_get_current_dir (), "tests/services", NULL);
    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    g_test_dbus_add_service_dir (dbus, services_dir);
    g_test_dbus_up (dbus);

    GError *error = NULL;
    proxy = g_dbus_proxy_new_for_bus_sync (
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      DBUS_ML_BUS_NAME,
      DBUS_TEST_PATH,
      DBUS_TEST_INTERFACE,
      NULL,
      &error);

    if (!proxy || error) {
      if (error) {
        g_critical ("Error Message : %s", error->message);
        g_clear_error (&error);
      }
    }

    g_free (services_dir);
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown() override
  {
    if (proxy)
      g_object_unref (proxy);

    g_test_dbus_down (dbus);
    g_object_unref (dbus);
  }
};

/**
 * @brief Call the 'get_state' DBus method and check the result.
 */
TEST_F (MLAgentTest, call_method)
{
  MachinelearningServiceTest *proxy = NULL;
  GError *error = NULL;
  int status = 0;
  int result = 0;

  /* Test : Connect to the DBus Interface */
  proxy = machinelearning_service_test_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_NONE,
    DBUS_ML_BUS_NAME,
    DBUS_TEST_PATH,
    NULL, &error);
  if (error != NULL) {
    g_error_free (error);
    FAIL();
  }

  /* Test: Call the DBus method */
  machinelearning_service_test_call_get_state_sync (proxy, &status, &result, NULL, &error);
  if (error != NULL) {
    g_critical ("Error : %s", error->message);
    g_error_free (error);
    FAIL();
  }

  /* Check the return value */
  EXPECT_EQ (result, 0);
  EXPECT_EQ (status, 1);

  g_object_unref (proxy);
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
