/**
 * @file        unittest_capi_service_db_mock.cc
 * @date        14 Apr 2022
 * @brief       Mocking unit test for ML DB Service C-API
 * @see         https://github.com/nnstreamer/api
 * @author      Sangjung Woo <sangjung.woo@samsung.com>
 * @bug         No known bugs
 */

#include <cstring>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <leveldb/c.h>

#include <ml-api-internal.h>
#include <ml-api-service.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

/**
 * @brief Interface for LevelDB mock class
 */
class ILevelDB {
public:
  /**
   * @brief Destroy the ILevelDB object
   */
  virtual ~ILevelDB() {};

  /**
   * @brief Interface for opening a LevelDB
   */
  virtual leveldb_t* leveldb_open (const leveldb_options_t* options,
      const char* name, char** errptr) = 0;

  /**
   * @brief Interface for writing a tuple in LevelDB
   */
  virtual void leveldb_put (leveldb_t* db, const leveldb_writeoptions_t* options,
      const char* key, size_t keylen, const char* val,
      size_t vallen, char** errptr) = 0;

  /**
   * @brief Interface for reading a tuple in LevelDB
   */
  virtual char* leveldb_get (leveldb_t* db, const leveldb_readoptions_t* options,
      const char* key, size_t keylen, size_t* vallen, char** errptr) = 0;

  /**
   * @brief Interface for deleting a tuple in LevelDB
   */
  virtual void leveldb_delete (leveldb_t* db, const leveldb_writeoptions_t* options,
      const char* key, size_t keylen, char** errptr) = 0;

  /**
   * @brief Interface for closing a LevelDB object
   */
  virtual void leveldb_close (leveldb_t* db) = 0;
};

/**
 * @brief Mock class for testing LevelDB
 */
class LevelDBMock : public ILevelDB {
public:
  MOCK_METHOD (leveldb_t*, leveldb_open,
    (const leveldb_options_t* options, const char* name, char** errptr));
  MOCK_METHOD (void, leveldb_put,
    (leveldb_t* db, const leveldb_writeoptions_t* options, const char* key, size_t keylen,
     const char* val, size_t vallen, char** errptr));
  MOCK_METHOD (char*, leveldb_get, (leveldb_t* db, const leveldb_readoptions_t* options,
      const char* key, size_t keylen, size_t* vallen, char** errptr));
  MOCK_METHOD (void, leveldb_delete, (leveldb_t* db, const leveldb_writeoptions_t* options,
      const char* key, size_t keylen, char** errptr));
  MOCK_METHOD (void, leveldb_close, (leveldb_t* db));
};
LevelDBMock *mockInstance = nullptr;

/**
 * @brief Mocking function for leveldb_open() of the LevelDB
 * @param[in] options LevelDB options
 * @param[in] name database path
 * @param[out] errptr error message if error occurs
 * @return leveldb_t* database handle
 */
leveldb_t* leveldb_open (const leveldb_options_t* options,
    const char* name, char** errptr)
{
  return mockInstance->leveldb_open (options, name, errptr);
}
/**
 * @brief Mocking function for leveldb_put() of the LevelDB
 * @param[in] db database handle.
 * @param[in] options write option.
 * @param[in] key key string.
 * @param[in] keylen length of key string.
 * @param[in] val value string.
 * @param[in] vallen length of value string.
 * @param[out] errptr error message if error occurs.
 */
void leveldb_put (leveldb_t* db, const leveldb_writeoptions_t* options,
    const char* key, size_t keylen, const char* val, size_t vallen, char** errptr)
{
  return mockInstance->leveldb_put (db, options, key, keylen, val, vallen, errptr);
}

/**
 * @brief Mocking function for leveldb_get() of the LevelDB
 * @param[in] db database handle.
 * @param[in] options read option.
 * @param[in] key key string.
 * @param[in] keylen length of key string.
 * @param[out] vallen length of value string.
 * @param[out] errptr error message if error occurs.
 * @return char* value string if found. Otherwise NULL.
 */
char* leveldb_get (leveldb_t* db, const leveldb_readoptions_t* options,
    const char* key, size_t keylen, size_t* vallen, char** errptr)
{
  return mockInstance->leveldb_get (db, options, key, keylen, vallen, errptr);
}

/**
 * @brief Mocking function for leveldb_delete() of the LevelDB
 * @param[in] db database handle.
 * @param[in] options write option.
 * @param[in] key key string.
 * @param[in] keylen length of key string.
 * @param[out] errptr error message if error occurs.
 */
void leveldb_delete (leveldb_t* db, const leveldb_writeoptions_t* options,
    const char* key, size_t keylen, char** errptr)
{
  mockInstance->leveldb_delete (db, options, key, keylen, errptr);
}

/**
 * @brief Mocking function for leveldb_close() of the LevelDB
 * @param db database handle.
 */
void leveldb_close (leveldb_t* db)
{
  mockInstance->leveldb_close (db);
}

/**
 * @brief Test base class for DBMock of ML Service API
 */
class MLServiceAPIDBMockTest : public ::testing::Test
{
protected:
  const gchar *key;
  const gchar *invalid_key;
  gchar *pipeline;
  gsize len;

