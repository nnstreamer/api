/**
 * @file        unittest_service_db.cc
 * @date        21 Mar 2023
 * @brief       Unit test for Service DB used by ML Agent
 * @see         https://github.com/nnstreamer/api
 * @author      Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <gio/gio.h>

#include "service-db.hh"

/**
 * @brief Negative test for set_pipeline. Empty name or description.
 */
TEST (serviceDB, set_pipeline_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();
  try {
    db.set_pipeline ("", "videotestsrc ! fakesink");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.set_pipeline ("test_key", "");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for get_pipeline. Empty name.
 */
TEST (serviceDB, get_pipeline_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  db.connectDB ();

  try {
    std::string pipeline_description;
    db.get_pipeline ("", pipeline_description);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for delete_pipeline. Empty name.
 */
TEST (serviceDB, delete_pipeline_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  db.connectDB ();

  try {
    db.delete_pipeline ("");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for set_model. Invalid param case (empty name, model or version).
 */
TEST (serviceDB, set_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  guint version;

  db.connectDB ();
  try {
    db.set_model ("", "model", true, "description", "", &version);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.set_model ("test", "", true, "description", "", &version);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.set_model ("test", "model", true, "", "", NULL);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Check model update.
 */
TEST (serviceDB, update_model_scenario)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  /* No exception to add, get, and delete model with name 'test'. */
  try {
    std::string model_info;
    gchar *pos;
    guint version, version_active;

    db.set_model ("test", "test_model1", true, "model1_description", "", &version_active);
    db.set_model ("test", "test_model2", false, "model2_description", "", &version);

    /* Check model info contains added string. */
    db.get_model ("test", model_info, 0);
    pos = g_strstr_len (model_info.c_str (), -1, "test_model1");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (model_info.c_str (), -1, "test_model2");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (model_info.c_str (), -1, "model1_description");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (model_info.c_str (), -1, "model2_description");
    EXPECT_TRUE (pos != NULL);

    db.get_model ("test", model_info, version);
    pos = g_strstr_len (model_info.c_str (), -1, "test_model2");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (model_info.c_str (), -1, "model2_description");
    EXPECT_TRUE (pos != NULL);

    db.get_model ("test", model_info, -1);
    pos = g_strstr_len (model_info.c_str (), -1, "test_model1");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (model_info.c_str (), -1, "model1_description");
    EXPECT_TRUE (pos != NULL);

    db.activate_model ("test", version);
    db.update_model_description ("test", version, "updated_desc_model2");
    db.get_model ("test", model_info, -1);
    pos = g_strstr_len (model_info.c_str (), -1, "test_model2");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (model_info.c_str (), -1, "updated_desc_model2");
    EXPECT_TRUE (pos != NULL);

    db.delete_model ("test", 0);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 0);

  db.disconnectDB ();
}

/**
 * @brief Negative test for get_model. Empty name, invalid version.
 */
TEST (serviceDB, get_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  db.connectDB ();

  try {
    std::string model_description;
    db.get_model ("", model_description, 0);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    std::string model_description;
    db.get_model ("test", model_description, -54321);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for update_model_description. Invalid param case (empty name or description, invalid version).
 */
TEST (serviceDB, update_model_description_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  db.connectDB ();

  try {
    db.update_model_description ("", 1, "description");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.update_model_description ("test", 1, "");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.update_model_description ("test", 0, "description");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for activate_model. Invalid param case (empty name or invalid version).
 */
TEST (serviceDB, activate_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  db.connectDB ();

  try {
    db.activate_model ("", 1);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.activate_model ("test", 0);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for delete_model. Empty name.
 */
TEST (serviceDB, delete_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  db.connectDB ();

  try {
    db.delete_model ("", 0);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
  db.disconnectDB ();
}

/**
 * @brief Negative test for delete_model. Model is not registered.
 */
TEST (serviceDB, delete_model_unregistered_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  guint version;

  db.connectDB ();

  /* Test condition, remove all model with name 'test'. */
  db.set_model ("test", "test_model", true, "", "", &version);
  db.delete_model ("test", 0U);

  try {
    db.delete_model ("test", version);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.disconnectDB ();
}

/**
 * @brief Negative test for delete_model. Model is activated.
 */
TEST (serviceDB, delete_model_activated_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;
  guint version;

  db.connectDB ();

  /* Test condition, add new model as activated. */
  db.set_model ("test", "test_model", true, "", "", &version);

  try {
    db.delete_model ("test", version);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.delete_model ("test", 0U);
  db.disconnectDB ();
}

/**
 * @brief Negative test for set_pipline. DB is not initialized.
 */
TEST (serviceDBNotInitalized, set_pipeline_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.set_pipeline ("test", "videotestsrc ! fakesink");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for get_pipline. DB is not initialized.
 */
TEST (serviceDBNotInitalized, get_pipeline_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    std::string pd;
    db.get_pipeline ("test", pd);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for delete_pipeline. DB is not initialized.
 */
TEST (serviceDBNotInitalized, delete_pipeline_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.delete_pipeline ("test");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for set_model. DB is not initialized.
 */
TEST (serviceDBNotInitalized, set_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    guint version;
    db.set_model ("test", "model", true, "description", "", &version);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for update_model_description. DB is not initialized.
 */
TEST (serviceDBNotInitalized, update_model_description_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.update_model_description ("test", 0, "description");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for activate_model. DB is not initialized.
 */
TEST (serviceDBNotInitalized, activate_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.activate_model ("test", 0);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for get_model. DB is not initialized.
 */
TEST (serviceDBNotInitalized, get_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    std::string model_path;
    db.get_model ("test", model_path, 0);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for delete_model. DB is not initialized.
 */
TEST (serviceDBNotInitalized, delete_model_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.delete_model ("test", 0U);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for set_resource. Invalid param case (empty name or path).
 */
TEST (serviceDB, set_resource_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  try {
    db.set_resource ("", "resource", "description", "");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  gotException = 0;
  try {
    db.set_resource ("test", "", "description", "");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.disconnectDB ();
}

/**
 * @brief Check resources.
 */
TEST (serviceDB, update_resource_scenario)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  /* No exception to add, get, and delete resources with name 'test'. */
  try {
    std::string res_info;
    gchar *pos;

    db.set_resource ("test", "test_resource1", "res1_description", "");
    db.set_resource ("test", "test_resource2", "res2_description", "");

    /* Check res info contains added string. */
    db.get_resource ("test", res_info);
    pos = g_strstr_len (res_info.c_str (), -1, "test_resource1");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (res_info.c_str (), -1, "test_resource2");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (res_info.c_str (), -1, "res1_description");
    EXPECT_TRUE (pos != NULL);
    pos = g_strstr_len (res_info.c_str (), -1, "res2_description");
    EXPECT_TRUE (pos != NULL);

    db.set_resource ("test", "test_resource2", "updated_desc_res2", "");
    db.get_resource ("test", res_info);
    pos = g_strstr_len (res_info.c_str (), -1, "updated_desc_res2");
    EXPECT_TRUE (pos != NULL);

    db.delete_resource ("test");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 0);

  db.disconnectDB ();
}

/**
 * @brief Negative test for get_resource. Empty name.
 */
TEST (serviceDB, get_resource_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  try {
    std::string res_description;
    db.get_resource ("", res_description);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.disconnectDB ();
}

/**
 * @brief Negative test for get_resource. Empty name or unregistered name.
 */
TEST (serviceDB, get_resource_unregistered_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  /* Test condition, remove all resource with name 'test'. */
  db.set_resource ("test", "test_resource", "", "");
  db.delete_resource ("test");

  gotException = 0;
  try {
    std::string res_description;
    db.get_resource ("test", res_description);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.disconnectDB ();
}

/**
 * @brief Negative test for delete_resource. Empty name.
 */
TEST (serviceDB, delete_resource_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  try {
    db.delete_resource ("");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.disconnectDB ();
}

/**
 * @brief Negative test for delete_resource. Resource is not registered.
 */
TEST (serviceDB, delete_resource_unregistered_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  db.connectDB ();

  /* Test condition, remove all resource with name 'test'. */
  db.set_resource ("test", "test_resource", "", "");
  db.delete_resource ("test");

  try {
    db.delete_resource ("test");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);

  db.disconnectDB ();
}

/**
 * @brief Negative test for set_resource. DB is not initialized.
 */
TEST (serviceDBNotInitalized, set_resource_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.set_resource ("test", "resource", "description", "");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for get_resource. DB is not initialized.
 */
TEST (serviceDBNotInitalized, get_resource_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    std::string res_description;
    db.get_resource ("test", res_description);
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
}

/**
 * @brief Negative test for delete_resource. DB is not initialized.
 */
TEST (serviceDBNotInitalized, delete_resource_n)
{
  MLServiceDB &db = MLServiceDB::getInstance ();
  int gotException = 0;

  try {
    db.delete_resource ("test");
  } catch (const std::exception &e) {
    g_critical ("Got Exception: %s", e.what ());
    gotException = 1;
  }
  EXPECT_EQ (gotException, 1);
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
