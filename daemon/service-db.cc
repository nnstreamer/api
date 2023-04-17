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
  [TBL_MODEL_INFO] = "tblModel (key TEXT NOT NULL, version INTEGER DEFAULT 1, active TEXT DEFAULT 'N', "
      "path TEXT, description TEXT, app_info TEXT, PRIMARY KEY (key, version), CHECK (length(path) > 0), CHECK (active IN ('T', 'F')))",
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
  sqlite3_stmt *res;

  if (name.empty () || description.empty ())
    throw std::invalid_argument ("Invalid name or value parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  if (sqlite3_prepare_v2 (_db, "INSERT OR REPLACE INTO tblPipeline VALUES (?1, ?2)", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 2, description.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to insert pipeline description of " + name);
  }

  sqlite3_finalize (res);
}

/**
 * @brief Get the pipeline description with the given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] description The pipeline corresponding with the given name.
 */
void
MLServiceDB::get_pipeline (const std::string name, std::string &description)
{
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  if (sqlite3_prepare_v2 (_db, "SELECT description FROM tblPipeline WHERE key = ?1", -1, &res, nullptr) == SQLITE_OK &&
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) == SQLITE_OK &&
      sqlite3_step (res) == SQLITE_ROW)
    value = g_strdup_printf ("%s", sqlite3_column_text (res, 0));

  sqlite3_finalize (res);

  if (value) {
    description = std::string (value);
    g_free (value);
  } else {
    throw std::invalid_argument ("Failed to get pipeline description of " + name);
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
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  if (sqlite3_prepare_v2 (_db, "DELETE FROM tblPipeline WHERE key = ?1", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to delete pipeline description of " + name);
  }

  sqlite3_finalize (res);

  /* count the number of rows modified */
  rc = sqlite3_changes (_db);
  if (rc == 0) {
    throw std::invalid_argument ("There is no pipeline description of " + name);
  }
}

/**
 * @brief Set the model with the given name.
 * @param[in] name Unique name for model.
 * @param[in] model The model to be stored.
 * @param[in] is_active The model is active or not.
 * @param[in] description The model description.
 * @param[out] version The version of the model.
 */
void
MLServiceDB::set_model (const std::string name, const std::string model, const bool is_active, const std::string description, const std::string app_info, guint *version)
{
  int rc;
  char *errmsg = nullptr;
  guint _version = 0U;
  sqlite3_stmt *res;

  if (name.empty () || model.empty () || !version)
    throw std::invalid_argument ("Invalid name, model, or version parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  rc = sqlite3_exec (_db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to begin transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to begin transaction.");
  }

  /* set other models as NOT active */
  if (is_active) {
    if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET active = 'F' WHERE key = ?1", -1, &res, nullptr) != SQLITE_OK ||
        sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
        sqlite3_step (res) != SQLITE_DONE) {
      sqlite3_finalize (res);
      throw std::runtime_error ("Failed to set other models as NOT active.");
    }
    sqlite3_finalize (res);
  }

  /* insert new row */
  if (sqlite3_prepare_v2 (_db, "INSERT OR REPLACE INTO tblModel VALUES (?1, IFNULL ((SELECT version from tblModel WHERE key = ?2 ORDER BY version DESC LIMIT 1) + 1, 1), ?3, ?4, ?5, ?6)", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 2, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 3, is_active ? "T" : "F", -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 4, model.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 5, description.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 6, app_info.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to register the model " + name);
  }

  sqlite3_finalize (res);

  /* END TRANSACTION */
  rc = sqlite3_exec (_db, "END TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to end transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to end transaction.");
  }

  long long int last_id = sqlite3_last_insert_rowid (_db);
  if (last_id == 0) {
    g_critical ("Failed to get last inserted row id: %s", sqlite3_errmsg (_db));
    throw std::runtime_error ("Failed to get last inserted row id.");
  }

  /* get model's version */
  if (sqlite3_prepare_v2 (_db, "SELECT version FROM tblModel WHERE rowid = ? ORDER BY version DESC LIMIT 1;", -1, &res, nullptr) == SQLITE_OK &&
      sqlite3_bind_int (res, 1, last_id) == SQLITE_OK &&
      sqlite3_step (res) == SQLITE_ROW) {
    _version = sqlite3_column_int (res, 0);
  }

  sqlite3_finalize (res);

  if (_version == 0) {
    g_critical ("Failed to get model version with name %s: %s (%d)", name.c_str (), sqlite3_errmsg (_db), rc);
    throw std::invalid_argument ("Failed to get model version of " + name);
  }

  *version = _version;
}

/**
 * @brief Update the model description with the given name.
 * @param[in] name Unique name for model.
 * @param[in] version The version of the model.
 * @param[in] description The model description.
 */
