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

#define sqlite3_clear_errmsg(m) do { if (m) { sqlite3_free (m); (m) = nullptr; } } while (0)

#define ML_DATABASE_PATH      DB_PATH"/.ml-service.db"
#define DB_KEY_PREFIX         MESON_KEY_PREFIX

/**
 * @brief The version of pipeline table schema. It should be a positive integer.
 */
#define TBL_VER_PIPELINE_DESCRIPTION (1)

/**
 * @brief The version of model table schema. It should be a positive integer.
 */
#define TBL_VER_MODEL_INFO (1)

typedef enum
{
  TBL_DB_INFO = 0,
  TBL_PIPELINE_DESCRIPTION = 1,
  TBL_MODEL_INFO = 2,

  TBL_MAX
} mlsvc_table_e;

const char *g_mlsvc_table_schema_v1[] =
{
  [TBL_DB_INFO] = "tblMLDBInfo (name TEXT PRIMARY KEY NOT NULL, version INTEGER DEFAULT 1)",
  [TBL_PIPELINE_DESCRIPTION] = "tblPipeline (key TEXT PRIMARY KEY NOT NULL, description TEXT, CHECK (length(description) > 0))",
  [TBL_MODEL_INFO] = "tblModel (key TEXT NOT NULL, version INTEGER DEFAULT 1, stable TEXT DEFAULT 'N', valid TEXT DEFAULT 'N', "
      "path TEXT, PRIMARY KEY (key, version), CHECK (length(path) > 0), CHECK (stable IN ('T', 'F')), CHECK (valid IN ('T', 'F')))",
  NULL
};

const char **g_mlsvc_table_schema = g_mlsvc_table_schema_v1;

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
: _path (path), _initialized (false), _db (nullptr)
{
}

/**
 * @brief Destroy the MLServiceDB object.
 */
MLServiceDB::~MLServiceDB ()
{
  disconnectDB ();
  _initialized = false;
}

/**
 * @brief Create table and handle database version.
 */
void
MLServiceDB::initDB ()
{
  int rc;
  int tbl_ver;
  char *sql;
  char *errmsg = nullptr;
  sqlite3_stmt *res;

  if (_initialized)
    return;

  /**
   * @todo data migration
   * handle database version and update each table
   * 1. get all records from table
   * 2. drop old table
   * 3. create new table and insert records
   */
  rc = sqlite3_exec (_db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to begin transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  /* Create tables. */
  sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s;", g_mlsvc_table_schema[TBL_DB_INFO]);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to create table for database info: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s;", g_mlsvc_table_schema[TBL_PIPELINE_DESCRIPTION]);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to create table for pipeline description: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s;", g_mlsvc_table_schema[TBL_MODEL_INFO]);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to create table for model info: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  /* Check pipeline table. */
  rc = sqlite3_prepare_v2 (_db, "SELECT version FROM tblMLDBInfo WHERE name = 'tblPipeline';", -1, &res, nullptr);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to get the version of pipeline table: %s (%d)", sqlite3_errmsg (_db), rc);
    return;
  }

  tbl_ver = (sqlite3_step (res) == SQLITE_ROW) ? sqlite3_column_int (res, 0) : TBL_VER_PIPELINE_DESCRIPTION;
  sqlite3_finalize (res);

  if (tbl_ver != TBL_VER_PIPELINE_DESCRIPTION) {
    /** @todo update pipeline table if table schema is changed */
  }

  sql = g_strdup_printf ("INSERT OR REPLACE INTO tblMLDBInfo VALUES ('tblPipeline', '%d');", TBL_VER_PIPELINE_DESCRIPTION);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to update version of pipeline table: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  /* Check model table. */
  rc = sqlite3_prepare_v2 (_db, "SELECT version FROM tblMLDBInfo WHERE name = 'tblModel';", -1, &res, nullptr);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to get the version of model table: %s (%d)", sqlite3_errmsg (_db), rc);
    return;
  }

  tbl_ver = (sqlite3_step (res) == SQLITE_ROW) ? sqlite3_column_int (res, 0) : TBL_VER_MODEL_INFO;
  sqlite3_finalize (res);

  if (tbl_ver != TBL_VER_MODEL_INFO) {
    /** @todo update model table if table schema is changed */
  }

  sql = g_strdup_printf ("INSERT OR REPLACE INTO tblMLDBInfo VALUES ('tblModel', '%d');", TBL_VER_MODEL_INFO);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to update version of model table: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  rc = sqlite3_exec (_db, "END TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to end transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return;
  }

  _initialized = true;
}

/**
 * @brief Connect to ML Service DB and initialize the private variables.
 */
