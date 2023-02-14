/**
 * @file        unittest_dbus_model.cc
 * @date        29 Jul 2022
 * @brief       Unit test for DBus Model interface
 * @see         https://github.com/nnstreamer/api
 * @author      Sangjung Woo <sangjung.woo@samsung.com>
 * @bug         No known bugs
 */

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <errno.h>

#include "model-dbus.h"
#include "../../daemon/includes/dbus-interface.h"

/**
 * @brief Test base class of DBus Model interface
 */
class DbusModelTest : public::testing::Test
{
protected:
  GTestDBus *dbus;
  GDBusProxy *server_proxy;
  MachinelearningServiceModel *client_proxy;

public:
  /**
   * @brief Construct a new DbusModelTest object
   */
  DbusModelTest()
    : dbus(nullptr), server_proxy(nullptr), client_proxy(nullptr) { }

  /**
   * @brief Setup method for each test case.
   */
  void SetUp() override
  {
    g_autofree gchar *services_dir = g_build_filename (g_get_current_dir (), "tests/services", NULL);
    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    g_test_dbus_add_service_dir (dbus, services_dir);
    g_test_dbus_up (dbus);

    GError *error = NULL;
    server_proxy = g_dbus_proxy_new_for_bus_sync (
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      DBUS_ML_BUS_NAME,
      DBUS_MODEL_PATH,
      DBUS_MODEL_INTERFACE,
      NULL,
      &error);

    if (!server_proxy || error) {
      if (error) {
        g_critical ("Error Message : %s", error->message);
        g_clear_error (&error);
      }
    }

    client_proxy = machinelearning_service_model_proxy_new_for_bus_sync (
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      DBUS_ML_BUS_NAME,
      DBUS_MODEL_PATH,
      NULL, &error);
    if (!client_proxy || error) {
      if (error) {
        g_critical ("Error Message : %s", error->message);
        g_clear_error (&error);
      }
    }
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown() override
  {
    if (server_proxy)
      g_object_unref (server_proxy);

    if (client_proxy)
      g_object_unref (client_proxy);

    g_test_dbus_down (dbus);
    g_object_unref (dbus);
  }
};

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
