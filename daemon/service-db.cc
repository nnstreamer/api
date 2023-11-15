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

#include "service-db.hh"
#include "log.h"

#define sqlite3_clear_errmsg(m) \
  do {                          \
    if (m) {                    \
      sqlite3_free (m);         \
      (m) = nullptr;            \
    }                           \
  } while (0)

#define ML_DATABASE_PATH DB_PATH "/.ml-service.db"
#define DB_KEY_PREFIX MESON_KEY_PREFIX

/**
 * @brief The version of pipeline table schema. It should be a positive integer.
 */
#define TBL_VER_PIPELINE_DESCRIPTION (1)

/**
 * @brief The version of model table schema. It should be a positive integer.
 */
#define TBL_VER_MODEL_INFO (1)

/**
 * @brief The version of resource table schema. It should be a positive integer.
 */
#define TBL_VER_RESOURCE_INFO (1)

typedef enum {
  TBL_DB_INFO = 0,
  TBL_PIPELINE_DESCRIPTION = 1,
  TBL_MODEL_INFO = 2,
  TBL_RESOURCE_INFO = 3,

  TBL_MAX
} mlsvc_table_e;

const char *g_mlsvc_table_schema_v1[] = { [TBL_DB_INFO] = "tblMLDBInfo (name TEXT PRIMARY KEY NOT NULL, version INTEGER DEFAULT 1)",
  [TBL_PIPELINE_DESCRIPTION] = "tblPipeline (key TEXT PRIMARY KEY NOT NULL, description TEXT, CHECK (length(description) > 0))",
  [TBL_MODEL_INFO]
  = "tblModel (key TEXT NOT NULL, version INTEGER DEFAULT 1, active TEXT DEFAULT 'F', "
    "path TEXT, description TEXT, app_info TEXT, PRIMARY KEY (key, version), CHECK (length(path) > 0), CHECK (active IN ('T', 'F')))",
  [TBL_RESOURCE_INFO] = "tblResource (key TEXT NOT NULL, path TEXT, description TEXT, app_info TEXT, PRIMARY KEY (key, path), CHECK (length(path) > 0))",
  NULL };

const char **g_mlsvc_table_schema = g_mlsvc_table_schema_v1;

/**
 * @brief Get an instance of MLServiceDB, which is created only once at runtime.
 * @return MLServiceDB& MLServiceDB instance
 */
MLServiceDB &
MLServiceDB::getInstance (void)
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
  int i, tbl_ver;

  if (_initialized)
    return;

  /**
   * @todo data migration
   * handle database version and update each table
   * 1. get all records from table
   * 2. drop old table
   * 3. create new table and insert records
   */
  if (!set_transaction (true))
    return;

  /* Create tables. */
  for (i = 0; i < TBL_MAX; i++) {
    if (!create_table (g_mlsvc_table_schema[i]))
      return;
  }

  /* Check pipeline table. */
  if ((tbl_ver = get_table_version ("tblPipeline", TBL_VER_PIPELINE_DESCRIPTION)) < 0)
    return;

  if (tbl_ver != TBL_VER_PIPELINE_DESCRIPTION) {
    /** @todo update pipeline table if table schema is changed */
  }

  if (!set_table_version ("tblPipeline", TBL_VER_PIPELINE_DESCRIPTION))
    return;

  /* Check model table. */
  if ((tbl_ver = get_table_version ("tblModel", TBL_VER_MODEL_INFO)) < 0)
    return;

  if (tbl_ver != TBL_VER_MODEL_INFO) {
    /** @todo update model table if table schema is changed */
  }

  if (!set_table_version ("tblModel", TBL_VER_MODEL_INFO))
    return;

  /* Check resource table. */
  if ((tbl_ver = get_table_version ("tblResource", TBL_VER_RESOURCE_INFO)) < 0)
    return;

  if (tbl_ver != TBL_VER_RESOURCE_INFO) {
    /** @todo update resource table if table schema is changed */
  }

  if (!set_table_version ("tblResource", TBL_VER_RESOURCE_INFO))
    return;

  if (!set_transaction (false))
    return;

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
    _E ("Failed to open database: %s (%d)", sqlite3_errmsg (_db), rc);
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
 * @brief Get table version.
 */
