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

#include <sqlite3.h>
#include <iostream>

/**
 * @brief Class for ML-Service Database.
 */
class MLServiceDB
{
public:
  MLServiceDB (const MLServiceDB &) = delete;
  MLServiceDB (MLServiceDB &&) = delete;
  MLServiceDB & operator= (const MLServiceDB &) = delete;
  MLServiceDB & operator= (MLServiceDB &&) = delete;

  virtual void connectDB ();
  virtual void disconnectDB ();
  virtual void set_pipeline (const std::string name, const std::string description);
  virtual void get_pipeline (const std::string name, std::string &description);
  virtual void delete_pipeline (const std::string name);
  virtual void set_model (const std::string name, const std::string model);
  virtual void get_model (const std::string name, std::string &model);
  virtual void delete_model (const std::string name);

  static MLServiceDB & getInstance (void);

private:
  MLServiceDB (std::string path);
  virtual ~MLServiceDB ();

  std::string _path;
  bool _initialized;
  sqlite3 *_db;
};

#endif /* __SERVICE_DB_HH__ */