  /**
   * @brief Setup metohd for each test case.
   */
  void SetUp () override
  {
    key = "ServiceName";
    invalid_key = "InvalidKey";
    pipeline = g_strdup_printf ("appsrc name=appsrc ! "
      "other/tensors,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_sink name=tensor_sink");
    len = strlen (pipeline);
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown () override
  {
    g_free (pipeline);
  }
};

/**
 * @brief Check the normal process of setting the pipeline description with a given name.
 */
TEST_F (MLServiceAPIDBMockTest, set_pipeline_description_0_p)
{
  int ret = 0;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .Times(1).WillOnce(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(SetArgPointee<6>(nullptr));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(1).WillRepeatedly(Return());

  /* Test */
  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  delete mockInstance;
}

/**
 * @brief Check the error handling when leveldb_open() raises an IO exception.
 */
TEST_F (MLServiceAPIDBMockTest, leveldb_open_IO_ERROR_n)
{
  int ret = 0;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<2>(strdup("leveldb_open() Error: not found DB files.")),
        Return(static_cast<leveldb_t*>(nullptr))));

  /* Test */
  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_IO_ERROR, ret);

  delete mockInstance;
}

/**
 * @brief Check the error handling when leveldb_put() raises an IO exception.
 */
TEST_F (MLServiceAPIDBMockTest, leveldb_put_IO_ERROR_n)
{
  int ret = 0;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .Times(1).WillOnce(DoAll(
        Return((leveldb_t*)0x1234)));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(SetArgPointee<6>(strdup("leveldb_put() Error: DB is locked.")));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(1).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_IO_ERROR, ret);

  delete mockInstance;
}

/**
 * @brief Check the normal process of getting the pipeline description with a given name.
 */
TEST_F (MLServiceAPIDBMockTest, get_pipeline_description_0_p)
{
  int ret = 0;
  gchar *ret_pipeline = NULL;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .WillRepeatedly(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(SetArgPointee<6>(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_get(_, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<4>(len),
        SetArgPointee<5>(nullptr),
        Return(strdup(pipeline))));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(2).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
  EXPECT_STREQ (pipeline, ret_pipeline);

  g_free (ret_pipeline);
  delete mockInstance;
}

/**
 * @brief Check the error handling when leveldb_get() does not find the given key.
 */
TEST_F (MLServiceAPIDBMockTest, leveldb_get_INVALID_PARAMETER_n)
{
  int ret = 0;
  gchar *ret_pipeline = NULL;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .WillRepeatedly(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(SetArgPointee<6>(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_get(_, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<4>(0),
        SetArgPointee<5>(nullptr),
        Return(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(2).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_get_pipeline (invalid_key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  delete mockInstance;
}

/**
 * @brief Check the error handling when leveldb_get() raises an IO exception.
 */
TEST_F (MLServiceAPIDBMockTest, leveldb_get_IO_ERROR_n)
{
  int ret = 0;
  gchar *ret_pipeline = NULL;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .WillRepeatedly(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(SetArgPointee<6>(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_get(_, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<4>(0),
        SetArgPointee<5>(strdup("Failed to call leveldb_get(): IO Error")),
        Return(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(2).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_IO_ERROR, ret);

  delete mockInstance;
}


/**
 * @brief Check the normal process of deleting the given name.
 */
TEST_F (MLServiceAPIDBMockTest, delete_pipeline_description_p)
{
  int ret = 0;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .WillRepeatedly(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(SetArgPointee<6>(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_get(_, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<4>(len),
        SetArgPointee<5>(nullptr),
        Return(strdup(pipeline))));
  EXPECT_CALL (*mockInstance, leveldb_delete(_, _, _, _, _))
      .Times(1).WillOnce(SetArgPointee<4>(nullptr));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(2).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_delete_pipeline (key);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  delete mockInstance;
}

/**
 * @brief Check the error handling when calling ml_service_delete_pipeline() because the key is not found.
 */
TEST_F (MLServiceAPIDBMockTest, leveldb_delete_INVALID_PARAMETER_n)
{
  int ret = 0;
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .WillRepeatedly(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(SetArgPointee<6>(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_get(_, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<5>(strdup("Failed to find the key")),
        Return(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(2).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_delete_pipeline (invalid_key);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  delete mockInstance;
}

/**
 * @brief Check the error handling when calling ml_service_delete_pipeline() because of an IO exception.
 */
TEST_F (MLServiceAPIDBMockTest, leveldb_delete_IO_ERROR_n)
{
  int ret = 0;
  gsize len = strlen(pipeline);
  mockInstance = new LevelDBMock();

  EXPECT_CALL (*mockInstance, leveldb_open(_, _, _))
      .WillRepeatedly(Return((leveldb_t*)0x1234));
  EXPECT_CALL (*mockInstance, leveldb_put(_, _, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(SetArgPointee<6>(nullptr)));
  EXPECT_CALL (*mockInstance, leveldb_get(_, _, _, _, _, _))
      .Times(1).WillOnce(DoAll(
        SetArgPointee<4>(len),
        SetArgPointee<5>(nullptr),
        Return(strdup(pipeline))));
  EXPECT_CALL (*mockInstance, leveldb_delete(_, _, _, _, _))
      .Times(1).WillOnce(SetArgPointee<4>(strdup("leveldb_delete(): Dababase is locked.")));
  EXPECT_CALL (*mockInstance, leveldb_close(_))
      .Times(2).WillRepeatedly(Return());

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_delete_pipeline (key);
  EXPECT_EQ (ML_ERROR_IO_ERROR, ret);

  delete mockInstance;
}

/**
 * @brief Main gtest function
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

  /* ignore tizen feature status while running the testcases */
  set_feature_state (ML_FEATURE, SUPPORTED);
  set_feature_state (ML_FEATURE_SERVICE, SUPPORTED);

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  set_feature_state (ML_FEATURE, NOT_CHECKED_YET);
  set_feature_state (ML_FEATURE_SERVICE, NOT_CHECKED_YET);

  return result;
}