int
MLServiceDB::get_table_version (const std::string tbl_name, const int default_ver)
{
  int rc, tbl_ver;
  sqlite3_stmt *res;
  std::string sql = "SELECT version FROM tblMLDBInfo WHERE name = '" + tbl_name + "';";

  rc = sqlite3_prepare_v2 (_db, sql.c_str (), -1, &res, nullptr);
  if (rc != SQLITE_OK) {
    _W ("Failed to get the version of table %s: %s (%d)", tbl_name.c_str (),
        sqlite3_errmsg (_db), rc);
    return -1;
  }

  tbl_ver = (sqlite3_step (res) == SQLITE_ROW) ? sqlite3_column_int (res, 0) : default_ver;
  sqlite3_finalize (res);

  return tbl_ver;
}

/**
 * @brief Set table version.
 */
bool
MLServiceDB::set_table_version (const std::string tbl_name, const int tbl_ver)
{
  sqlite3_stmt *res;
  std::string sql = "INSERT OR REPLACE INTO tblMLDBInfo VALUES (?1, ?2);";

  bool is_done = (sqlite3_prepare_v2 (_db, sql.c_str (), -1, &res, nullptr) == SQLITE_OK
                  && sqlite3_bind_text (res, 1, tbl_name.c_str (), -1, nullptr) == SQLITE_OK
                  && sqlite3_bind_int (res, 2, tbl_ver) == SQLITE_OK
                  && sqlite3_step (res) == SQLITE_DONE);

  sqlite3_finalize (res);

  if (!is_done)
    _W ("Failed to update version of table %s.", tbl_name.c_str ());
  return is_done;
}

/**
 * @brief Create DB table.
 */
bool
MLServiceDB::create_table (const std::string tbl_name)
{
  int rc;
  char *errmsg = nullptr;
  std::string sql = "CREATE TABLE IF NOT EXISTS " + tbl_name;

  rc = sqlite3_exec (_db, sql.c_str (), nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    _W ("Failed to create table %s: %s (%d)", tbl_name.c_str (), errmsg, rc);
    sqlite3_clear_errmsg (errmsg);
    return false;
  }

  return true;
}

/**
 * @brief Begin/end transaction.
 */
bool
MLServiceDB::set_transaction (bool begin)
{
  int rc;
  char *errmsg = nullptr;

  rc = sqlite3_exec (_db, begin ? "BEGIN TRANSACTION;" : "END TRANSACTION;",
      nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK)
    _W ("Failed to %s transaction: %s (%d)", begin ? "begin" : "end", errmsg, rc);

  sqlite3_clear_errmsg (errmsg);
  return (rc == SQLITE_OK);
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

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_pipeline_");
  key_with_prefix += name;

  if (!set_transaction (true))
    throw std::runtime_error ("Failed to begin transaction.");

  if (sqlite3_prepare_v2 (_db,
          "INSERT OR REPLACE INTO tblPipeline VALUES (?1, ?2)", -1, &res, nullptr)
          != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 2, description.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to insert pipeline description of " + name);
  }

  sqlite3_finalize (res);

  if (!set_transaction (false))
    throw std::runtime_error ("Failed to end transaction.");
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

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_pipeline_");
  key_with_prefix += name;

  if (sqlite3_prepare_v2 (_db,
          "SELECT description FROM tblPipeline WHERE key = ?1", -1, &res, nullptr)
          == SQLITE_OK
      && sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) == SQLITE_OK
      && sqlite3_step (res) == SQLITE_ROW)
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
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_pipeline_");
  key_with_prefix += name;

  if (sqlite3_prepare_v2 (_db, "DELETE FROM tblPipeline WHERE key = ?1", -1, &res, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to delete pipeline description of " + name);
  }

  sqlite3_finalize (res);

  if (sqlite3_changes (_db) == 0) {
    throw std::invalid_argument ("There is no pipeline description of " + name);
  }
}

/**
 * @brief Check the model is registered.
 */
