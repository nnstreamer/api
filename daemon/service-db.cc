/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file service-db.cc
 * @date 29 Jul 2022
 * @brief Database implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>

#include "service-db.hh"

#define ML_DATABASE_PATH      DB_PATH"/.ml-service-leveldb"
#define DB_KEY_PREFIX         MESON_KEY_PREFIX

/**
 * @brief Get an instance of MLServiceDB, which is created only once at runtime.
 * @return MLServiceDB& MLServiceDB instance
 */
MLServiceDB & MLServiceDB::getInstance (void)
{
  static MLServiceDB instance (ML_DATABASE_PATH);

  return instance;
}

/**
 * @brief Construct a new MLServiceDB object.
 * @param path database path
 */
MLServiceDB::MLServiceDB (std::string path)
: path (path), db_obj (nullptr), db_roptions (nullptr), db_woptions (nullptr)
{
  db_roptions = leveldb_readoptions_create ();
  db_woptions = leveldb_writeoptions_create ();
  leveldb_writeoptions_set_sync (db_woptions, 1);
}

/**
 * @brief Destroy the MLServiceDB object.
 */
MLServiceDB::~MLServiceDB ()
{
  disconnectDB ();
  leveldb_readoptions_destroy (db_roptions);
  leveldb_writeoptions_destroy (db_woptions);
}

/**
 * @brief Connect to ML Service DB and initialize the private variables.
 */
void
MLServiceDB::connectDB ()
{
  char *err = nullptr;
  leveldb_options_t *db_options;

  if (db_obj)
    return;

  db_options = leveldb_options_create ();
  leveldb_options_set_create_if_missing (db_options, 1);

  db_obj = leveldb_open (db_options, path.c_str (), &err);
  leveldb_options_destroy (db_options);
  if (err != nullptr) {
    g_warning ("Error! Failed to open database located at '%s': leveldb_open() has returned an error: %s",
        path.c_str (), err);
    leveldb_free (err);
    disconnectDB ();
    throw std::runtime_error ("Failed to connectDB()!");
  }
}

/**
 * @brief Disconnect the DB.
 * @note LevelDB does not support multi-process and it might cause
 * the IO exception when multiple clients write the key simultaneously.
 */
void
MLServiceDB::disconnectDB ()
{
  if (db_obj) {
    leveldb_close (db_obj);
    db_obj = nullptr;
  }
}

/**
 * @brief Set the value with the given name.
 * @note If the name already exists, the pipeline description is overwritten.
 * @param[in] name Unique name to set the associated pipeline description.
 * @param[in] value The pipeline description to be stored.
 */
void
MLServiceDB::put (const std::string name, const std::string value)
{
  char *err = nullptr;

  if (name.empty() || value.empty())
    throw std::invalid_argument ("Invalid name or value parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;
  std::size_t hash[2] = { std::hash<std::string>{}(key_with_prefix), 0U };

  leveldb_put (db_obj, db_woptions, (char *) hash, sizeof (std::size_t) * 2,
      value.c_str (), value.size (), &err);
  if (err != nullptr) {
    g_warning
        ("Failed to call leveldb_put () for the name, '%s' of the pipeline description (size: %zu bytes / description: '%.40s')",
        name.c_str (), value.size (), value.c_str ());
    g_warning ("leveldb_put () has returned an error: %s", err);
    leveldb_free (err);
    throw std::runtime_error ("Failed to put()!");
  }
}

/**
 * @brief Get the value with the given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] value The pipeline corresponding with the given name.
 */
void
MLServiceDB::get (const std::string name, std::string & out_value)
{
  char *err = nullptr;
  char *value = nullptr;
  gsize read_len;

  if (name.empty())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;
  std::size_t hash[2] = { std::hash<std::string>{}(key_with_prefix), 0U };

  value = leveldb_get (db_obj, db_roptions, (char *) hash, sizeof (std::size_t) * 2,
      &read_len, &err);
  if (err != nullptr) {
    g_warning
        ("Failed to call leveldb_get() for the name %s. Error message is %s.",
        name.c_str (), err);
    leveldb_free (err);
    if (value)
      leveldb_free (value);
    throw std::runtime_error ("Failed to get()!");
  }

  if (!value) {
    g_warning
        ("Failed to find the key %s. The key should be set before reading it.",
        name.c_str ());
    throw std::invalid_argument ("Failed to find the key.");
  }

  out_value = std::string (value, read_len);
  leveldb_free (value);
  return;
}

/**
 * @brief Delete the value with a given name.
 * @param[in] name The unique name to delete
 */
void
MLServiceDB::del (const std::string name)
{
  char *err = nullptr;
  char *value = nullptr;
  gsize read_len;

  if (name.empty())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;
  std::size_t hash[2] = { std::hash<std::string>{}(key_with_prefix), 0U };

  /* Check whether the key exists or not. */
  value = leveldb_get (db_obj, db_roptions, (char *) hash, sizeof (std::size_t) * 2,
      &read_len, &err);
  if (!value || err) {
    g_warning
        ("Failed to find the key %s. The key should be set before reading it.",
        name.c_str ());
    if (err)
      leveldb_free (err);
    throw std::invalid_argument ("Failed to find the key.");
  }
  leveldb_free (value);

  leveldb_delete (db_obj, db_woptions, (char *) hash, sizeof (std::size_t) * 2, &err);
  if (err != nullptr) {
    g_warning ("Failed to delete the key %s. Error message is %s.",
        name.c_str (), err);
    leveldb_free (err);
    throw std::runtime_error ("Failed to del()!");
  }
}