void
MLServiceDB::update_model_description (const std::string name, const guint version, const std::string description)
{
  sqlite3_stmt *res;

  if (name.empty () || description.empty ())
    throw std::invalid_argument ("Invalid name or description parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* check the existence of given model */
  if (sqlite3_prepare_v2 (_db, "SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = ?1 AND version = ?2)", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_int (res, 2, version) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_ROW ||
      sqlite3_column_int (res, 0) != 1) {
    sqlite3_finalize (res);
    throw std::invalid_argument ("Failed to check the existence of " + name + " version " + std::to_string (version));
  }

  sqlite3_finalize (res);

  /* update model description */
  if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET description = ?1 WHERE key = ?2 AND version = ?3", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, description.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_text (res, 2, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_int (res, 3, version) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to update model description.");
  }

  sqlite3_finalize (res);
}

/**
 * @brief Activate the model with the given name.
 * @param[in] name Unique name for model.
 * @param[in] version The version of the model.
 */
void
MLServiceDB::activate_model (const std::string name, const guint version)
{
  int rc;
  char *errmsg = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* check the existence */
  if (sqlite3_prepare_v2 (_db, "SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = ?1 AND version = ?2)", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_int (res, 2, version) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_ROW ||
      sqlite3_column_int (res, 0) != 1) {
    sqlite3_finalize (res);
    throw std::invalid_argument ("There is no model with name " + name + " and version " + std::to_string (version));
  }

  sqlite3_finalize (res);

  rc = sqlite3_exec (_db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to begin transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to begin transaction.");
  }

  /* set other row active as F */
  if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET active = 'F' WHERE key = ?1", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to deactivate other models of " + name);
  }

  sqlite3_finalize (res);

  /* set the given row active as T */
  if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET active = 'T' WHERE key = ?1 AND version = ?2", -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_bind_int (res, 2, version) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to activate model with name " + name + " of version " + std::to_string (version));
  }

  sqlite3_finalize (res);

  rc = sqlite3_exec (_db, "END TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to end transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to end transaction.");
  }
}

/**
 * @brief Get the model with the given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] model The model corresponding with the given name.
 * @param[in] version The version of the model. If it is 0, all models will return, if it is -1, return the active model.
 */
void
MLServiceDB::get_model (const std::string name, std::string &model, const gint version)
{
  char *sql;
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  if (version == 0)
    sql = g_strdup ("SELECT json_group_array(json_object('key', key, 'version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description, 'app_info', app_info)) FROM tblModel WHERE key = ?1");
  else if (version == -1)
    sql = g_strdup ("SELECT json_object('key', key, 'version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description, 'app_info', app_info) FROM tblModel WHERE key = ?1 and active = 'T' ORDER BY version DESC LIMIT 1");
  else if (version > 0)
    sql = g_strdup_printf ("SELECT json_object('key', key, 'version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description, 'app_info', app_info) FROM tblModel WHERE key = ?1 and version = %d", version);
  else
    throw std::invalid_argument ("Invalid version parameter!");

  if (sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) == SQLITE_OK &&
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) == SQLITE_OK &&
      sqlite3_step (res) == SQLITE_ROW)
    value = g_strdup_printf ("%s", sqlite3_column_text (res, 0));

  sqlite3_finalize (res);
  g_free (sql);

  if (value) {
    model = std::string (value);
    g_free (value);
  } else {
    throw std::invalid_argument ("Failed to get model with name " + name + " and version " + std::to_string (version));
  }
}

/**
 * @brief Delete the model.
 * @param[in] name The unique name to delete
 * @param[in] version The version of the model to delete
 */
void
MLServiceDB::delete_model (const std::string name, const guint version)
{
  int rc;
  char *sql;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* existence check */
  if (0U != version) {
    if (sqlite3_prepare_v2 (_db, "SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = ?1 AND version = ?2);", -1, &res, nullptr) != SQLITE_OK ||
        sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
        sqlite3_bind_int (res, 2, version) != SQLITE_OK ||
        sqlite3_step (res) != SQLITE_ROW) {
      sqlite3_finalize (res);
      throw std::runtime_error ("Failed to check the existence of model with name " + name + " and version " + std::to_string (version));
    }

    if (sqlite3_column_int (res, 0) != 1) {
      sqlite3_finalize (res);
      throw std::invalid_argument ("There is no model with name " + name + " and version " + std::to_string (version));
    }

    sqlite3_finalize (res);
  }

  if (0U == version)
    sql = g_strdup ("DELETE FROM tblModel WHERE key = ?1");
  else
    sql = g_strdup_printf ("DELETE FROM tblModel WHERE key = ?1 and version = %u", version);

  if (sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) != SQLITE_OK ||
      sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, NULL) != SQLITE_OK ||
      sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    g_free (sql);
    throw std::runtime_error ("Failed to delete model with name " + name + " and version " + std::to_string (version));
  }

  sqlite3_finalize (res);
  g_free (sql);

  rc = sqlite3_changes (_db);
  if (rc == 0) {
    throw std::invalid_argument ("There is no model with the given name " + name + " and version " + std::to_string (version));
  }
}
