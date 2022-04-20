/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-db.c
 * @date 07 Mar 2022
 * @brief Database implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <iostream>
#include <cstring>

#include "ml-api-inference-internal.h"
#include "ml-service-db.hh"
#include "ml-api-service.h"

#define ML_DATABASE_PATH      "/tmp/.ml-service-leveldb"

/**
 * @brief Class for implementation of IMLServiceDB
 */
class MLServiceLevelDB:public IMLServiceDB
{
public:
  MLServiceLevelDB (const MLServiceLevelDB &) = delete;
  MLServiceLevelDB (MLServiceLevelDB &&) = delete;
  MLServiceLevelDB & operator= (const MLServiceLevelDB &) = delete;
  MLServiceLevelDB & operator= (MLServiceLevelDB &&) = delete;

  virtual void connectDB () override;
  virtual void setPipelineDescription (const std::string name,
      const std::string pipeline_description) override;
  virtual void getPipelineDescription (std::string name,
      std::string & pipeline_description) override;
  virtual void delPipelineDescription (std::string name);

  static IMLServiceDB & getInstance (void);

private:
  MLServiceLevelDB (std::string path);
  virtual ~MLServiceLevelDB ();

  std::string path;
  leveldb_t *db_obj;
  leveldb_readoptions_t *db_roptions;
  leveldb_writeoptions_t *db_woptions;
};

/**
 * @brief Get an instance of IMLServiceDB, which is created only once at runtime.
 * @return IMLServiceDB& IMLServiceDB instance
 */
IMLServiceDB & MLServiceLevelDB::getInstance (void)
{
  static MLServiceLevelDB
  instance (ML_DATABASE_PATH);
  instance.connectDB ();

  return instance;
}

/**
 * @brief Construct a new MLServiceLevelDB object
 * @param path database path
 */
MLServiceLevelDB::MLServiceLevelDB (std::string path)
:  path (path), db_obj (nullptr), db_roptions (nullptr), db_woptions (nullptr)
{
}

/**
 * @brief Destroy the MLServiceLevelDB object
 */
MLServiceLevelDB::~MLServiceLevelDB ()
{
  leveldb_close (db_obj);
  leveldb_readoptions_destroy (db_roptions);
  leveldb_writeoptions_destroy (db_woptions);
}

/**
 * @brief Connect the level DB and initialize the private variables.
 */
void
MLServiceLevelDB::connectDB ()
{
  static bool isConnected = false;
  char *err = nullptr;
  leveldb_options_t *db_options;

  if (isConnected)
    return;

  db_options = leveldb_options_create ();
  leveldb_options_set_create_if_missing (db_options, 1);

  db_obj = leveldb_open (db_options, path.c_str (), &err);
  if (err != nullptr) {
    _ml_loge
        ("Error! Failed to open database located at '%s': leveldb_open () has returned an error: %s",
        path.c_str (), err);
    leveldb_free (err);
    throw std::runtime_error ("Failed to connectDB()!");
  }

  db_roptions = leveldb_readoptions_create ();
  db_woptions = leveldb_writeoptions_create ();
  leveldb_writeoptions_set_sync (db_woptions, 1);

  isConnected = true;
  return;
}

/**
 * @brief Set the pipeline description with the given name.
 * @note If the name already exists, the pipeline description is overwritten.
 * @param[in] name Unique name to retrieve the associated pipeline description.
 * @param[in] pipeline_description The pipeline description to be stored.
 */
void
MLServiceLevelDB::setPipelineDescription (const std::string name,
    const std::string pipeline_description)
{
  char *err = NULL;

  leveldb_put (db_obj, db_woptions, name.c_str (), name.size (),
      pipeline_description.c_str (), pipeline_description.size (), &err);
  if (err != nullptr) {
    _ml_loge
        ("Failed to call leveldb_put () for the name, '%s' of the pipeline description (size: %zu bytes / decription: '%.40s')",
        name.c_str (), pipeline_description.size (),
        pipeline_description.c_str ());
    _ml_loge ("leveldb_put () has returned an error: %s", err);
    leveldb_free (err);
    throw std::runtime_error ("Failed to setPipelineDescription()!");
  }
}

