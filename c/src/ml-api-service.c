/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service.c
 * @date 07 Mar 2022
 * @brief NNStreamer/Service C-API implementation
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <nnstreamer.h>
#include <leveldb/c.h>

#include "ml-api-inference-internal.h"
#include "ml-api-service.h"

#define ML_DATABASE_PATH      SYS_DB_DIR"/.ml-service-leveldb"

static leveldb_t *ml_service_db;
static leveldb_options_t *db_options;
static leveldb_readoptions_t *db_roptions;
static leveldb_writeoptions_t *db_woptions;

static int service_init_db (void);

int
ml_service_pipeline_add (const char *name, const char *pipeline_desc)
{
  int ret = 0;
  char *err = NULL;
  gsize name_len;
  gsize pipeline_len;

  if (!name || !pipeline_desc) {
    _ml_loge ("Error! name and pipeline_desc should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!ml_service_db) {
    ret = service_init_db ();
    if (ret < 0)
      return ret;
  }

  /* Put name and pipeline */
  name_len = strlen (name);
  pipeline_len = strlen (pipeline_desc);
  leveldb_put (ml_service_db, db_woptions, name, name_len,
      pipeline_desc, pipeline_len, &err);
  if (err != NULL) {
    _ml_loge ("Error! Failed to put %s : %s", name, err);
    leveldb_free (err);
    return -1;
  }

  return ML_ERROR_NONE;
}

int
ml_service_pipeline_get (const char *name, char **pipeline_desc)
{
  int ret = 0;
  gsize read_len;
  gsize name_len;
  char *err = NULL;
  char *value = NULL;

  if (!name) {
    _ml_loge ("Error! name should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!pipeline_desc) {
    _ml_loge ("Error! pipeline_desc should not be NULL");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!ml_service_db) {
    ret = service_init_db ();
    if (ret < 0)
      return ret;
  }

  /* Get the value */
  name_len = strlen (name);
  value = leveldb_get (ml_service_db, db_roptions, name, name_len,
      &read_len, &err);
  if (err != NULL) {
    _ml_loge ("Error! Fail to read %s : %s", name, err);
    leveldb_free (err);
    return -1;
  }

  *pipeline_desc = g_malloc (read_len + 1);
  memcpy (*pipeline_desc, value, read_len);
  pipeline_desc[read_len] = '\0';

  free (value);
  return ML_ERROR_NONE;
}

static int
service_init_db (void)
{
  char *err = NULL;

  /* Create database */
  db_options = leveldb_options_create ();
  leveldb_options_set_create_if_missing (db_options, 1);

  ml_service_db = leveldb_open (db_options, ML_DATABASE_PATH, &err);
  if (err != NULL) {
    _ml_loge ("Error! Failed to open Database : %s", err);
    leveldb_free (err);
    return -1;
  }

  db_roptions = leveldb_readoptions_create ();
  db_woptions = leveldb_writeoptions_create ();
  leveldb_writeoptions_set_sync (db_woptions, 1);

  return ML_ERROR_NONE;
}

/*
 * TODO: when is the DB closed? At Exit?
static int
service_finalize_db (void)
{
  if (ml_service_db) {
    leveldb_close(ml_service_db);
    ml_service_db = NULL;
  }

  return 0;
}
*/
