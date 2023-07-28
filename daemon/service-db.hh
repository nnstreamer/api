/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file service-db.hh
 * @date 28 Mar 2022
 * @brief NNStreamer/Service Database Interface
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#ifndef __SERVICE_DB_HH__
#define __SERVICE_DB_HH__

#include <glib.h>
#include <iostream>
#include <sqlite3.h>

/**
 * @brief Class for ML-Service Database.
 */
class MLServiceDB
{
  public:
  MLServiceDB (const MLServiceDB &) = delete;
  MLServiceDB (MLServiceDB &&) = delete;
  MLServiceDB &operator= (const MLServiceDB &) = delete;
  MLServiceDB &operator= (MLServiceDB &&) = delete;

  virtual void connectDB ();
  virtual void disconnectDB ();
  virtual void set_pipeline (const std::string name, const std::string description);
  virtual void get_pipeline (const std::string name, std::string &description);
  virtual void delete_pipeline (const std::string name);
  virtual void set_model (const std::string name, const std::string model, const bool is_active,
      const std::string description, const std::string app_info, guint *version);
  virtual void update_model_description (const std::string name,
      const guint version, const std::string description);
  virtual void activate_model (const std::string name, const guint version);
  virtual void get_model (const std::string name, std::string &model, const gint version);
  virtual void delete_model (const std::string name, const guint version);
  virtual void set_resource (const std::string name, const std::string path,
      const std::string description, const std::string app_info);
  virtual void get_resource (const std::string name, std::string &resource);
  virtual void delete_resource (const std::string name);

  static MLServiceDB &getInstance (void);

  private:
  MLServiceDB (std::string path);
  virtual ~MLServiceDB ();

  void initDB ();
  int get_table_version (const std::string tbl_name, const int default_ver);
  bool set_table_version (const std::string tbl_name, const int tbl_ver);
  bool create_table (const std::string tbl_name);
  bool set_transaction (bool begin);
  bool is_model_registered (const std::string key, const guint version);
  bool is_model_activated (const std::string key, const guint version);
  bool is_resource_registered (const std::string key);

  std::string _path;
  bool _initialized;
  sqlite3 *_db;
};

#endif /* __SERVICE_DB_HH__ */