/**
 * @brief Get the pipeline description with the given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] pipeline_description The pipeline corresponding with the given name.
 */
void
MLServiceLevelDB::getPipelineDescription (const std::string name,
    std::string & pipeline_description)
{
  char *err = NULL;
  char *value = NULL;
  gsize read_len;

  value = leveldb_get (db_obj, db_roptions, name.c_str (), name.size (),
      &read_len, &err);
  if (!value) {
    _ml_loge
        ("Failed to find the key %s. The key should be set before reading it",
        name.c_str ());
    throw std::invalid_argument ("Fail to find the key");
  }

  if (err != nullptr) {
    _ml_loge
        ("Failed to call leveldb_get() for the name %s. Error message is %s.",
        name.c_str (), err);
    leveldb_free (err);
    throw std::runtime_error ("Failed to getPipelineDescription()!");
  }

  pipeline_description = std::string (value, read_len);
  return;
}

/**
 * @brief Delete the pipeline description with a given name.
 * @param[in] name The unique name to delete
 */
void
MLServiceLevelDB::delPipelineDescription (const std::string name)
{
  char *err = NULL;
  char *value = NULL;
  gsize read_len;

  /* Check whether the key exists or not. */
  value = leveldb_get (db_obj, db_roptions, name.c_str (), name.size (),
      &read_len, &err);
  if (!value) {
    _ml_loge
        ("Failed to find the key %s. The key should be set before reading it",
        name.c_str ());
    throw std::invalid_argument ("Fail to find the key");
  }
  leveldb_free (value);

  leveldb_delete (db_obj, db_woptions, name.c_str (), name.size (), &err);
  if (err != nullptr) {
    _ml_loge ("Failed to delete the key %s. Error message is %s", name.c_str (),
        err);
    leveldb_free (err);
    throw std::runtime_error ("Failed to delPipelineDescription()!");
  }
}

/**
 * @brief Set the pipeline description with a given name.
 */
extern "C" int
ml_service_set_pipeline (const char *name, const char *pipeline_desc)
{
  check_feature_state (ML_FEATURE_SERVICE);

  if (!name || !pipeline_desc) {
    _ml_loge ("Error! name and pipeline_desc should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  try {
    IMLServiceDB & db = MLServiceLevelDB::getInstance ();
    db.setPipelineDescription (name, std::string (pipeline_desc));
  }
  catch (const std::runtime_error & e)
  {
    return ML_ERROR_IO_ERROR;
  }
  catch (const std::exception & e)
  {
    return ML_ERROR_IO_ERROR;
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Get the pipeline description with a given name.
 */
extern "C" int
ml_service_get_pipeline (const char *name, char **pipeline_desc)
{
  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_loge ("Error! name should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!pipeline_desc) {
    _ml_loge ("Error! pipeline_desc should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  IMLServiceDB & db = MLServiceLevelDB::getInstance ();
  std::string ret_pipeline;
  try {
    db.getPipelineDescription (name, ret_pipeline);
  }
  catch (const std::invalid_argument & e)
  {
    return ML_ERROR_INVALID_PARAMETER;
  }
  catch (const std::runtime_error & e)
  {
    return ML_ERROR_IO_ERROR;
  }
  catch (const std::exception & e)
  {
    return ML_ERROR_IO_ERROR;
  }

  *pipeline_desc = strdup (ret_pipeline.c_str ());
  return ML_ERROR_NONE;
}

/**
 * @brief Delete the pipeline description with a given name.
 */
extern "C" int
ml_service_delete_pipeline (const char *name)
{
  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_loge ("Error! name should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  IMLServiceDB & db = MLServiceLevelDB::getInstance ();
  try {
    db.delPipelineDescription (name);
  }
  catch (const std::invalid_argument & e)
  {
    return ML_ERROR_INVALID_PARAMETER;
  }
  catch (const std::runtime_error & e)
  {
    return ML_ERROR_IO_ERROR;
  }
  catch (const std::exception & e)
  {
    return ML_ERROR_IO_ERROR;
  }

  return ML_ERROR_NONE;
}