bool
MLServiceDB::is_model_registered (const std::string key, const guint version)
{
  sqlite3_stmt *res;
  gchar *sql;
  bool registered;

  if (version > 0U)
    sql = g_strdup_printf (
        "SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = ?1 AND version = %u)", version);
  else
    sql = g_strdup ("SELECT EXISTS(SELECT 1 FROM tblModel WHERE key = ?1)");

  registered
      = !(sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) != SQLITE_OK
          || sqlite3_bind_text (res, 1, key.c_str (), -1, nullptr) != SQLITE_OK
          || sqlite3_step (res) != SQLITE_ROW || sqlite3_column_int (res, 0) != 1);
  sqlite3_finalize (res);
  g_free (sql);

  return registered;
}

/**
 * @brief Check the model is activated.
 */
bool
MLServiceDB::is_model_activated (const std::string key, const guint version)
{
  sqlite3_stmt *res;
  gchar *sql;
  bool activated;

  sql = g_strdup ("SELECT active FROM tblModel WHERE key = ?1 AND version = ?2");

  activated = !(sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) != SQLITE_OK
                || sqlite3_bind_text (res, 1, key.c_str (), -1, nullptr) != SQLITE_OK
                || sqlite3_bind_int (res, 2, version) != SQLITE_OK
                || sqlite3_step (res) != SQLITE_ROW
                || !g_str_equal (sqlite3_column_text (res, 0), "T"));
  sqlite3_finalize (res);
  g_free (sql);

  return activated;
}

/**
 * @brief Check the resource is registered.
 */