void
MLServiceDB::connectDB ()
{
  int rc;

  if (_db != nullptr)
    return;

  rc = sqlite3_open (_path.c_str (), &_db);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to open database: %s (%d)", sqlite3_errmsg (_db), rc);
    goto error;
  }

  initDB ();

error:
  if (!_initialized) {
    disconnectDB ();
    throw std::runtime_error ("Failed to connect DB.");
  }
}

/**
 * @brief Disconnect the DB.
 */
void
MLServiceDB::disconnectDB ()
{
  if (_db) {
    sqlite3_close (_db);
    _db = nullptr;
  }
}

/**
 * @brief Set the pipeline description with the given name.
 * @note If the name already exists, the pipeline description is overwritten.
 * @param[in] name Unique name to set the associated pipeline description.
 * @param[in] description The pipeline description to be stored.
 */
void
MLServiceDB::set_pipeline (const std::string name, const std::string description)
{
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty () || description.empty ())
    throw std::invalid_argument ("Invalid name or value parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* (key, description) */
  sql = g_strdup_printf ("INSERT OR REPLACE INTO tblPipeline VALUES ('%s', '%s');",
      key_with_prefix.c_str (), description.c_str ());
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to insert pipeline description with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to insert pipeline description.");
  }
}

/**
 * @brief Get the pipeline description with the given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] description The pipeline corresponding with the given name.
 */
void
MLServiceDB::get_pipeline (const std::string name, std::string &description)
{
  int rc;
  char *sql;
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  sql = g_strdup_printf ("SELECT description FROM tblPipeline WHERE key = '%s';",
      key_with_prefix.c_str ());
  rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to get pipeline description with name %s: %s (%d)", name.c_str (), sqlite3_errmsg (_db), rc);
    goto error;
  }

  rc = sqlite3_step (res);
  if (rc == SQLITE_ROW)
    value = g_strdup_printf ("%s", sqlite3_column_text (res, 0));

  sqlite3_finalize (res);

error:
  if (value) {
    description = std::string (value);
    g_free (value);
  } else {
    throw std::invalid_argument ("Failed to get pipeline description.");
  }
}

/**
 * @brief Delete the pipeline description with a given name.
 * @param[in] name The unique name to delete
 */
void
MLServiceDB::delete_pipeline (const std::string name)
{
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  sql = g_strdup_printf ("DELETE FROM tblPipeline WHERE key = '%s';",
      key_with_prefix.c_str ());
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to delete pipeline description with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to delete pipeline description.");
  }

  /* count the number of rows modified */
  rc = sqlite3_changes (_db);
  if (rc == 0) {
    g_warning ("No pipeline description with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::invalid_argument ("There is no pipeline description with the given name.");
  }
}

/**
 * @brief Set the model with the given name.
 * @param[in] name Unique name for model.
 * @param[in] model The model to be stored.
 */
void
MLServiceDB::set_model (const std::string name, const std::string model)
{
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty () || model.empty ())
    throw std::invalid_argument ("Invalid name or value parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* (key, version, stable, valid, path) */
  sql = g_strdup_printf ("INSERT OR REPLACE INTO tblModel VALUES ('%s', IFNULL ((SELECT version from tblModel WHERE key = '%s' ORDER BY version DESC LIMIT 1) + 1, 1), 'T', 'T', '%s');",
      key_with_prefix.c_str (), key_with_prefix.c_str (), model.c_str ());
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to insert pipeline description with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to insert model.");
  }
}

/**
 * @brief Get the model with the given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] model The model corresponding with the given name.
 */
void
MLServiceDB::get_model (const std::string name, std::string &model)
{
  int rc;
  char *sql;
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  sql = g_strdup_printf ("SELECT path FROM tblModel WHERE key = '%s' ORDER BY version DESC LIMIT 1;",
      key_with_prefix.c_str ());
  rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to get pipeline description with name %s: %s (%d)", name.c_str (), sqlite3_errmsg (_db), rc);
    goto error;
  }

  rc = sqlite3_step (res);
  if (rc == SQLITE_ROW)
    value = g_strdup_printf ("%s", sqlite3_column_text (res, 0));

  sqlite3_finalize (res);

error:
  if (value) {
    model = std::string (value);
    g_free (value);
  } else {
    throw std::invalid_argument ("Failed to get model.");
  }
}

/**
 * @brief Delete the model.
 * @param[in] name The unique name to delete
 */
void
MLServiceDB::delete_model (const std::string name)
{
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  sql = g_strdup_printf ("DELETE FROM tblModel WHERE key = '%s';",
      key_with_prefix.c_str ());
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_warning ("Failed to delete model with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::invalid_argument ("Failed to delete model.");
  }
}
