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
      "path TEXT, description TEXT, PRIMARY KEY (key, version), CHECK (length(path) > 0), CHECK (active IN ('T', 'F')))",
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
  char *sql;
  char *errmsg = nullptr;

  if (_initialized)
    return;

  /**
   * @todo handle database version
   * - check old level db and insert pipeline description into sqlite.
   * - check database info (TBL_DB_INFO), fetch data and update table schema when version is updated, ...
   * - need transaction
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
 * @param[in] is_active The model is active or not.
 * @param[in] description The model description.
 * @param[out] version The version of the model.
 */
void
MLServiceDB::set_model (const std::string name, const std::string model, const bool is_active, const std::string description, guint *version)
{
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty () || model.empty ())
    throw std::invalid_argument ("Invalid name or value parameters!");

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
    sql = g_strdup_printf ("UPDATE tblModel SET active = 'F' WHERE key = '%s';",
        key_with_prefix.c_str ());
    rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
    g_free (sql);
    if (rc != SQLITE_OK) {
      g_critical ("Failed to set other models as NOT active: %s (%d)", errmsg, rc);
      sqlite3_clear_errmsg (errmsg);
      throw std::runtime_error ("Failed to set other models as NOT active.");
    }
  }

  /* (key, version, active, path, description) */
  sql = g_strdup_printf ("INSERT OR REPLACE INTO tblModel VALUES ('%s', IFNULL ((SELECT version from tblModel WHERE key = '%s' ORDER BY version DESC LIMIT 1) + 1, 1), '%s', '%s', '%s');",
      key_with_prefix.c_str (), key_with_prefix.c_str (), is_active ? "T" : "F", model.c_str (), description.c_str ());

  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to register model with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to register model.");
  }

  /* END TRANSACTION */
  rc = sqlite3_exec (_db, "END TRANSACTION;", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to end transaction: %s (%d)", errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to end transaction.");
  }

  long long int last_id = sqlite3_last_insert_rowid (_db);
  if (last_id == 0) {
    g_critical ("Failed to get last inserted row id: %s", errmsg);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to get last inserted row id.");
  }

  /* get model's version */
  sql = g_strdup_printf ("SELECT version FROM tblModel WHERE rowid = %lld ORDER BY version DESC LIMIT 1;", last_id);

  sqlite3_stmt *res;
  rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);

  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to get model version with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to get model version.");
  }

  /* set rowid as the last_id int the sql */
  sqlite3_bind_int (res, 1, last_id);
  int step = sqlite3_step (res);
  if (step == SQLITE_ROW) {
    *version = sqlite3_column_int (res, 0);
  }

  sqlite3_finalize (res);

  if (version == 0) {
    g_critical ("Failed to get model version with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to get model version.");
  }
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
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* check the existence of given model */
  sql = g_strdup_printf ("SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = '%s' AND version = %u);",
      key_with_prefix.c_str (), version);

  sqlite3_stmt *res;
  rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to check the existence of model with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to check the existence of model.");
  }

  int step = sqlite3_step (res);
  if (step == SQLITE_ROW) {
    int count = sqlite3_column_int (res, 0);
    if (count != 1) {
      g_critical ("There is no model with name %s and version %u", name.c_str (), version);
      sqlite3_finalize (res);
      throw std::invalid_argument ("There is no model with the given name and version.");
    }
  }

  sqlite3_finalize (res);

  /* (key, version, active, path, description) */
  sql = g_strdup_printf ("UPDATE tblModel SET description = '%s' WHERE key = '%s' AND version = %u;",
      description.c_str (), key_with_prefix.c_str (), version);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to update model description with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to update model description.");
  }
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
  char *sql;
  char *errmsg = nullptr;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* check the existence */
  sql = g_strdup_printf ("SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = '%s' AND version = %u);",
      key_with_prefix.c_str (), version);

  sqlite3_stmt *res;
  rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to check the existence of model with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to check the existence of model.");
  }

  int step = sqlite3_step (res);
  if (step == SQLITE_ROW) {
    int count = sqlite3_column_int (res, 0);
    if (count != 1) {
      g_critical ("There is no model with name %s and version %u", name.c_str (), version);
      sqlite3_finalize (res);
      throw std::invalid_argument ("There is no model with the given name and version.");
    }
  }

  sqlite3_finalize (res);

  /* set other row active as F */
  sql = g_strdup_printf ("UPDATE tblModel SET active = 'F' WHERE key = '%s';", key_with_prefix.c_str ());
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to deactivate model other than name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to update model description.");
  }

  /* (key, version, active, path, description) */
  sql = g_strdup_printf ("UPDATE tblModel SET active = 'T' WHERE key = '%s' AND version = %u;",
      key_with_prefix.c_str (), version);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to activate model with name %s with version %u: %s (%d)", name.c_str (), version, errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::runtime_error ("Failed to update model description.");
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
  int rc;
  char *sql;
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  if (version == 0)
    sql = g_strdup_printf ("SELECT json_group_array(json_object('key', key, 'version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description)) FROM tblModel WHERE key = '%s';",
      key_with_prefix.c_str ());
  else if (version == -1)
    sql = g_strdup_printf ("SELECT json_object('key', key, 'version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description) FROM tblModel WHERE key = '%s' and active = 'T' ORDER BY version DESC LIMIT 1;",
        key_with_prefix.c_str ());
  else if (version > 0)
    sql = g_strdup_printf ("SELECT json_object('key', key, 'version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description) FROM tblModel WHERE key = '%s' and version = %d;",
        key_with_prefix.c_str (), version);
  else
    throw std::invalid_argument ("Invalid version parameter!");

  rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to get model info with name %s: %s (%d)", name.c_str (), sqlite3_errmsg (_db), rc);
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
 * @param[in] version The version of the model to delete
 */
void
MLServiceDB::delete_model (const std::string name, const guint version)
{
  int rc;
  char *sql;
  char *errmsg = nullptr;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX;
  key_with_prefix += name;

  /* existence check */
  if (0U != version) {
    sql = g_strdup_printf ("SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = '%s' AND version = %u);",
      key_with_prefix.c_str (), version);

    sqlite3_stmt *res;
    rc = sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr);
    g_free (sql);
    if (rc != SQLITE_OK) {
      g_critical ("Failed to check the existence of model with name %s: %s (%d)", name.c_str (), errmsg, rc);
      sqlite3_clear_errmsg (errmsg);
      throw std::runtime_error ("Failed to check the existence of model.");
    }

    int step = sqlite3_step (res);
    if (step == SQLITE_ROW) {
      int count = sqlite3_column_int (res, 0);
      if (count != 1) {
        g_critical ("There is no model with name %s and version %u", name.c_str (), version);
        sqlite3_finalize (res);
        throw std::invalid_argument ("There is no model with the given name and version.");
      }
    }

    sqlite3_finalize (res);
  }

  if (0U == version)
    sql = g_strdup_printf ("DELETE FROM tblModel WHERE key = '%s';",
      key_with_prefix.c_str ());
  else
    sql = g_strdup_printf ("DELETE FROM tblModel WHERE key = '%s' and version = %u;",
      key_with_prefix.c_str (), version);
  rc = sqlite3_exec (_db, sql, nullptr, nullptr, &errmsg);
  g_free (sql);
  if (rc != SQLITE_OK) {
    g_critical ("Failed to delete model with name %s: %s (%d)", name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    throw std::invalid_argument ("Failed to delete model.");
  }
}
