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

#include <leveldb/c.h>
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
  virtual void put (const std::string name, const std::string value);
  virtual void get (std::string name, std::string &out_value);
  virtual void del (std::string name);

  static MLServiceDB & getInstance (void);

private:
  MLServiceDB (std::string path);
  virtual ~MLServiceDB ();

  std::string path;
  leveldb_t *db_obj;
  leveldb_readoptions_t *db_roptions;
  leveldb_writeoptions_t *db_woptions;
};

#endif /* __SERVICE_DB_HH__ */