bool
MLServiceDB::is_resource_registered (const std::string key)
{
  sqlite3_stmt *res;
  gchar *sql;
  bool registered;

  sql = g_strdup_printf ("SELECT EXISTS(SELECT 1 FROM tblResource WHERE key = ?1)");

  registered
      = !(sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) != SQLITE_OK
          || sqlite3_bind_text (res, 1, key.c_str (), -1, nullptr) != SQLITE_OK
          || sqlite3_step (res) != SQLITE_ROW || sqlite3_column_int (res, 0) != 1);
  sqlite3_finalize (res);
  g_free (sql);

  return registered;
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
MLServiceDB::set_model (const std::string name, const std::string model, const bool is_active,
    const std::string description, const std::string app_info, guint *version)
{
  guint _version = 0U;
  sqlite3_stmt *res;

  if (name.empty () || model.empty () || !version)
    throw std::invalid_argument ("Invalid name, model, or version parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_model_");
  key_with_prefix += name;

  if (!set_transaction (true))
    throw std::runtime_error ("Failed to begin transaction.");

  /* set other models as NOT active */
  if (is_active) {
    if (sqlite3_prepare_v2 (_db,
            "UPDATE tblModel SET active = 'F' WHERE key = ?1", -1, &res, nullptr)
            != SQLITE_OK
        || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
        || sqlite3_step (res) != SQLITE_DONE) {
      sqlite3_finalize (res);
      throw std::runtime_error ("Failed to set other models as NOT active.");
    }
    sqlite3_finalize (res);
  }

  /* insert new row */
  if (sqlite3_prepare_v2 (_db, "INSERT OR REPLACE INTO tblModel VALUES (?1, IFNULL ((SELECT version from tblModel WHERE key = ?2 ORDER BY version DESC LIMIT 1) + 1, 1), ?3, ?4, ?5, ?6)",
          -1, &res, nullptr)
          != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 2, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 3, is_active ? "T" : "F", -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 4, model.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 5, description.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 6, app_info.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to register the model " + name);
  }

  sqlite3_finalize (res);

  long long int last_id = sqlite3_last_insert_rowid (_db);
  if (last_id == 0) {
    _E ("Failed to get last inserted row id: %s", sqlite3_errmsg (_db));
    throw std::runtime_error ("Failed to get last inserted row id.");
  }

  /* get model's version */
  if (sqlite3_prepare_v2 (_db, "SELECT version FROM tblModel WHERE rowid = ? ORDER BY version DESC LIMIT 1;",
          -1, &res, nullptr)
          == SQLITE_OK
      && sqlite3_bind_int (res, 1, last_id) == SQLITE_OK && sqlite3_step (res) == SQLITE_ROW) {
    _version = sqlite3_column_int (res, 0);
  }

  sqlite3_finalize (res);

  if (!set_transaction (false))
    throw std::runtime_error ("Failed to end transaction.");

  if (_version == 0) {
    _E ("Failed to get model version with name %s: %s", name.c_str (),
        sqlite3_errmsg (_db));
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
MLServiceDB::update_model_description (
    const std::string name, const guint version, const std::string description)
{
  sqlite3_stmt *res;

  if (name.empty () || description.empty ())
    throw std::invalid_argument ("Invalid name or description parameter!");

  if (version == 0U)
    throw std::invalid_argument ("Invalid version number!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_model_");
  key_with_prefix += name;

  /* check the existence of given model */
  if (!is_model_registered (key_with_prefix, version)) {
    throw std::invalid_argument ("Failed to check the existence of " + name
                                 + " version " + std::to_string (version));
  }

  if (!set_transaction (true))
    throw std::runtime_error ("Failed to begin transaction.");

  /* update model description */
  if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET description = ?1 WHERE key = ?2 AND version = ?3",
          -1, &res, nullptr)
          != SQLITE_OK
      || sqlite3_bind_text (res, 1, description.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 2, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_int (res, 3, version) != SQLITE_OK || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to update model description.");
  }

  sqlite3_finalize (res);

  if (!set_transaction (false))
    throw std::runtime_error ("Failed to end transaction.");
}

/**
 * @brief Activate the model with the given name.
 * @param[in] name Unique name for model.
 * @param[in] version The version of the model.
 */
void
MLServiceDB::activate_model (const std::string name, const guint version)
{
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameter!");

  if (version == 0U)
    throw std::invalid_argument ("Invalid version number!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_model_");
  key_with_prefix += name;

  /* check the existence */
  if (!is_model_registered (key_with_prefix, version)) {
    throw std::invalid_argument ("There is no model with name " + name
                                 + " and version " + std::to_string (version));
  }

  if (!set_transaction (true))
    throw std::runtime_error ("Failed to begin transaction.");

  /* set other row active as F */
  if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET active = 'F' WHERE key = ?1", -1, &res, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to deactivate other models of " + name);
  }

  sqlite3_finalize (res);

  /* set the given row active as T */
  if (sqlite3_prepare_v2 (_db, "UPDATE tblModel SET active = 'T' WHERE key = ?1 AND version = ?2",
          -1, &res, nullptr)
          != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_int (res, 2, version) != SQLITE_OK || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to activate model with name " + name
                              + " and version " + std::to_string (version));
  }

  sqlite3_finalize (res);

  if (!set_transaction (false))
    throw std::runtime_error ("Failed to end transaction.");
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
  const char model_info_json[]
      = "json_object('version', CAST(version AS TEXT), 'active', active, 'path', path, 'description', description, 'app_info', app_info)";
  char *sql;
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_model_");
  key_with_prefix += name;

  if (version == 0)
    sql = g_strdup_printf (
        "SELECT json_group_array(%s) FROM tblModel WHERE key = ?1", model_info_json);
  else if (version == -1)
    sql = g_strdup_printf ("SELECT %s FROM tblModel WHERE key = ?1 and active = 'T' ORDER BY version DESC LIMIT 1",
        model_info_json);
  else if (version > 0)
    sql = g_strdup_printf ("SELECT %s FROM tblModel WHERE key = ?1 and version = %d",
        model_info_json, version);
  else
    throw std::invalid_argument ("Invalid version parameter!");

  if (sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) == SQLITE_OK
      && sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) == SQLITE_OK
      && sqlite3_step (res) == SQLITE_ROW)
    value = g_strdup_printf ("%s", sqlite3_column_text (res, 0));

  sqlite3_finalize (res);
  g_free (sql);

  if (value) {
    model = std::string (value);
    g_free (value);
  } else {
    throw std::invalid_argument ("Failed to get model with name " + name
                                 + " and version " + std::to_string (version));
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
  char *sql;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_model_");
  key_with_prefix += name;

  /* existence check */
  if (!is_model_registered (key_with_prefix, version)) {
    throw std::invalid_argument ("There is no model with name " + name
                                 + " and version " + std::to_string (version));
  }

  if (version > 0U) {
    if (is_model_activated (key_with_prefix, version))
      throw std::invalid_argument ("The model with name " + name
                                   + " and version " + std::to_string (version)
                                   + " is activated, cannot delete it.");

    sql = g_strdup_printf ("DELETE FROM tblModel WHERE key = ?1 and version = %u", version);
  } else {
    sql = g_strdup ("DELETE FROM tblModel WHERE key = ?1");
  }

  if (sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    g_free (sql);
    throw std::runtime_error ("Failed to delete model with name " + name
                              + " and version " + std::to_string (version));
  }

  sqlite3_finalize (res);
  g_free (sql);

  if (sqlite3_changes (_db) == 0) {
    throw std::invalid_argument ("There is no model with the given name " + name
                                 + " and version " + std::to_string (version));
  }
}

/**
 * @brief Set the resource with given name.
 * @param[in] name Unique name of ml-resource.
 * @param[in] path The path to be stored.
 * @param[in] description The description for ml-resource.
 */
void
MLServiceDB::set_resource (const std::string name, const std::string path,
    const std::string description, const std::string app_info)
{
  sqlite3_stmt *res;

  if (name.empty () || path.empty ())
    throw std::invalid_argument ("Invalid name or path parameter!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_resource_");
  key_with_prefix += name;

  if (!set_transaction (true))
    throw std::runtime_error ("Failed to begin transaction.");

  if (sqlite3_prepare_v2 (_db,
          "INSERT OR REPLACE INTO tblResource VALUES (?1, ?2, ?3, ?4)", -1, &res, nullptr)
          != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 2, path.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 3, description.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 4, app_info.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    throw std::runtime_error ("Failed to add the resource " + name);
  }

  sqlite3_finalize (res);

  if (!set_transaction (false))
    throw std::runtime_error ("Failed to end transaction.");

  long long int last_id = sqlite3_last_insert_rowid (_db);
  if (last_id == 0) {
    _E ("Failed to get last inserted row id: %s", sqlite3_errmsg (_db));
    throw std::runtime_error ("Failed to get last inserted row id.");
  }
}

/**
 * @brief Get the resource with given name.
 * @param[in] name The unique name to retrieve.
 * @param[out] resource The resource corresponding with the given name.
 */
void
MLServiceDB::get_resource (const std::string name, std::string &resource)
{
  const char res_info_json[]
      = "json_object('path', path, 'description', description, 'app_info', app_info)";
  char *sql;
  char *value = nullptr;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_resource_");
  key_with_prefix += name;

  /* existence check */
  if (!is_resource_registered (key_with_prefix))
    throw std::invalid_argument ("There is no resource with name " + name);

  /* Get json string with insertion order. */
  sql = g_strdup_printf ("SELECT json_group_array(%s) FROM (SELECT * FROM tblResource WHERE key = ?1 ORDER BY ROWID ASC)",
      res_info_json);

  if (sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) == SQLITE_OK
      && sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) == SQLITE_OK
      && sqlite3_step (res) == SQLITE_ROW)
    value = g_strdup_printf ("%s", sqlite3_column_text (res, 0));

  sqlite3_finalize (res);
  g_free (sql);

  if (!value)
    throw std::invalid_argument ("Failed to get resource with name " + name);

  resource = std::string (value);
  g_free (value);
}

/**
 * @brief Delete the resource.
 * @param[in] name The unique name to delete
 */
void
MLServiceDB::delete_resource (const std::string name)
{
  char *sql;
  sqlite3_stmt *res;

  if (name.empty ())
    throw std::invalid_argument ("Invalid name parameters!");

  std::string key_with_prefix = DB_KEY_PREFIX + std::string ("_resource_");
  key_with_prefix += name;

  /* existence check */
  if (!is_resource_registered (key_with_prefix))
    throw std::invalid_argument ("There is no resource with name " + name);

  sql = g_strdup ("DELETE FROM tblResource WHERE key = ?1");

  if (sqlite3_prepare_v2 (_db, sql, -1, &res, nullptr) != SQLITE_OK
      || sqlite3_bind_text (res, 1, key_with_prefix.c_str (), -1, nullptr) != SQLITE_OK
      || sqlite3_step (res) != SQLITE_DONE) {
    sqlite3_finalize (res);
    g_free (sql);
    throw std::runtime_error ("Failed to delete resource with name " + name);
  }

  sqlite3_finalize (res);
  g_free (sql);

  if (sqlite3_changes (_db) == 0)
    throw std::invalid_argument ("There is no resource with name " + name);
}
